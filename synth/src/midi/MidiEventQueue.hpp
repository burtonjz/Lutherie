/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __MIDI_EVENT_QUEUE_HPP
#define __MIDI_EVENT_QUEUE_HPP

#include <array>
#include <atomic>
#include "midi/MidiNote.hpp"

struct MidiEvent {
    enum class Type { NotePressed, NoteReleased, NoteOff };
    Type type ;
    ActiveNote anote ;
    bool rePressed = false ;
};

class MidiEventQueue {
private:
    std::array<MidiEvent, 128> buffer_ ;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};

public:
    bool push(const MidiEvent& event){
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = ( head + 1 ) % 128 ;
        if ( next == tail_.load(std::memory_order_acquire) ) return false ;
        buffer_[head] = event ;
        head_.store(next, std::memory_order_release);
        return true ;
    }

    bool pop( MidiEvent& outEvent ){
        size_t tail = tail_.load(std::memory_order_relaxed);
        if ( tail == head_.load(std::memory_order_acquire) ) return false ;
        outEvent = buffer_[tail];
        tail_.store((tail + 1) % 128, std::memory_order_release);
        return true ;
    }
};

#endif // __MIDI_EVENT_QUEUE_HPP