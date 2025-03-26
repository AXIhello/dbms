#include "Record.h"
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

Record::Record(const std::string& record) {
    parse_record(record);
}

void Record::parse_record(const std::string& record) {
    std::regex insert_regex(R"(insert\s+into\s+(\w+)\s*\((.*?)\)\s*values\s*\((.*?)\))", std::regex::icase);
    std::smatch match;

    if (std::regex_search(record, match, insert_regex)) {
        this->table_name = match[1].str();
        parse_columns(match[2].str());
        parse_values(match[3].str());
    }
    else {
        throw std::invalid_argument("Invalid SQL insert statement.");
    }
}

void Record::parse_columns(const std::string& cols) {
    std::stringstream ss(cols);
    std::string column;
    while (std::getline(ss, column, ',')) {
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        columns.push_back(column);
    }
}

void Record::parse_values(const std::string& vals) {
    std::stringstream ss(vals);
    std::string value;
    while (std::getline(ss, value, ',')) {
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        values.push_back(value);
    }
}

void Record::insert_into(const std::string& db_name) {
    if (columns.size() != values.size()) {
        throw std::runtime_error("Column count does not match value count.");
    }

    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app);
    if (!file) {
        throw std::runtime_error("Failed to open table record file.");
    }

    file << "INSERT INTO " << this->table_name << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        file << columns[i] << (i + 1 < columns.size() ? ", " : "");
    }
    file << ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        file << values[i] << (i + 1 < values.size() ? ", " : "");
    }
    file << ");" << std::endl;

    file.close();
    std::cout << "Record inserted into " << this->table_name << " successfully." << std::endl;
}
