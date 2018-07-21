#pragma once

#include <MIDI_Senders/DigitalNoteSender.hpp>
#include <MIDI_Outputs/Abstract/MIDIButtonLatching.hpp>

class NoteButtonLatching : public MIDIButtonLatching<DigitalNoteSender> {
  public:
    NoteButtonLatching(pin_t pin, uint8_t note, uint8_t channel)
        : MIDIButtonLatching(pin, note, channel) {}
};