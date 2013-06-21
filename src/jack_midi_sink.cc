
#include "jack_midi_sink.h"

#include <stdexcept>

namespace midiaud {

JackMidiSink::JackMidiSink(jack_port_t *port, jack_nframes_t nframes,
                           jack_nframes_t framerate) 
    : buffer_(jack_port_get_buffer(port, nframes)),
      framerate_(framerate) {
  if (buffer_ == nullptr)
    throw std::runtime_error("jack_port_get_buffer failed");
  jack_midi_clear_buffer(buffer_);
}

void JackMidiSink::WriteMidi(double offset_seconds,
                             jack_midi_data_t *data,
                             size_t size) {
  jack_nframes_t offset =
      static_cast<jack_nframes_t>(offset_seconds * framerate_);
  if (jack_midi_event_write(buffer_, offset, data, size) != 0)
    throw std::runtime_error("jack_midi_event_write failure");
}

void JackMidiSink::WriteProgramChange(double offset_seconds,
                                      uint8_t channel,
                                      uint8_t program) {
  jack_midi_data_t buffer[] = {
    static_cast<jack_midi_data_t>(0xc0 | channel), program
  };
  WriteMidi(offset_seconds, buffer, sizeof(buffer));
}

void JackMidiSink::WriteNoteOn(double offset_seconds, uint8_t channel,
                               uint8_t note, uint8_t velocity) {
  jack_midi_data_t buffer[] = {
    static_cast<jack_midi_data_t>(0x90 | channel), note, velocity
  };
  WriteMidi(offset_seconds, buffer, sizeof(buffer));
}

void JackMidiSink::WritePitchWheelChange(double offset_seconds,
                                         uint8_t channel,
                                         uint16_t pitch) {
  jack_midi_data_t least = static_cast<jack_midi_data_t>(pitch & 0x7f);
  jack_midi_data_t most = static_cast<jack_midi_data_t>((pitch >> 7) & 0x7f);
  jack_midi_data_t buffer[] = {
    static_cast<jack_midi_data_t>(0xd0 | channel), least, most
  };
  WriteMidi(offset_seconds, buffer, sizeof(buffer));
}

void JackMidiSink::WriteControlChange(double offset_seconds,
                                      uint8_t channel, uint8_t control,
                                      uint8_t value) {
  jack_midi_data_t buffer[] = {
    static_cast<jack_midi_data_t>(0xb0 | channel), control, value
  };
  WriteMidi(offset_seconds, buffer, sizeof(buffer));
}

void JackMidiSink::WriteAllSoundOff(double offset_seconds,
                                    uint8_t channel) {
  WriteControlChange(offset_seconds, channel, 0x78, 0x00);
}

void JackMidiSink::WriteGlobalSoundOff(double offset_seconds) {
  for (uint8_t channel = 0; channel < 16; ++channel)
    WriteAllSoundOff(offset_seconds, channel);
}

void JackMidiSink::WriteResetAllControllers(double offset_seconds,
                                            uint8_t channel) {
  WriteControlChange(offset_seconds, channel, 0x79, 0x00);
}

void JackMidiSink::WriteGlobalResetControllers(double offset_seconds) {
  for (uint8_t channel = 0; channel < 16; ++channel)
    WriteAllSoundOff(offset_seconds, channel);
}

}
