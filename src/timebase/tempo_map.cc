
#include "timebase/tempo_map.h"

#include <cmath>
#include <algorithm>
#include <iostream>

#include <boost/assert.hpp>

namespace midiaud {
namespace timebase {

TempoMap::TempoMap() {
  positions_.push_back(Position());
}

TempoMap::TempoMap(smf_t *smf) {
  Position position;
  position.set_ticks_per_beat(smf->ppqn);

  smf_tempo_t *initial_tempo = smf_get_tempo_by_number(smf, 0);
  if (initial_tempo == nullptr || initial_tempo->time_seconds != 0) {
    std::cerr << "Warning: Unknown initial tempo and time signature"
              << std::endl;
    positions_.push_back(position);
  }
  
  smf_tempo_t *tempo;
  // TODO Here we should not rely on libsmf's internal tempo map. We
  // should parse the control messages: time signature changes should
  // start new bars, while tempo changes should not. Moreover, there
  // should be support to set the current bar number through MIDI.
  for (int i = 0;
       (tempo = smf_get_tempo_by_number(smf, i)) != nullptr;
       ++i) {
    position.SetToTicks(tempo->time_pulses);
    position.StartNewBar();

    position.set_beats_per_bar(tempo->numerator);
    position.set_beat_type(tempo->denominator);
    // Work out how many beats are in a MIDI quarter based on how many
    // written 32nd notes are in it.
    double beats_per_quarter = static_cast<double>(tempo->notes_per_note)
        * tempo->denominator / 32;
    // PPQN stands for pulses per MIDI quarter.
    position.set_ticks_per_beat(smf->ppqn * beats_per_quarter);
    double qpm = 6.e7 / tempo->microseconds_per_quarter_note;
    position.set_beats_per_minute(qpm * beats_per_quarter);

    positions_.push_back(position);
  }
}

void TempoMap::FillBBT(jack_position_t *pos) const {
  double seconds = static_cast<double>(pos->frame) / pos->frame_rate;
  Position position(Get(seconds));
  position.RoundUp();
  pos->bar = position.bbt().bar;
  pos->beat = position.bbt().beat;
  pos->tick = position.bbt().tick;
  pos->beats_per_bar = position.beats_per_bar();
  pos->beat_type = position.beat_type();
  pos->ticks_per_beat = position.ticks_per_beat();
  pos->beats_per_minute = position.beats_per_minute();
  double offset_seconds = position.seconds() - seconds;
  pos->bbt_offset = offset_seconds * pos->frame_rate;
  pos->valid = static_cast<jack_position_bits_t>(
      pos->valid | JackPositionBBT | JackBBTFrameOffset);
}

jack_nframes_t TempoMap::BBTToFrame(jack_position_t *pos) const {
  (void) pos;
  // TODO
  return 0;
}

Position TempoMap::Get(double seconds) const {
  Position pos_to_find(seconds);
  auto upper_bound = std::upper_bound(positions_.cbegin(),
                                      positions_.cend(), pos_to_find,
                                      ComparePositionBySeconds());
  BOOST_ASSERT(upper_bound != positions_.begin());
  Position position(*--upper_bound);
  position.SetToSeconds(seconds);
  return position;
}

} // timebase
} // midiaud
