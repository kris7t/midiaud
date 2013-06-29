
#include "timebase/position.h"

#include <cmath>

#include <boost/assert.hpp>

static constexpr double kEps = 1e-9;

namespace midiaud {
namespace timebase {

Position::Position()
    : seconds_(0), ticks_(0),
      bbt_{Position::kInitialBar, Position::kInitialBeat, 0},
      beats_per_bar_(4), beat_type_(4),
      ticks_per_beat_(Position::kDefaultTicksPerBeat),
      beats_per_minute_(120) {
}

Position::Position(double seconds)
    : seconds_(seconds) {
}

Position::Position(const BBT &bbt)
    : bbt_(bbt) {
}

void Position::StartNewBar() {
  if (bbt_.beat != 1 || bbt_.tick > kEps) {
    bbt_.bar += 1;
    bbt_.beat = 1;
  }
  bbt_.tick = 0;
}

void Position::SetToSeconds(double seconds) {
  BOOST_ASSERT(seconds >= seconds_);
  IncrementBySeconds(seconds - seconds_);
}

void Position::SetToTicks(double ticks) {
  BOOST_ASSERT(ticks >= ticks_);
  IncrementByTicks(ticks - ticks_);
}

void Position::IncrementBySeconds(double seconds) {
  double ticks = SecondsToTicks(seconds);
  IncrementByTicks(ticks);
}

void Position::IncrementByTicks(double ticks) {
  seconds_ += TicksToSeconds(ticks);
  ticks_ += ticks;

  ticks += bbt_.tick;
  // Pretend that bbt_.tick = 0;
  double beats = ticks / ticks_per_beat_;
  double beat_increment;
  double fractional_beat = std::modf(beats, &beat_increment);
  double updated_tick = fractional_beat * ticks_per_beat_;
  bbt_.tick = updated_tick;
  
  // beat_increment is an integer.
  bbt_.beat += beat_increment;
  // We suppose that beats_per_bar is integer (it comes from a char).
  int32_t beats_per_bar_i = beats_per_bar_;
  bbt_.bar += (bbt_.beat - kInitialBeat) / beats_per_bar_i;
  bbt_.beat = (bbt_.beat - kInitialBeat) % beats_per_bar_i
      + kInitialBeat;
}

void Position::RoundUp() {
  double ticks_ceil = std::ceil(bbt_.tick);
  double tick_difference = ticks_ceil - bbt_.tick;
  IncrementByTicks(tick_difference);
}

double Position::SecondsToTicks(double seconds) const {
  double beats = seconds * beats_per_minute_ / 60;
  return beats * ticks_per_beat_;
}

double Position::TicksToSeconds(double ticks) const {
  double beats = ticks / ticks_per_beat_;
  return beats * 60 / beats_per_minute_;
}

} // midiaud
} // timebase
