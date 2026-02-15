#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include "nlohmann/json.hpp"

struct CableDatabase {

  json data;

  static CableDatabase from_file(const std::string& filename) {

    std::ifstream file(filename);
    if (!file.is_open()) {

      throw std::runtime_error(("Could not open cable data: " + filename));

    }

    CableDatabase db;

    file >> db.data;
    return db;


  }

  const json& get_cable(const std::string& size) const {
    if (!data.contains(size)) {

      throw std::runtime_error("Cable sinze not found:" + size);

    }

    return data[size];
  }

  double get_property(const std::string& size, const std::string& property) const{

    auto& cable = get_cable(size);

    if (cable[property].is_null()) {
      throw std::runtime_error("Property unavailable: " + property);
    }

    return cable[property].get<double>();
  }
};
