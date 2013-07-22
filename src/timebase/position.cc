
#include "timebase/position.h"

#include <cmath>

#include <boost/assert.hpp>

static constexpr double kEps = 1e-9;
static constexpr double kMicrosecondsPerMinute = 6.e7;

namespace midiaud {
namespace timebase {

Position::Position()
    : seconds_(0), ticks_(0),
      bbt_{BBT::kInitialBar, BBT::kInitialBeat, 0},
      beats_per_bar_(4), beat_type_(4),
      ticks_per_beat_(Position::kDefaultTicksPerBeat),
      beats_per_minute_(120),
      microseconds_per_midi_quarter_(
          kMicrosecondsPerMinute / beats_per_minute_
      ),
      beats_per_midi_quarter_(1), 
      ppqn_(Position::kDefaultTicksPerBeat) {
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
  if (seconds < seconds_) {
    // We cannot deal with this issue here. Throwing is unacceptable,
    // since we might end up in this branch due to a mere
    // floating-point error. Let's hope we are not off by too much and
    // nothing important is missed.
    return;
  }
  IncrementBySeconds(seconds - seconds_);
}

void Position::SetToTicks(double ticks) {
  if (ticks < ticks_) {
    // See SetToSeconds for explanation.
    return;
  }
  IncrementByTicks(ticks - ticks_);
}

void Position::SetToBBT(const BBT &bbt) {
  double bar_diff = bbt.bar - bbt_.bar;
  double beat_diff = bbt.beat - bbt_.beat + bar_diff * beats_per_bar_;
  double tick_diff = bbt.tick - bbt_.tick + beat_diff * ticks_per_beat_;
  if (tick_diff < 0) {
    // See SetToSeconds for explanation.
    return;
  }
  IncrementByTicks(tick_diff);
}

void Position::IncrementBySeconds(double seconds) {
  BOOST_ASSERT(seconds >= 0);

  double ticks = SecondsToTicks(seconds);
  IncrementByTicks(ticks);
}

void Position::IncrementByTicks(double ticks) {
  BOOST_ASSERT(ticks >= 0);

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
  bbt_.bar += (bbt_.beat - BBT::kInitialBeat) / beats_per_bar_i;
  bbt_.beat = (bbt_.beat - BBT::kInitialBeat) % beats_per_bar_i
      + BBT::kInitialBeat;
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

void Position::PpqnChange(double ppqn) {
  ppqn_ = ppqn;
  ticks_per_beat_ = ppqn_ / beats_per_midi_quarter_;
}

void Position::TimeSignatureChange(double beats_per_bar, double beat_type,
                                   double clocks_per_metronome_click,
                                   double thirty_seconds_per_midi_quarter) {
  // Ignore MIDI metronome information, we are not a metronome.
  (void) clocks_per_metronome_click;

  // Force a new bar to be started. Time signature changes never
  // appear in the middle of the bar in real music, and would confuse
  // the BBT calculation. Moreover, repeating a time signature could
  // be a workaround for notating anacrusis (pick-up bar).
  StartNewBar();

  beats_per_bar_ = beats_per_bar;
  beat_type_ = beat_type;
  beats_per_midi_quarter_ =
      thirty_seconds_per_midi_quarter * beat_type / 32;
  ticks_per_beat_ = ppqn_ / beats_per_midi_quarter_;
  double midi_quarters_per_minute =
      kMicrosecondsPerMinute / microseconds_per_midi_quarter_;
  beats_per_minute_ = midi_quarters_per_minute * beats_per_midi_quarter_;
}

void Position::TempoChange(double microseconds_per_midi_quarter) {
  microseconds_per_midi_quarter_ = microseconds_per_midi_quarter;
  double midi_quarters_per_minute =
      kMicrosecondsPerMinute / microseconds_per_midi_quarter_;
  beats_per_minute_ = midi_quarters_per_minute * beats_per_midi_quarter_;
}

} // midiaud
} // timebase
