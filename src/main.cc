
#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include <boost/program_options.hpp>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <smf.h>

namespace po = boost::program_options;

jack_client_t *jack_client{nullptr};

std::atomic_bool running{false};

std::exception_ptr pending_exception;

jack_port_t *midi_port;

bool repositioned;

smf_t *smf;

double jack_to_secs(jack_position_t *pos) {
  return static_cast<double>(pos->frame) / pos->frame_rate;
}

void my_seek_to_seconds(double start_secs) {
  for (;;) {
    smf_event_t *next_event = smf_peek_next_event(smf);
    if (next_event == nullptr || next_event->time_seconds >= start_secs)
      break;
    smf_skip_next_event(smf);
  }    
}

int sync_callback(jack_transport_state_t state, jack_position_t *pos, void *) {
  try {
    if (state == JackTransportStarting) {
      double secs = jack_to_secs(pos);
      smf_rewind(smf);
      my_seek_to_seconds(secs);
      repositioned = true;
    }
  } catch (...) {
    pending_exception = std::current_exception();
    running.store(false);
  }

  return true;
}

int process_callback(jack_nframes_t nframes, void *) {
  try {
    static jack_transport_state_t last_state = JackTransportStopped;

    void *buffer = jack_port_get_buffer(midi_port, nframes);
    jack_midi_clear_buffer(buffer);

    jack_position_t pos;
    jack_transport_state_t state = jack_transport_query(jack_client, &pos);

    if (repositioned ||
        (state == JackTransportStopped && last_state != JackTransportStopped)) {
      // Send all sounds OFF message to all channels.
      for (unsigned char i = 0xb0; i <= 0xbf; ++i) {
        jack_midi_data_t message[] = { i, 0x78, 0x00 };
        if (jack_midi_event_write(buffer, 0, message, sizeof(message)) != 0)
          throw std::runtime_error("jack_midi_event_write failed");
      }
    }

    // TODO After repositioning, send controller values and program numbers.

    if (state == JackTransportRolling) {
      double start_secs = jack_to_secs(&pos);
      double delta_secs = static_cast<double>(nframes) / pos.frame_rate;
      double end_secs = start_secs + delta_secs;

      my_seek_to_seconds(start_secs);
      for (;;) {
        smf_event_t *next_event = smf_peek_next_event(smf);
        if (next_event == nullptr || next_event->time_seconds >= end_secs)
          break;
        double offset_secs = next_event->time_seconds - start_secs;
        jack_nframes_t offset_frames =
            static_cast<jack_nframes_t>(offset_secs * pos.frame_rate);
        if (jack_midi_event_write(buffer, offset_frames, next_event->midi_buffer,
                                  next_event->midi_buffer_length) != 0)
          throw std::runtime_error("jack_midi_event_write failed");
        smf_skip_next_event(smf);
      }
    }

    repositioned = false;
    last_state = state;

    return 0;
  } catch (...) {
    pending_exception = std::current_exception();
    running.store(false);
    return -1;
  }
}

void shutdown_callback(void *) {
  running.store(false);
}

void signal_handler(int signal) {
  std::cerr << "Received signal " << signal << std::endl;
  if (signal == SIGINT) {
    std::cerr << "Exiting!" << std::endl;
    running.store(false);
  }
}

void print_usage(char *argv0) {
  std::cout << "Usage: " << argv0 << " [options] input-file\n";
}

int main(int argc, char *argv[]) {
  po::options_description generic_options_desc{"Allowed options"};
  generic_options_desc.add_options()
      ("version,v", "produce version information")
      ("help", "produce help message")
      ("client,c", po::value<std::string>()->default_value("midiaud"),
       "Jack client name")
      ("port,p", po::value<std::string>()->default_value("midi_out"),
       "Jack port name")
      ;

  po::options_description hidden_options_desc;
  hidden_options_desc.add_options()
      ("input-file", po::value<std::string>()->required(),
       "input file")
      ;

  po::options_description options_desc;
  options_desc.add(generic_options_desc).add(hidden_options_desc);

  po::positional_options_description positional_options_desc;
  positional_options_desc.add("input-file", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv)
              .options(options_desc)
              .positional(positional_options_desc)
              .run(), vm);
    // Check for --help and --version before raising exception about
    // required options.
    if (vm.count("version") > 0) {
      std::cerr << "midiaud version 0\n";
      return 0;
    }
    if (vm.count("help") > 0) {
      print_usage(argv[0]);
      std::cerr << generic_options_desc << "\n";
      return 0;
    }
    // Now we can raise pending exceptions.
    po::notify(vm);
  } catch (std::exception &e) {
    // Command-line options are probably malformed, better print usage
    // as well as exceptions message.
    std::cerr << e.what() << "\n\n";
    print_usage(argv[0]);
    std::cerr << generic_options_desc << "\n";
    return -1;
  }

  smf = smf_load(vm["input-file"].as<std::string>().c_str());
  if (smf == nullptr) {
    std::cerr << "smf_load failed\n";
    return -1;
  }

  jack_client = jack_client_open(vm["client"].as<std::string>().c_str(),
                                 JackNullOption, nullptr);
  if (jack_client == nullptr) {
    std::cerr << "jack_client_open failed\n";
    return -1;
  }

  try {
    if (jack_set_sync_callback(jack_client, &sync_callback, nullptr) != 0)
      throw std::runtime_error("jack_set_sync_callback failed");
    if (jack_set_process_callback(jack_client, &process_callback, nullptr) != 0)
      throw std::runtime_error("jack_set_process_callback failed");
    jack_on_shutdown(jack_client, &shutdown_callback, nullptr);

    midi_port = jack_port_register(jack_client,
                                   vm["port"].as<std::string>().c_str(),
                                   JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput,
                                   0);
    if (midi_port == nullptr)
      throw std::runtime_error("jack_port_register failed");

    std::signal(SIGINT, &signal_handler);
    running.store(true);
    if (jack_activate(jack_client) != 0)
      throw std::runtime_error("jack_activate failed");

    while (running.load()) {
      // TODO Reload when file on disk changes.
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    if (pending_exception)
      std::rethrow_exception(pending_exception);
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    jack_deactivate(jack_client);
    jack_client_close(jack_client);
    return -1;
  }

  jack_deactivate(jack_client);
  jack_client_close(jack_client);
  return 0;
}
