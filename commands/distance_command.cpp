#include "distance_command.hpp"

std::string DistanceCommand::getName() const {
    return "distance";
}

std::string DistanceCommand::getDescription() const {
  return "Calculate maximum cable distance for achievable given power loss and voltage drop limits"
}

void DistanceCommand::showHelp() const {
    std::cout << "Usage: distance <cable_size> <length_km> <current_A>\n"
              << "Calculates voltage drop and checks against limits\n";
}

