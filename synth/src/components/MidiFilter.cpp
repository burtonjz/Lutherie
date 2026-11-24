#include "components/MidiFilter.hpp"

#include "capabilities/Capability.hpp"
#include "capabilities/MidiIO.hpp"     // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "meta/CollectionDescriptor.hpp"
#include "meta/ComponentRegistry.hpp"

MidiFilter::MidiFilter(int id, [[maybe_unused]] MidiFilterConfig cfg):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::ADSREnvelope),
    MidiProcessor(),
    midiValue_(createParameterAccessor())
{
    addCapability<Capability::MidiIO>(*this, 1, 1);
    auto& params = addCapability<Capability::Parameters>();

    const CollectionDescriptor& col = ComponentRegistry
        ::getComponentDescriptor(componentType())
        .collection.value();

    params.addCollection(col);
    params.bindParameter(midiValue_, ParameterType::MIDI_VALUE);
}

void MidiFilter::handleKeyPressed(MidiNote note, size_t ch){    
    if ( passNote(note.note()) ) push(std::move(note), ch);
}

void MidiFilter::handleKeyReleased(MidiNote note, size_t ch){
    if ( passNote(note.note()) ) push(std::move(note), ch);
}

bool MidiFilter::passNote(uint8_t midi) const {
    for ( size_t i = 0 ; i < midiValue_.slotCount(); ++i ){
        if ( midiValue_.slotValue<ParameterType::MIDI_VALUE>(i) == midi ){
            return true ;
        } 
    }
    return false ;
}