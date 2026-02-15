#ifndef DISTANCE_COMMAND_HPP
#define DISTANCE_COMMAND_HPP

#include "base_command.hpp"
#include "nlohmann/json.hpp"
#include "ParsedArgs.hpp" 
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <complex>
#include <cmath> // For mathematical constants like M_PI

using json = nlohmann::json;

class DistanceCommand : public BaseCommand {

  private:


  public:

    DistanceCommand(const SoftwareConfig* cfg, const CableDatabase* db)
        : BaseCommand(cfg, db) {}

    std::string getName() const override;
    std::string getDescription() const override;
    int execute(const std::vector<std::string>& args) override;
    void showHelp() const override;

}


#endif
