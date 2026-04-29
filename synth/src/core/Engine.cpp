/*
 * Engine.cpp - Refactored with proper thread management
 */

#include "core/Engine.hpp"
#include "dsp/AnalyticsEngine.hpp"
#include "config/Config.hpp"
#include "api/ApiHandler.hpp"
#include "meta/ComponentRegistry.hpp"
#include "midi/MidiEventHandler.hpp"
#include "midi/MidiEventListener.hpp"
#include "types/SocketType.hpp"
#include "types/Waveform.hpp"
#include "midi/MidiController.hpp"
#include "dsp/detune.hpp"
#include "dsp/math.hpp"

#include <chrono>
#include <csignal>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>
#include <spdlog/spdlog.h>

// Static members
std::atomic<bool> Engine::stop_flag{false};

void Engine::signalHandler(int signum){
    SPDLOG_INFO("Caught signal {}, stopping threads...", signum);
    stop_flag = true;
}

// Constructor
Engine::Engine():
    // publically available interfaces
    componentManager(&midiController),
    componentFactory(&componentManager),
    signalController(&componentManager), 
    midiController(&midiState_),
    // thread state flags
    apiServerRunning_(false),
    engineRunning_(false),
    midiRunning_(false),
    audioRunning_(false),
    analysisRunning_(false),
    // audio 
    dac_(),
    availableAudioDevices_(),
    selectedAudioOutput_(0),
    // midi
    midiIn_(),
    availableMidiPorts_(),
    selectedMidiPort_(-1),
    midiState_(),
    midiDefaultHandler_()
{
    ApiHandler::instance()->initialize(this);

    registerBaseMidiHandler(&midiDefaultHandler_);
    midiController.addHandler(&midiDefaultHandler_);
    signal(SIGINT, Engine::signalHandler);

    dsp::initializeDetuneLUT();
}

// Destructor - ensure clean shutdown
Engine::~Engine() {
    shutdown();
}

// ============================================================================
// STATE FUNCTIONS
// ============================================================================

void Engine::initialize(){
    Config::load();

    // Get MIDI list
    int numPorts = midiIn_.getPortCount();
    for (int i = 0; i < numPorts; ++i){
        availableMidiPorts_[i] = midiIn_.getPortName(i);
    }

    // Get audio device list
    std::vector<unsigned int> devices = dac_.getDeviceIds();
    for (auto dev : devices){
        auto info = dac_.getDeviceInfo(dev);
        availableAudioDevices_[info.ID] = info.name;
    }

    // Start API server thread
    apiServerRunning_ = true;
    apiServerThread_ = std::thread([&](){
        ApiHandler::instance()->start();
    });

    SPDLOG_INFO("Engine initialized. API server running.");

    while ( !stop_flag && apiServerRunning_ ){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Engine::run(){
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    if (engineRunning_){
        SPDLOG_WARN("Engine already running!");
        return;
    }

    SPDLOG_INFO("Starting engine...");
    
    setup();
    
    engineRunning_ = true;
    
    // start threads
    midiRunning_ = true;
    midiThread_ = std::thread(&Engine::midiLoop, this);
    
    audioRunning_ = true;
    audioThread_ = std::thread(&Engine::audioLoop, this);
    
    analysisRunning_ = true;
    analysisThread_ = std::thread(&Engine::analysisLoop, this);
    
    SPDLOG_INFO("Engine running with 3 worker threads.");
}

void Engine::stop(){
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    if (!engineRunning_){
        SPDLOG_WARN("Engine not running!");
        return;
    }
    
    SPDLOG_INFO("Stopping engine...");
    
    // Signal all threads to stop
    engineRunning_ = false;
    audioRunning_ = false;
    midiRunning_ = false;
    analysisRunning_ = false;
    
    // Unlock before joining to avoid deadlock
    lock.unlock();
    
    stopAudio();
    
    if (audioThread_.joinable()){
        SPDLOG_INFO("Waiting for audio thread...");
        audioThread_.join();
    }
    
    if (midiThread_.joinable()){
        SPDLOG_INFO("Waiting for MIDI thread...");
        midiThread_.join();
    }
    
    if (analysisThread_.joinable()){
        SPDLOG_INFO("Waiting for analysis thread...");
        analysisThread_.join();
    }
    
    stopMidi();
    
    SPDLOG_INFO("Engine stopped");
}

void Engine::shutdown(){
    SPDLOG_INFO("Shutting down engine...");
    
    // Stop engine if running
    if (engineRunning_){
        stop();
    }
    
    // Stop API server
    stop_flag = true;
    apiServerRunning_ = false;
    
    if (apiServerThread_.joinable()){
        SPDLOG_INFO("Waiting for API server thread...");
        apiServerThread_.join();
    }
    
    SPDLOG_INFO("Engine shutdown complete");
}

// ============================================================================
// THREAD LOOPS
// ============================================================================

void Engine::midiLoop(){
    SPDLOG_INFO("MIDI thread started");
    
    // Open MIDI port
    int deviceId = getMidiDeviceId();
    if (deviceId >= 0){
        midiIn_.openPort(deviceId);
        midiIn_.setCallback(&MidiController::onMidiEvent, static_cast<void*>(&midiController));
        midiIn_.ignoreTypes(false, false, false);
        SPDLOG_INFO("Listening for MIDI input on device id {}", deviceId);
    }
    
    // Keep thread alive while running
    while (midiRunning_ && engineRunning_){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    SPDLOG_INFO("MIDI thread stopping");
}

void Engine::audioLoop(){
    SPDLOG_INFO("Audio thread started");
    
    // Initialize audio
    unsigned int buffer = Config::get<unsigned int>("audio.buffer_size").value();
    unsigned int sampleRate = Config::get<unsigned int>("audio.sample_rate").value();
    
    RtAudio::StreamParameters parameters;
    RtAudio::StreamOptions options ;
    options.flags = RTAUDIO_SCHEDULE_REALTIME ;
    options.priority = 50 ;

    std::map<int,std::string> devices = getAvailableAudioDevices();
    int deviceId = getAudioDeviceId();
    RtAudio::DeviceInfo deviceInfo;
    void* userData = static_cast<void*>(this);
    
    // Select device
    if (deviceId == 0 || devices.find(deviceId) == devices.end()){
        deviceInfo = dac_.getDeviceInfo(dac_.getDefaultOutputDevice());
    } else {
        deviceInfo = dac_.getDeviceInfo(deviceId);
    }
    
    parameters.deviceId = deviceInfo.ID;
    parameters.nChannels = 1;
    parameters.firstChannel = 0;
    
    // Validate sample rate
    auto &sampleRates = deviceInfo.sampleRates;
    if (std::count(sampleRates.begin(), sampleRates.end(), sampleRate) == 0){
        SPDLOG_WARN("Configured sample rate of {} is not supported by device {}.", sampleRate, deviceInfo.name);
        SPDLOG_INFO("Setting to device preferred sample rate of {}.", deviceInfo.preferredSampleRate);
        sampleRate = deviceInfo.preferredSampleRate;
        Config::set("audio.sample_rate", sampleRate);
    }

    sampleRate_ = sampleRate ;
    
    // Open audio stream
    if (dac_.openStream(
        &parameters,
        NULL,
        RTAUDIO_FLOAT64,
        sampleRate,
        &buffer,
        &audioCallback,
        userData,
        &options
    )){
        SPDLOG_ERROR("Error initializing audio: {}",  dac_.getErrorText());
        audioRunning_ = false;
        return;
    }
    
    // Start the stream
    if (dac_.startStream()){
        SPDLOG_ERROR("Error starting audio stream: {}",  dac_.getErrorText());
        if (dac_.isStreamOpen()){
            dac_.closeStream();
        }
        audioRunning_ = false;
        return;
    }
    
    SPDLOG_INFO("Audio stream started");
    
    // Keep thread alive while running
    // The actual audio processing happens in the callback
    while (audioRunning_ && engineRunning_){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    SPDLOG_INFO("Audio thread stopping");
}

void Engine::analysisLoop(){
    SPDLOG_INFO("Analysis thread started");
    
    Config::load();
    AnalyticsEngine::instance()->start();

    while ( analysisRunning_ && engineRunning_ ){
        AnalyticsEngine::instance()->processContexts();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    
    AnalyticsEngine::instance()->stop();
    SPDLOG_INFO("Analysis thread stopping");
}

// ============================================================================
// AUDIO CALLBACK (runs in audio thread context)
// ============================================================================

int Engine::audioCallback(
    void *outputBuffer, [[maybe_unused]] void *inputBuffer, 
    unsigned int nBufferFrames, [[maybe_unused]] double streamTime, 
    [[maybe_unused]] RtAudioStreamStatus status, void *userData 
){
    Engine* engine = static_cast<Engine*>(userData);
    double* buffer = static_cast<double*>(outputBuffer);
    
    // Check if we should still be running
    if (!engine->audioRunning_ || !engine->engineRunning_){
        // Fill buffer with silence and signal to stop
        std::fill_n(buffer, nBufferFrames, 0.0);
        return 1; // Non-zero signals stream should stop
    }

    float bufferDt = nBufferFrames / static_cast<float>(engine->getSampleRate());
    engine->midiController.tick(bufferDt);
    double sample;
    for (unsigned int i = 0; i < nBufferFrames; ++i){
        engine->componentManager.runParameterModulation();
        sample = engine->signalController.processFrame();
        buffer[i] = dsp::fastAtan(sample);
    }
    
    engine->componentManager.runAnalyzers();
    
    return 0;
}

// ============================================================================
// CLEANUP FUNCTIONS
// ============================================================================

void Engine::stopAudio(){
    SPDLOG_INFO("Stopping audio stream...");
    if (dac_.isStreamRunning()){
        dac_.stopStream();
    }
    if (dac_.isStreamOpen()){
        dac_.closeStream();
    }
}

void Engine::stopMidi(){
    SPDLOG_INFO("Stopping MIDI...");
    if (midiIn_.isPortOpen()){
        midiIn_.cancelCallback();
        midiIn_.closePort();
    }
}

void Engine::audioCleanup(RtAudio* dac, RtAudioErrorType error){
    if (error) SPDLOG_ERROR(dac->getErrorText());
    if (dac->isStreamOpen()){
        dac->closeStream();
    }
}

// ============================================================================
// SETUP AND TEARDOWN
// ============================================================================

void Engine::setup(){
    sampleRate_ = Config::get<unsigned int>("audio.sample_rate").value();
    
    midiController.initialize();
    signalController.updateProcessingGraph();
}

void Engine::destroy(){
    componentManager.reset();
    signalController.reset();
    midiState_.reset();
}

// ============================================================================
// GETTERS AND SETTERS
// ============================================================================

RtAudio* Engine::getDac(){
    return &dac_;
}

RtMidiIn* Engine::getMidiIn(){
    return &midiIn_;
}

MidiController* Engine::getMidiController(){
    return &midiController;
}

MidiEventHandler* Engine::getDefaultMidiHandler(){
    return &midiDefaultHandler_;
}

int Engine::getSampleRate() const {
    return sampleRate_ ;
}

int Engine::getAudioDeviceId() const {
    return selectedAudioOutput_;
}

bool Engine::setAudioDeviceId(int deviceId){
    auto it = availableAudioDevices_.find(deviceId);
    if ( it == availableAudioDevices_.end() ){
        SPDLOG_ERROR("Cannot set audio device ID to {}. Invalid ID",  deviceId);
        return false ;
    }

    SPDLOG_INFO("audio device id set to {}.", deviceId);
    selectedAudioOutput_ = deviceId ;
    return true ;
}

int Engine::getMidiDeviceId() const {
    return selectedMidiPort_;
}

bool Engine::setMidiDeviceId(int deviceId){
    auto it = availableMidiPorts_.find(deviceId);
    if ( it == availableMidiPorts_.end() ){
        SPDLOG_ERROR("Cannot set midi device ID to {}. Invalid ID.", deviceId);
        return false ;
    }

    SPDLOG_INFO("midi device id set to {}.", deviceId);
    selectedMidiPort_ = deviceId ;
    return true ;
}

const std::map<int,std::string> Engine::getAvailableMidiDevices() const {
    return availableMidiPorts_;
}

const std::map<int, std::string> Engine::getAvailableAudioDevices() const {
    return availableAudioDevices_;
}

// ============================================================================
// MIDI CONNECTION MANAGEMENT
// ============================================================================

bool Engine::handleMidiConnection(ConnectionRequest request){
    BaseComponent* inbound = nullptr ;
    BaseComponent* outbound = nullptr ;

    if ( request.inboundID.has_value() ){
        inbound = componentManager.getRaw(request.inboundID.value());
    }

    if ( request.outboundID.has_value() ){
        outbound = componentManager.getMidiHandler(request.outboundID.value());
    }
    
    // Case 1: inbound and outbound components both exist
    if ( inbound && outbound ){
        auto inboundDescriptor = ComponentRegistry::getComponentDescriptor(inbound->getType());
        auto outboundDescriptor = ComponentRegistry::getComponentDescriptor(outbound->getType());

        if ( inboundDescriptor.isMidiListener() && outboundDescriptor.isMidiHandler() ){
            auto handler = dynamic_cast<MidiEventHandler*>(outbound);
            auto listener = dynamic_cast<MidiEventListener*>(inbound);

            if ( request.remove ){
                handler->removeListener(listener);   
            } else {
                handler->addListener(listener);
            }
            return true ;
        }
    }
    
    // case 2: outbound is undefined
    if ( ! outbound && inbound ){
        auto inboundDescriptor = ComponentRegistry::getComponentDescriptor(inbound->getType());

        // A: inbound is a handler, so we need to register this handler against the MidiState (receive raw midi events)
        if ( inboundDescriptor.isMidiHandler() ){
            auto handler = dynamic_cast<MidiEventHandler*>(inbound);
            if ( request.remove ){
                unregisterBaseMidiHandler(handler);
                return true ;
            }
            registerBaseMidiHandler(handler);
            return true ;
        }

        // B: inbound is only a listener (so we register to the default handler)
        if ( inboundDescriptor.isMidiListener() ){
            auto listener = dynamic_cast<MidiEventListener*>(inbound);
            if ( request.remove ){
                getDefaultMidiHandler()->removeListener(listener);
                return true ;
            }
            getDefaultMidiHandler()->addListener(listener);
            return true ;
        }
    }

    // case 3: only outbound specified
    json j = request ;
    SPDLOG_ERROR("Invalid connection request. {}", j.dump());
    return false ;
}

bool Engine::registerBaseMidiHandler(MidiEventHandler* handler){
    if (!handler){
        SPDLOG_WARN("specified midi handler is a null pointer. Unable to register.");
        return false;
    }
    midiState_.addHandler(handler);
    return true;
}

bool Engine::unregisterBaseMidiHandler(MidiEventHandler* handler){
    if (!handler){
        SPDLOG_WARN("Specified midi handler is a null pointer. Unable to unregister.");
        return false;
    }
    midiState_.removeHandler(handler);
    return true;
}

bool Engine::handleSignalConnection(ConnectionRequest request){
    BaseModule* inbound = nullptr ;
    BaseModule* outbound = nullptr ;
    Analyzer* analyzer = nullptr ;

    // inbound component can be module, analyzer, or peripheral
    analyzer = componentManager.getAnalyzer(request.inboundID.value_or(-1));
    inbound = componentManager.getModule(request.inboundID.value_or(-1));
    
    // outbound can be module only
    outbound = componentManager.getModule(request.outboundID.value_or(-1));

    // Case 1: if outbound is a peripheral
    if ( ! request.outboundID.has_value() ){
        SPDLOG_WARN("receiving audio from an peripheral source is not yet supported.");
        return false ;
    }

    // Case 2: if inbound is a peripheral
    if ( ! request.inboundID.has_value() ){
        if ( request.remove ){
            signalController.unregisterSink(outbound, request.outboundIdx.value());
            return true ;
        }
        signalController.registerSink(outbound, request.outboundIdx.value());
        return true ;
    }

    // Case 3: analyzer inbound
    if ( analyzer ){
        if ( outbound ){
            if ( request.remove ){
                analyzer->disconnectInput(outbound, request.outboundIdx.value());
            } else {
                analyzer->connectInput(outbound, request.outboundIdx.value());
            }
            return true ;
        } else {
            SPDLOG_WARN("analyzer component defined but outbound module invalid. Cannot connect");
            return false ;
        }
    }

    // Case 4: module to module
    if ( request.remove ){
        signalController.disconnect(outbound, request.outboundIdx.value(), inbound, request.inboundIdx.value());
        return true ;
    }

    signalController.connect(outbound, request.outboundIdx.value(), inbound, request.inboundIdx.value());
    return true ;
}

std::vector<ConnectionRequest> Engine::getComponentConnections(ComponentId id) const {
    std::vector<ConnectionRequest> requests ;
    getComponentConnections(id, requests);
    return requests ;
}

void Engine::getComponentConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    componentManager.getComponentConnections(id, requests);
    getPeripheralConnections(id, requests);
};

void Engine::getPeripheralConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    // check if module is a sink
    BaseModule* m = componentManager.getModule(id);
    if ( m ){
        for ( const auto& conn : signalController.getSinks()){
            if ( m == conn.module ){
                ConnectionRequest req ;
                req.outboundID = id ;
                req.outboundIdx = conn.index ;
                req.inboundSocket = SocketType::SignalInbound ;
                req.outboundSocket = SocketType::SignalOutbound ;
                req.inboundIdx = 0 ; // TODO: support multi-channel sink
                requests.push_back(req);
            }
        }
    }
    
    // midi peripherals are registered to midi state or to the default midi handler
    MidiEventHandler* handler = componentManager.getMidiHandler(id);
    MidiEventListener* listener = componentManager.getMidiListener(id);
    if ( handler ){
        auto handlers = midiState_.getHandlers();
        if ( std::find(handlers.begin(), handlers.end(), handler) != handlers.end() ){
            ConnectionRequest req ;
            req.inboundID = id ;
            req.inboundSocket = SocketType::MidiInbound ;
            req.outboundSocket = SocketType::MidiOutbound ;
            requests.push_back(req);
        }
    } else if ( listener ){
        for ( auto h : listener->getHandlers() ){
            if ( h == &midiDefaultHandler_ ){
                ConnectionRequest req ;
                req.inboundID = id ;
                req.inboundSocket = SocketType::MidiInbound ;
                req.outboundSocket = SocketType::MidiOutbound ;
                requests.push_back(req);
            }
        }
    }
}

bool Engine::handleModulationConnection(ConnectionRequest request){
    if ( ! request.outboundID.has_value() || ! request.inboundID.has_value() ){
        SPDLOG_ERROR("modulation connections must have valid IDs for both inbound and outbound objects.");
        return false ;
    }

    BaseModulator* modulator = componentManager.getModulator(request.outboundID.value());
    BaseComponent* component = componentManager.getRaw(request.inboundID.value());

    if (!modulator || !component ){
        SPDLOG_ERROR("valid modulator or component not found.");;
        return false ;
    }

    if ( request.depthConnection ){
        if ( request.remove ){
            component->removeParameterDepthModulation(request.inboundParameter.value());
        } else {
            component->setParameterDepthModulation(request.inboundParameter.value(), modulator);
        }
    } else {
        if ( request.remove ){
            component->removeParameterModulation(request.inboundParameter.value());
        } else {
            component->setParameterModulation(request.inboundParameter.value(), modulator);
        }
    }
    
    // stateful modulators need to be in the signal processing graph
    if ( dynamic_cast<BaseModule*>(modulator) ){
        signalController.updateProcessingGraph();
    }

    return true ;
}

// ============================================================================
// SERIALIZATION
// ============================================================================
json Engine::serialize() const {
    json output ;

    // capture component data (parameters, midi, modulation, signal)
    output["components"] = componentManager.serializeComponents();

    // capture connections
    std::vector<ConnectionRequest> connections ;
    for ( auto id : componentManager.getComponentIds() ){
        getComponentConnections(id, connections);
    }
    std::set<ConnectionRequest> unique(connections.begin(), connections.end());

    output["connections"];
    for ( auto r : unique ){
        output["connections"].push_back(r);
    }

    return output ;
}