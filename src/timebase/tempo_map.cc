
#include "timebase/tempo_map.h"

#include <cmath>
#include <algorithm>
#include <iostream>

#include <boost/assert.hpp>

static constexpr double kEps = 1e-9;

namespace midiaud {
namespace timebase {

TempoMap::TempoMap() {
  positions_.push_back(Position());
}

TempoMap::TempoMap(double ppqn)
    : TempoMap() {
  positions_.back().PpqnChange(ppqn);
}

void TempoMap::AcknowledgeEvent(const Event &event) {
  Position position(positions_.back());
  BOOST_ASSERT(position.ticks() <= event.ticks());
  position.SetToTicks(event.ticks());

  if (!event.is_metadata()) return;

  switch (event.midi()[1]) {
    case 0x58:
      if (event.midi().size() == 7) {
        // event->midi_buffer[2] is the metaevent length, which
        // takes a single byte with variable-length encoding.
        uint8_t numerator = event.midi()[3];
        uint8_t denomiator = std::pow(2, event.midi()[4]);
        uint8_t clocks_per_metronome_click = event.midi()[5];
        uint8_t thirty_seconds_per_midi_quarter = event.midi()[6];
        position.TimeSignatureChange(numerator, denomiator,
                                     clocks_per_metronome_click,
                                     thirty_seconds_per_midi_quarter);
        AppendOrReplace(position);
      } else {
        std::cerr << "Warning: ignoring invalid time signature metaevent"
                  << " at tick " << position.ticks() << "\n";
      }
      break;

    case 0x51:
      if (event.midi().size() == 6) {
        uint32_t microseconds_per_midi_quarter =
            (event.midi()[3] << 16) | (event.midi()[4] << 8) | event.midi()[5];
        position.TempoChange(microseconds_per_midi_quarter);
        AppendOrReplace(position);
      } else {
        std::cerr << "Warning: ignoring invalid tempo metaevent"
                  << " at tick " << position.ticks() << "\n";
      }
      break;
  }
}

Position TempoMap::GetSeconds(double seconds) const {
  Position pos_to_find((Position::ConstructFromSeconds()), seconds);
  auto upper_bound = std::upper_bound(positions_.cbegin(),
                                      positions_.cend(), pos_to_find,
                                      ComparePositionBySeconds());
  if (upper_bound == positions_.begin()) {
    // We can't go to before tick 0, so we hope the best.
    return positions_.front();
  }
  Position position(*--upper_bound);
  position.SetToSeconds(seconds);
  return position;
}

Position TempoMap::GetTicks(double ticks) const {
  Position pos_to_find((Position::ConstructFromTicks()), ticks);
  auto upper_bound = std::upper_bound(positions_.cbegin(),
                                      positions_.cend(), pos_to_find,
                                      ComparePositionByTicks());
  if (upper_bound == positions_.begin()) {
    return positions_.front();
  }
  Position position(*--upper_bound);
  position.SetToTicks(ticks);
  return position;
}

Position TempoMap::GetBBT(const BBT &bbt) const {
  Position pos_to_find(bbt);
  auto upper_bound = std::upper_bound(positions_.cbegin(),
                                      positions_.cend(), pos_to_find,
                                      ComparePositionByBBT());
  if (upper_bound == positions_.begin()) {
    return positions_.front();
  }
  Position position(*--upper_bound);
  position.SetToBBT(bbt);
  return position;
}

void TempoMap::FillBBT(jack_position_t *pos) const {
  double seconds = static_cast<double>(pos->frame) / pos->frame_rate;
  Position position(GetSeconds(seconds));
  position.RoundUp();
  pos->bar = position.bbt().bar;
  pos->beat = position.bbt().beat;
  pos->tick = position.bbt().tick;
  Position bar_start(GetBBT(position.bbt().last_bar_start()));
  // No need to round up, bar_start_tick is double.
  pos->bar_start_tick = bar_start.ticks();
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

void TempoMap::AppendOrReplace(const Position &position) {
  if (positions_.empty()) {
    positions_.push_back(position);
  } else {
    double diff = position.ticks() - positions_.back().ticks();
    if (diff > kEps) {
      positions_.push_back(position);
    } else if (diff >= 0) {
      positions_.back() = position;
    } else {
      throw std::runtime_error("Going back in tempo map is forbidden.");
    }
  }
}

} // timebase
} // midiaud
