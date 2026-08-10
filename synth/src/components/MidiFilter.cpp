#include "components/MidiFilter.hpp"
#include "midi/MidiEventHandler.hpp"
#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"

MidiFilter::MidiFilter(ComponentId id, [[maybe_unused]] MidiFilterConfig cfg):
    BaseComponent(id, ComponentType::MidiFilter)
{
    parameters_->addCollection<ParameterType::MIDI_VALUE>({});
}

void MidiFilter::onKeyPressed(const ActiveNote* note, bool rePressed){
    if ( !note ) return ;
    
    if ( passNote(note->note.getMidiNote()) ){
        MidiEventHandler::onKeyPressed(note, rePressed);
    } 
}

void MidiFilter::onKeyReleased(ActiveNote anote){
    if ( passNote(anote.note.getMidiNote()) ){
        MidiEventHandler::onKeyReleased(anote);
    }
}

bool MidiFilter::passNote(uint8_t midi) const {
    auto c = parameters_->getCollection<ParameterType::MIDI_VALUE>();
    for ( const auto& idx : c->getIndices() ){
        if ( c->getValue(idx) == midi ) return true ;
    }
    return false ;
}