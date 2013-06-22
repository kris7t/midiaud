
#include <string>
#include <iostream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <csignal>

#include <boost/program_options.hpp>

#include "jack_midi_player.h"
#include "smf_streamer.h"

namespace po = boost::program_options;

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

  std::string client_name(vm["client"].as<std::string>());
  std::string port_name(vm["port"].as<std::string>());
  std::string input_file(vm["input-file"].as<std::string>());

  try {
    midi_player.reset(new midiaud::JackMidiPlayer(client_name, port_name));
    midi_player->EmplaceSmfStreamer(input_file);
    std::signal(SIGINT, &signal_handler);

    midi_player->Activate();
    while (midi_player->keep_running()) {
      // TODO Reload when file on disk changes.
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    midi_player->Deactivate();

    return 0;
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return -1;
  }
}
