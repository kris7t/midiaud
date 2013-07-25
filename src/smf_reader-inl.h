#ifndef SMF_READER_INL_H_
#define SMF_READER_INL_H_

#include <string>
#include <memory>

#include <smf.h>

#include "event.h"

namespace midiaud {

/**
 * Stateless deleter for `unique_ptr` to `smf_t`.
 */
struct DeleteSmfT {
  void operator()(smf_t *smf) {
    smf_delete(smf);
  }
};

template <typename OutputIterator>
void ReadStandardMidiFile(const std::string &filename,
                          OutputIterator result,
                          double &ppqn) {
  std::unique_ptr<smf_t, DeleteSmfT> smf(smf_load(filename.c_str()));
  if (smf == nullptr)
    throw std::runtime_error("smf_load failed");
  ReadFromSmfT(smf.get(), result, ppqn);
}

template <typename OutputIterator>
void ReadFromSmfT(smf_t *smf,
                  OutputIterator result,
                  double &ppqn) {
  ppqn = smf->ppqn;
  smf_rewind(smf);
  for (smf_event_t *event = smf_get_next_event(smf);
       event != nullptr;
       event = smf_get_next_event(smf)) {
    double ticks = event->time_pulses;
    unsigned char *begin = event->midi_buffer;
    unsigned char *end = event->midi_buffer + event->midi_buffer_length;
    *result++ = Event(ticks, begin, end);
  }
}

} // midiaud

#endif // SMF_READER_H_
