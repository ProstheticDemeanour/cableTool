#include "config.h"
#include "command_registry.hpp"
#include "cableLoader.hpp"
#include "distance_command.hpp"
#include <iostream>

int main(int argc, char* argv[]) {

  try {

    SoftwareConfig config = SoftwareConfig::from_file("inputs/config.json");
    CableDatabase db = CableDatabase::from_file("inputs/Cables.json");

    CommandRegistry registry;

    //register all commands
    //registry.registerCommand(std::make_unique<comandname>(&config));
    registry.registerCommand(std::make_unique<DistanceCommand>(&config, &db));

    if (argc < 2) {
            registry.showGlobalHelp();
            return 1;
        }

      // Look up the requested command
      std::string commandName = argv[1];
      BaseCommand* command = registry.getCommand(commandName);

      if (!command) {
          std::cerr << "Unknown command: " << commandName << "\n";
          registry.listCommands();
          return 1;
      }

      // Prepare arguments for the command (exclude program name and command name)
      std::vector<std::string> commandArgs;
      for (int i = 2; i < argc; ++i) {
          commandArgs.push_back(argv[i]);
      }

      // Execute the command
      return command->execute(commandArgs);

  } catch (const std::exception& e) {

      std::cerr << "Error: " << e.what() << "\n";
      return 1;
  }
}
