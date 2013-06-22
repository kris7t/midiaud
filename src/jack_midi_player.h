#ifndef JACK_MIDI_PLAYER_H_
#define JACK_MIDI_PLAYER_H_

#include <string>
#include <atomic>
#include <exception>
#include <type_traits>

#include <jack/jack.h>

#include "smf_streamer.h"
#include "lockfree_resource.h"
#include "lockfree_resource-inl.h"

namespace midiaud {

class JackMidiPlayer {
 public:
  JackMidiPlayer(std::string client_name, std::string port_name);
  JackMidiPlayer(const JackMidiPlayer &) = delete;
  JackMidiPlayer(JackMidiPlayer &&) = delete;
  ~JackMidiPlayer();
  JackMidiPlayer &operator=(const JackMidiPlayer &) = delete;

  void Activate();
  /**
   * Deactivate client then rethrown the first pending exception from
   * the main thread (if any).
   */
  void Deactivate();
  void RequestDeactivate(std::memory_order order =
                         std::memory_order_release) noexcept;

  /**
   * Emplaces a new SmfStreamer into the resource container.
   *
   * The resource will be eventually used by the RT thread, and the
   * old resource will eventually be destructed.
   */
  template <typename... Args> void EmplaceSmfStreamer(Args &&... args) {
    smf_streamer_container_.Emplace(std::forward<Args>(args)...);
  }

  const std::string &client_name() { return client_name_; }
  const std::string &port_name() { return port_name_; }
  bool activated() { return activated_; }
  /**
   * Check in main thread whether the client wants to remain active.
   */
  bool keep_running() {
    return keep_running_.load(std::memory_order_acquire);
  }

 protected:
  int SyncCallback(jack_transport_state_t , jack_position_t *pos);
  int ProcessCallback(jack_nframes_t nframes);
  void ShutdownCallback();

 private:
  static int StaticSyncCallback(jack_transport_state_t state,
                                jack_position_t *pos,
                                void *arg) noexcept;
  static int StaticProcessCallback(jack_nframes_t nframes,
                                   void *arg) noexcept;
  static void StaticShutdownCallback(void *arg) noexcept;

  /**
   * Runs the functor and propagates any exception to the main thread.
   *
   * The functor will not be run if there is pending exception from
   * the RT thread. If the functor throws, the exception is saved and
   * deactivation is requested. Upon deactivation, the exception will
   * be rethrown.
   *
   * @tparam Functor the type of the functor to call.
   * @param functor the functor to call.
   * @param on_error the value to return on error.
   * @returns the return value of the functor, or on_error if there was
   *          an exception.
   */
  template <typename Functor>
  auto RunAndPropagateException(const Functor &functor,
                                decltype(functor()) on_error = decltype(functor())())
      noexcept -> decltype(functor()) {
    if (pending_exception_) return on_error;
    try {
      return functor();
    } catch (...) {
      pending_exception_ = std::current_exception();
      RequestDeactivate();
      return on_error;
    }
  }

  /**
   * RunAndPropagateException without value to return on error.
   */
  template <typename Functor>
  auto RunAndPropagateException(const Functor &functor) noexcept
      -> typename std::enable_if<std::is_void<decltype(functor())>::value, void>::type {
    if (pending_exception_) return;
    try {
      functor();
    } catch (...) {
      pending_exception_ = std::current_exception();
      RequestDeactivate();
    }
  }

  std::string client_name_; // For main thread.
  std::string port_name_; // For main thread.
  bool activated_; // For main thread!
  /**
   * Signals to the main loop that the Jack client in RT thread wants
   * to be deactivated.
   *
   * When JackMidiPlayer is constructed, this flag is set. It will be
   * set again every time keep_running() is called.
   */
  std::atomic<bool> keep_running_;
  /**
   * Carries exceptions from the RT thread to be rethrown in the main
   * thread when Deactivate() is called.
   */
  std::exception_ptr pending_exception_;
  jack_client_t *jack_client_; // For RT thread (initialized in main thread).
  jack_port_t *midi_port_; // For RT thread (initialized in main thread).
  LockfreeResource<SmfStreamer> smf_streamer_container_;
};

}

#endif // JACK_MIDI_PLAYER_H_
