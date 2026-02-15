
#include "nlohmann/json.hpp"

using json = nlohmann::json; //no reason to do this, except I prefer it.

struct SoftwareConfig {

	std::filesystem::path input_directory;
	std::filesystem::path csv_directory;
	std::filesystem::path ledger_directory;
	std::filesystem::path output_directory;
	std::string input_bank;
	double income;
	size_t max_file_size = 1024 * 1024;
	bool enable_logging = true;
	std::string Assets_Folder;
	std::string Cash_Flow_Folder;
	std::string Export_Folder;
	std::string Import_Folder;
	std::string Income_Statement_Folder;

	static SoftwareConfig from_json(const json& j){

		SoftwareConfig config;

		 // Schema validation of must have directories for dataknot to work
        if (!j.contains("input_directory") || !j["input_directory"].is_string())
            throw std::runtime_error("Missing or invalid 'input_directory'");
        if (!j.contains("csv_directory") || !j["csv_directory"].is_string())
            throw std::runtime_error("Missing or invalid 'csv_directory'");
        if (!j.contains("ledger_directory") || !j["ledger_directory"].is_string())
            throw std::runtime_error("Missing or invalid 'ledger_directory'");
        if (!j.contains("output_directory") || !j["output_directory"].is_string())
            throw std::runtime_error("Missing or invalid 'output_directory'");

        //software dependant configurations that must exist for the program to function
        config.input_directory = j.at("input_directory").get<std::string>();
        config.csv_directory    = j.at("csv_directory").get<std::string>();
        config.ledger_directory = j.at("ledger_directory").get<std::string>();
        config.output_directory = j.at("output_directory").get<std::string>();

        //optional values, with defaults provided
        config.income					= j.value("default_income", 11928.83);
        config.max_file_size    		= j.value("max_file_size", 1024 * 1024);
        config.enable_logging   		= j.value("enable_logging", true);
        config.input_bank				= j.value("bank", "Bankwest");
        config.Assets_Folder			= j.value("Assets_Folder", "Assets");
        config.Cash_Flow_Folder 		= j.value("Cash_Flow_Folder", "CashFlows");
        config.Export_Folder			= j.value("Export_Folder", "Export");
        config.Import_Folder			= j.value("Import_Folder", "Import");
        config.Income_Statement_Folder	= j.value("Income_Statement_Folder", "IncomeStatements");


        return config;
	}

	static SoftwareConfig from_file(const std::string& filename) {
		std::ifstream file(filename);

		if(!file.is_open()) {

			throw std::runtime_error("could not open config file: " + filename);

		}


		json j;
		file >> j;
		return from_json(j);
	}


};
