
#include "timebase/tempo_map.h"

#include <cmath>
#include <algorithm>
#include <iostream>

#include <boost/assert.hpp>

namespace midiaud {
namespace timebase {

Position TempoMap::kDefaultTempo = {
  0, 0, { 1, 1, 0 }, 4, 4, 768, 120
};

TempoMap::TempoMap()
    : ppqn_(TempoMap::kDefaultTempo.ticks_per_beat) {
  positions_.push_back(kDefaultTempo);
}

TempoMap::TempoMap(smf_t *smf)
    : ppqn_(smf->ppqn) {
  smf_tempo_t *initial_tempo = smf_get_tempo_by_number(smf, 0);
  if (initial_tempo == nullptr || initial_tempo->time_seconds != 0) {
    std::cerr << "Unknown initial tempo and time signature" << std::endl;
    Position default_position(kDefaultTempo);
    default_position.ticks_per_beat = ppqn_;
    positions_.push_back(default_position);
  }
  
  const Position *last_pos = &kDefaultTempo;
  smf_tempo_t *tempo;
  // TODO Here we should not rely on libsmf's internal tempo map. We
  // should parse the control messages: time signature changes should
  // start new bars, while tempo changes should not. Moreover, there
  // should be support to set the current bar number through MIDI.
  for (int i = 0; (tempo = smf_get_tempo_by_number(smf, i)) != nullptr; ++i) {
    Position current_pos;

    current_pos.seconds = tempo->time_seconds;
    current_pos.ticks = tempo->time_pulses;
    BOOST_ASSERT(last_pos->seconds <= current_pos.seconds);
    BOOST_ASSERT(last_pos->ticks <= current_pos.ticks);

    double delta_seconds = current_pos.seconds - last_pos->seconds;
    double delta_bars = delta_seconds * last_pos->beats_per_minute /
        (60 * last_pos->beat_type);
    BOOST_ASSERT(last_pos->bbt.beat == 1);
    BOOST_ASSERT(last_pos->bbt.tick == 0);
    // HACK Each time signature change starts a new bar.
    current_pos.bbt.bar = std::ceil(last_pos->bbt.bar + delta_bars);
    current_pos.bbt.beat = 1;
    current_pos.bbt.tick = 0;

    current_pos.beats_per_bar = tempo->numerator;
    current_pos.beat_type = tempo->denominator;
    // Work out how many beats are in a MIDI quarter based on how many
    // written 32nd notes are in it.
    double beats_per_quarter = static_cast<double>(tempo->notes_per_note)
        * current_pos.beat_type / 32;
    // PPQN stands for pulses per MIDI quarter.
    current_pos.ticks_per_beat = ppqn_ * beats_per_quarter;
    double qpm = 6.e7 / tempo->microseconds_per_quarter_note;
    current_pos.beats_per_minute = qpm * beats_per_quarter;

    positions_.push_back(std::move(current_pos));
    last_pos = &positions_.back();
  }
}

void TempoMap::FillBBT(jack_position_t *pos) const {
  Position pos_to_find;
  pos_to_find.seconds = static_cast<double>(pos->frame) / pos->frame_rate;
  auto last_pos = --std::upper_bound(positions_.begin(),
                                     positions_.end(),
                                     pos_to_find,
                                     ComparePositionBySeconds());
  BOOST_ASSERT(last_pos->bbt.beat == 1);
  BOOST_ASSERT(last_pos->bbt.tick == 0);

  // TODO This BBT calculation is extremely complicated, not to
  // mention incorrent.
  double delta_seconds = pos_to_find.seconds - last_pos->seconds;
  double delta_beats = delta_seconds * last_pos->beats_per_minute / 60;
  int32_t delta_bars = std::floor(delta_beats / last_pos->beats_per_bar);
  pos->bar = last_pos->bbt.bar + delta_bars;
  delta_beats -= delta_bars * last_pos->beats_per_bar;

  // delta_beats now holds the beats from the bar start.
  double delta_beats_ipart;
  double delta_beats_fpart = std::modf(delta_beats, &delta_beats_ipart);
  pos->beat = last_pos->bbt.beat + delta_beats_ipart;

  // delta_ticks now holds the ticks from the beat start.
  double delta_ticks = delta_beats_fpart * last_pos->ticks_per_beat;
  double delta_ticks_ceil = std::ceil(delta_ticks);
  pos->tick = last_pos->bbt.tick + delta_ticks_ceil;
  pos->bar_start_tick = last_pos->ticks +
      delta_bars * last_pos->beat_type * last_pos->ticks_per_beat;

  pos->beats_per_bar = last_pos->beats_per_bar;
  pos->beat_type = last_pos->beat_type;
  pos->ticks_per_beat = last_pos->ticks_per_beat;
  pos->beats_per_minute = last_pos->beats_per_minute;
  
  // offset_ticks holds the fractional ticks to the tick start.
  double offset_ticks = delta_ticks_ceil - delta_ticks;
  double offset_beats = offset_ticks / last_pos->ticks_per_beat;
  double offset_seconds = offset_beats * 60 / last_pos->beats_per_minute;
  pos->bbt_offset = offset_seconds * pos->frame_rate;

  pos->valid = static_cast<jack_position_bits_t>(
      pos->valid | JackPositionBBT | JackBBTFrameOffset);
}

jack_nframes_t TempoMap::BBTToFrame(jack_position_t *pos) const {
  (void) pos;
  // TODO
  return 0;
}

} // timebase
} // midiaud
