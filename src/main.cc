
#include <ctime>
#include <string>
#include <iostream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <csignal>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "jack_midi_player.h"
#include "smf_streamer.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

std::unique_ptr<midiaud::JackMidiPlayer> midi_player;

void signal_handler(int signal) {
  std::cerr << "Received signal " << signal << std::endl;
  if (signal == SIGINT) {
    if (midi_player->activated()) {
      std::cerr << "Deactivating Jack client!" << std::endl;
      midi_player->RequestDeactivate(std::memory_order_relaxed);
    }
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
      ("destination-port,d", po::value<std::string>(),
       "Destination for MIDI output")
      ("watch,w", "watch input file for changes")
      ;

  po::options_description hidden_options_desc;
  hidden_options_desc.add_options()
      ("input-file", po::value<fs::path>()->required(),
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

  std::string client_name(vm["client"].as<std::string>());
  std::string port_name(vm["port"].as<std::string>());
  fs::path input_file(vm["input-file"].as<fs::path>());
  bool watch = (vm.count("watch") > 0);

  try {
    midi_player.reset(new midiaud::JackMidiPlayer(client_name, port_name));
    midi_player->EmplaceSmfStreamer(input_file.string());

    time_t last_load_time;
    constexpr int max_reload_retries = 5;
    int reload_retries = 0;
    std::time(&last_load_time);
    std::signal(SIGINT, &signal_handler);

    midi_player->Activate();

    // Only an activated client can have its ports connected.
    if (vm.count("destination-port") > 0) {
      std::string destination_port(vm["destination-port"].as<std::string>());
      midi_player->ConnectPort(destination_port);
    }

    while (midi_player->keep_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
      if (watch) {
        time_t last_modified = fs::last_write_time(input_file);
        if (std::difftime(last_load_time, last_modified) < 0) {
          if (reload_retries == 0)
            std::cerr << "Reloading " << input_file << std::endl;
          try {
            midi_player->EmplaceSmfStreamer(input_file.string());
            std::time(&last_load_time);
            reload_retries = 0;
          } catch (...) {
            // The reload of the MIDI file may have failed because the
            // application that produced it have not finished writing
            // yet (and there is no temporary file involved). Retry a
            // couple times before giving up.
            if (++reload_retries >= max_reload_retries) throw;
          }
        }
      }
    }

    midi_player->Deactivate();

    return 0;
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return -1;
  }
}
