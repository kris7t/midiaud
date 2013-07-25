#ifndef JACK_MIDI_SINK_H_
#define JACK_MIDI_SINK_H_

#include <jack/jack.h>
#include <jack/midiport.h>

namespace midiaud {

class JackMidiSink {
 public:
  JackMidiSink(jack_port_t *port, jack_nframes_t nframes,
               jack_nframes_t framerate);

  void WriteMidi(double offset_seconds,
                 const jack_midi_data_t *data, size_t size);
  void WriteProgramChange(double offset_seconds,
                          uint8_t channel, uint8_t program);
  void WriteNoteOn(double offset_seconds, uint8_t channel,
                   uint8_t note, uint8_t velocity);
  void WritePitchWheelChange(double offset_seconds,
                          uint8_t channel, uint16_t pitch);
  void WriteControlChange(double offset_seconds,
                          uint8_t channel, uint8_t control,
                          uint8_t value);
  void WriteAllSoundOff(double offset_seconds, uint8_t channel);
  void WriteGlobalSoundOff(double offset_seconds);
  void WriteResetAllControllers(double offset_seconds,
                                uint8_t channel);
  void WriteGlobalResetControllers(double offset_seconds);

 private:
  void *buffer_;
  jack_nframes_t framerate_;
};

}

#endif // JACK_MIDI_SINK_H_
