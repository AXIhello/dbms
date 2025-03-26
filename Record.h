#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>

class Record {
public:
    explicit Record(const std::string& record);
    void insert_into(const std::string& db_name);

private:
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::string> values;

    void parse_record(const std::string& record);
    void parse_columns(const std::string& cols);
    void parse_values(const std::string& vals);
};

#endif // RECORD_H