#include "Record.h"
#include "manager/parse.h"
#include "ui/output.h"

#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

Record::Record() {
    // 暂定为空
}

void Record::insert_record(const std::string& table_name, const std::string& cols, const std::string& vals) {
    this->table_name = table_name;
    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }
    if (!cols.empty()) {
        // 带列名的情况
        // 解析列名和值
        parse_columns(cols);
        parse_values(vals);

        // 验证列名是否存在于表中
        validate_columns();

        // 验证值的类型是否匹配
        validate_types();
    }
    else if (cols.empty()) {
        // 从.tdf文件中读取字段信息
        read_table_structure();

        // 解析值
        parse_values(vals);

        // 检查值的数量是否与表中字段数量匹配
        if (values.size() != table_structure.size()) {
            throw std::runtime_error("Value count (" + std::to_string(values.size()) +
                ") does not match column count (" +
                std::to_string(table_structure.size()) + ").");
        }

        // 验证值的类型是否匹配
        validate_types_without_columns();
    }
    else {
        throw std::invalid_argument("Invalid SQL insert statement.");
    }
    insert_into();
}

// insert_record 全局接口实现
void insert_record(const std::string& table_name, const std::string& cols, const std::string& vals) {
    Record r;
    r.insert_record(table_name, cols, vals);
}


bool Record::table_exists(const std::string& table_name) {
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);
    return file.good();
}

void Record::read_table_structure() {
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    table_structure.clear(); // 确保清空之前的结构信息
    columns.clear(); // 清空列名

    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name, column_type;

        if (ss >> column_name >> column_type) {
            // 将列名和类型添加到表结构中
            table_structure[column_name] = column_type;
            // 同时保持原始的列顺序
            columns.push_back(column_name);
        }
    }
}

void Record::parse_columns(const std::string& cols) {
    columns.clear();
    std::stringstream ss(cols);
    std::string column;

    while (std::getline(ss, column, ',')) {
        // 去除前后空格
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        columns.push_back(column);
    }
}

void Record::parse_values(const std::string& vals) {
    values.clear();
    std::stringstream ss(vals);
    std::string value;
    bool in_quotes = false;
    std::string current_value;

    for (char c : vals) {
        if (c == '\'' || c == '\"') {
            in_quotes = !in_quotes;
            current_value += c;
        }
        else if (c == ',' && !in_quotes) {
            // 完成一个值的解析
            current_value.erase(0, current_value.find_first_not_of(" \t"));
            current_value.erase(current_value.find_last_not_of(" \t") + 1);
            values.push_back(current_value);
            current_value.clear();
        }
        else {
            current_value += c;
        }
    }

    // 添加最后一个值
    if (!current_value.empty()) {
        current_value.erase(0, current_value.find_first_not_of(" \t"));
        current_value.erase(current_value.find_last_not_of(" \t") + 1);
        values.push_back(current_value);
    }
}

void Record::validate_columns() {
    // 首先从.tdf文件读取表结构
    if (table_structure.empty()) {
        read_table_structure();
    }

    // 验证所有提供的列名是否在表结构中
    for (const auto& column : columns) {
        if (table_structure.find(column) == table_structure.end()) {
            throw std::runtime_error("字段'" + column + "' 并不存在于表'" + table_name + "'中。");
        }
    }
}

bool Record::is_valid_type(const std::string& value, const std::string& type) {
    if (type == "INT") {
        // 检查是否为整数
        try {
            std::stoi(value);
            return true;
        }
        catch (...) {
            return false;
        }
    }
    else if (type == "FLOAT" || type == "DOUBLE") {
        // 检查是否为浮点数
        try {
            std::stod(value);
            return true;
        }
        catch (...) {
            return false;
        }
    }
    else if (type == "VARCHAR" || type == "CHAR" || type == "TEXT") {
        // 检查字符串是否被引号括起来
        return (value.front() == '\'' && value.back() == '\'') ||
            (value.front() == '\"' && value.back() == '\"');
    }
    else if (type == "DATE" || type == "DATETIME") {
        // 简单检查日期格式，待拓展
        std::regex date_regex(R"('(\d{4}-\d{2}-\d{2})')");
        return std::regex_match(value, date_regex);
    }

    return true;
}

void Record::validate_types() {
    // 验证每个值的类型是否与对应的列类型匹配
    for (size_t i = 0; i < columns.size(); ++i) {
        std::string column = columns[i];
        std::string value = values[i];
        std::string expected_type = table_structure[column];

        if (!is_valid_type(value, expected_type)) {
            throw std::runtime_error("Type mismatch for column '" + column +
                "'. Expected " + expected_type +
                ", got value: " + value);
        }
    }
}

void Record::validate_types_without_columns() {
    // 按表结构中的列顺序验证值的类型
    size_t i = 0;
    for (const auto& column : columns) {
        if (i < values.size()) {  // 确保不会越界
            std::string expected_type = table_structure[column];
            std::string value = values[i];

            if (!is_valid_type(value, expected_type)) {
                throw std::runtime_error("Type mismatch for column '" + column +
                    "'. Expected " + expected_type +
                    ", got value: " + value);
            }
            ++i;
        }
    }
}

void Record::insert_into() {
    std::string file_name = this->table_name + ".trd";
    std::ofstream file(file_name, std::ios::app);

    if (!file) {
        throw std::runtime_error("打开文件" + file_name + "失败。");
    }

    // 使用新的格式：字段.值,字段.值,...,END
    for (size_t i = 0; i < columns.size() && i < values.size(); ++i) {
        file << columns[i] << "." << values[i];
        if (i + 1 < columns.size() && i + 1 < values.size()) {
            file << ",";
        }
    }
    file << ",END" << std::endl;  // 使用END标记结束一条记录

    file.close();
    std::cout << "记录插入表 " << this->table_name << " 成功。" << std::endl;
}



// 静态版本的表结构读取函数，用于select操作
std::unordered_map<std::string, std::string> Record::read_table_structure_static(const std::string& table_name) {
    std::unordered_map<std::string, std::string> result;
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name, column_type;

        if (ss >> column_name >> column_type) {
            result[column_name] = column_type;
        }
    }

    return result;
}

// 获取列的顺序
std::vector<std::string> Record::get_column_order(const std::string& table_name) {
    std::vector<std::string> result;
    std::string tdf_filename = table_name + ".tdf";
    std::ifstream file(tdf_filename);

    if (!file) {
        throw std::runtime_error("Failed to open table definition file: " + tdf_filename);
    }

    std::string line;
    // 假设.tdf文件格式为: column_name column_type
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string column_name;

        if (ss >> column_name) {
            result.push_back(column_name);
        }
    }

    return result;
}

// 解析列名列表
std::vector<std::string> Record::parse_column_list(const std::string& columns) {
    std::vector<std::string> result;
    std::stringstream ss(columns);
    std::string column;

    while (std::getline(ss, column, ',')) {
        // 去除前后空格
        column.erase(0, column.find_first_not_of(" \t"));
        column.erase(column.find_last_not_of(" \t") + 1);
        result.push_back(column);
    }

    return result;
}

// 设置表名
void Record::set_table_name(const std::string& name) {
    this->table_name = name;
}

// 添加列
void Record::add_column(const std::string& column) {
    columns.push_back(column);
}

// 添加值
void Record::add_value(const std::string& value) {
    values.push_back(value);
}

// 获取表名
std::string Record::get_table_name() const {
    return table_name;
}

// 获取所有列
const std::vector<std::string>& Record::get_columns() const {
    return columns;
}

// 获取所有值
const std::vector<std::string>& Record::get_values() const {
    return values;
}

void Record::update(const std::string& tableName, const std::string& setClause, const std::string& condition) {
    this->table_name = tableName;

    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    // 读取表结构
    read_table_structure();

    // 解析SET子句
    std::unordered_map<std::string, std::string> update_values;
    std::istringstream set_stream(setClause);
    std::string pair;

    while (std::getline(set_stream, pair, ',')) {
        // 去除前后空格
        pair.erase(0, pair.find_first_not_of(" \t"));
        pair.erase(pair.find_last_not_of(" \t") + 1);

        // 查找等号位置
        size_t eq_pos = pair.find('=');
        if (eq_pos == std::string::npos) {
            throw std::runtime_error("无效的SET子句格式: " + pair);
        }

        // 提取字段名和新值
        std::string field = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);

        // 去除字段和值的前后空格
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 检查字段是否存在
        if (table_structure.find(field) == table_structure.end()) {
            throw std::runtime_error("字段 '" + field + "' 不存在于表 '" + this->table_name + "' 中。");
        }

        // 验证值的类型
        if (!is_valid_type(value, table_structure[field])) {
            throw std::runtime_error("值 '" + value + "' 的类型与字段 '" + field + "' 的类型不匹配。");
        }

        update_values[field] = value;
    }

    // 如果没有要更新的字段，则退出
    if (update_values.empty()) {
        throw std::runtime_error("没有提供要更新的字段。");
    }

    // 解析条件
    if (!condition.empty()) {
        parse_condition(condition);
    }

    // 执行更新操作
    // 1. 读取现有记录
    std::string trd_filename = this->table_name + ".trd";
    std::string temp_filename = this->table_name + ".tmp";
    std::ifstream infile(trd_filename);
    std::ofstream outfile(temp_filename);

    if (!infile) {
        throw std::runtime_error("无法打开表数据文件: " + trd_filename);
    }

    if (!outfile) {
        throw std::runtime_error("无法创建临时文件: " + temp_filename);
    }

    std::string line;
    int updated_count = 0;

    // 读取并处理每一行记录
    while (std::getline(infile, line)) {
        // 解析记录行
        std::unordered_map<std::string, std::string> record_data;
        std::string field_value_pair;
        std::stringstream ss(line);
        bool is_valid_record = true;

        // 处理每个字段值对
        while (std::getline(ss, field_value_pair, ',')) {
            // 如果遇到END标记，结束当前记录的解析
            if (field_value_pair == "END") {
                break;
            }

            // 分割字段名和值
            size_t dot_pos = field_value_pair.find('.');
            if (dot_pos != std::string::npos) {
                std::string field = field_value_pair.substr(0, dot_pos);
                std::string value = field_value_pair.substr(dot_pos + 1);

                // 存储字段和值
                record_data[field] = value;
            }
            else {
                is_valid_record = false;
                break;
            }
        }

        if (!is_valid_record) {
            // 如果记录格式不正确，保持原样写入
            outfile << line << std::endl;
            continue;
        }

        // 检查记录是否满足条件
        if (condition.empty() || matches_condition(record_data)) {
            // 更新满足条件的记录
            for (const auto& [field, new_value] : update_values) {
                record_data[field] = new_value;
            }
            updated_count++;
        }

        // 将更新后的记录写回文件
        bool first = true;
        for (const auto& [field, value] : record_data) {
            if (!first) outfile << ",";
            outfile << field << "." << value;
            first = false;
        }
        outfile << ",END" << std::endl;
    }

    infile.close();
    outfile.close();

    // 替换原文件
    if (std::remove(trd_filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        throw std::runtime_error("无法删除原表数据文件。");
    }

    if (std::rename(temp_filename.c_str(), trd_filename.c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件。");
    }

    std::cout << "成功更新了 " << updated_count << " 条记录。" << std::endl;
}

void Record::delete_(const std::string& tableName, const std::string& condition) {
    this->table_name = tableName;

    // 检查表是否存在
    if (!table_exists(this->table_name)) {
        throw std::runtime_error("表 '" + this->table_name + "' 不存在。");
    }

    // 读取表结构
    read_table_structure();

    // 解析条件
    if (!condition.empty()) {
        try {
            parse_condition(condition);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("条件解析错误: " + std::string(e.what()));
        }
    }

    // 执行删除操作
    // 1. 读取现有记录
    std::string trd_filename = this->table_name + ".trd";
    std::string temp_filename = this->table_name + ".tmp";
    std::ifstream infile(trd_filename);
    std::ofstream outfile(temp_filename);

    if (!infile) {
        throw std::runtime_error("无法打开表数据文件: " + trd_filename);
    }

    if (!outfile) {
        throw std::runtime_error("无法创建临时文件: " + temp_filename);
    }

    std::string line;
    int deleted_count = 0;

    // 读取并处理每一行记录
    while (std::getline(infile, line)) {
        // 解析记录行
        std::unordered_map<std::string, std::string> record_data;
        std::string field_value_pair;
        std::stringstream ss(line);
        bool is_valid_record = true;

        // 处理每个字段值对
        while (std::getline(ss, field_value_pair, ',')) {
            // 如果遇到END标记，结束当前记录的解析
            if (field_value_pair == "END") {
                break;
            }

            // 分割字段名和值
            size_t dot_pos = field_value_pair.find('.');
            if (dot_pos != std::string::npos) {
                std::string field = field_value_pair.substr(0, dot_pos);
                std::string value = field_value_pair.substr(dot_pos + 1);

                // 存储字段和值
                record_data[field] = value;
            }
            else {
                is_valid_record = false;
                break;
            }
        }

        if (!is_valid_record) {
            // 如果记录格式不正确，保持原样写入
            outfile << line << std::endl;
            continue;
        }

        // 检查记录是否满足条件
        if (!condition.empty() && !matches_condition(record_data)) {
            // 不满足删除条件，保留记录
            outfile << line << std::endl;
        }
        else {
            // 满足删除条件，不写入（即删除）
            deleted_count++;
        }
    }

    infile.close();
    outfile.close();

    // 替换原文件
    if (std::remove(trd_filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        throw std::runtime_error("无法删除原表数据文件。");
    }

    if (std::rename(temp_filename.c_str(), trd_filename.c_str()) != 0) {
        throw std::runtime_error("无法重命名临时文件。");
    }

    std::cout << "成功删除了 " << deleted_count << " 条记录。" << std::endl;
}
// 实现条件解析方法
void Record::parse_condition(const std::string& condition) {
    // 清除之前的条件数据
    condition_field = "";
    condition_operator = "";
    condition_value = "";

    // 如果条件为空则跳过
    if (condition.empty()) {
        return;
    }

    // 用于匹配不同比较运算符的正则表达式
    std::regex eq_regex("\\s*(\\w+)\\s*=\\s*([^\\s]+)\\s*");   // 字段 = 值
    std::regex neq_regex("\\s*(\\w+)\\s*!=\\s*([^\\s]+)\\s*"); // 字段 != 值
    std::regex gt_regex("\\s*(\\w+)\\s*>\\s*([^\\s]+)\\s*");   // 字段 > 值
    std::regex lt_regex("\\s*(\\w+)\\s*<\\s*([^\\s]+)\\s*");   // 字段 < 值
    std::regex gte_regex("\\s*(\\w+)\\s*>=\\s*([^\\s]+)\\s*"); // 字段 >= 值
    std::regex lte_regex("\\s*(\\w+)\\s*<=\\s*([^\\s]+)\\s*"); // 字段 <= 值

    std::smatch matches;

    // 尝试匹配每种操作符模式
    if (std::regex_match(condition, matches, eq_regex)) {
        condition_field = matches[1];
        condition_operator = "=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, neq_regex)) {
        condition_field = matches[1];
        condition_operator = "!=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, gt_regex)) {
        condition_field = matches[1];
        condition_operator = ">";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, lt_regex)) {
        condition_field = matches[1];
        condition_operator = "<";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, gte_regex)) {
        condition_field = matches[1];
        condition_operator = ">=";
        condition_value = matches[2];
    }
    else if (std::regex_match(condition, matches, lte_regex)) {
        condition_field = matches[1];
        condition_operator = "<=";
        condition_value = matches[2];
    }
    else {
        throw std::runtime_error("无效的条件格式：" + condition);
    }

    // 验证字段是否存在于表中
    if (!table_structure.empty() && table_structure.find(condition_field) == table_structure.end()) {
        throw std::runtime_error("字段 '" + condition_field + "' 不存在于表 '" + table_name + "' 中。");
    }
}

// 实现条件匹配判断方法
bool Record::matches_condition(const std::unordered_map<std::string, std::string>& record_data) const {
    // 如果没有条件，则所有记录都匹配
    if (condition_field.empty() || condition_operator.empty()) {
        return true;
    }

    // 检查记录中是否包含条件字段
    if (record_data.find(condition_field) == record_data.end()) {
        return false; // 字段不存在，不匹配
    }

    // 获取记录中对应字段的值
    std::string field_value = record_data.at(condition_field);

    // 根据字段类型进行比较
    std::string field_type = table_structure.at(condition_field);

    // 数值型比较
    if (field_type == "INT" || field_type == "FLOAT" || field_type == "DOUBLE") {
        try {
            // 将字符串转换为数值进行比较
            double record_val = std::stod(field_value);
            double condition_val = std::stod(condition_value);

            if (condition_operator == "=") return record_val == condition_val;
            if (condition_operator == "!=") return record_val != condition_val;
            if (condition_operator == ">") return record_val > condition_val;
            if (condition_operator == "<") return record_val < condition_val;
            if (condition_operator == ">=") return record_val >= condition_val;
            if (condition_operator == "<=") return record_val <= condition_val;
        }
        catch (const std::exception& e) {
            // 转换失败，按字符串比较
            std::cerr << "数值转换失败，按字符串比较: " << e.what() << std::endl;
        }
    }

    // 字符串比较（或数值转换失败的情况）
    if (field_type == "VARCHAR" || field_type == "CHAR" || field_type == "TEXT" ||
        condition_operator == "=" || condition_operator == "!=") {

        // 处理引号
        std::string clean_field_value = field_value;
        std::string clean_condition_value = condition_value;

        // 去除可能存在的引号
        auto remove_quotes = [](std::string& s) {
            if ((s.front() == '\'' && s.back() == '\'') ||
                (s.front() == '\"' && s.back() == '\"')) {
                s = s.substr(1, s.length() - 2);
            }
            };

        remove_quotes(clean_field_value);
        remove_quotes(clean_condition_value);

        if (condition_operator == "=") return clean_field_value == clean_condition_value;
        if (condition_operator == "!=") return clean_field_value != clean_condition_value;
        if (condition_operator == ">") return clean_field_value > clean_condition_value;
        if (condition_operator == "<") return clean_field_value < clean_condition_value;
        if (condition_operator == ">=") return clean_field_value >= clean_condition_value;
        if (condition_operator == "<=") return clean_field_value <= clean_condition_value;
    }

    // 日期比较 (简单实现，可能需要更复杂的日期解析)
    if (field_type == "DATE" || field_type == "DATETIME") {
        // 去除引号后直接比较字符串（因为ISO格式日期字符串可以直接比较）
        std::string clean_field_value = field_value;
        std::string clean_condition_value = condition_value;

        auto remove_quotes = [](std::string& s) {
            if ((s.front() == '\'' && s.back() == '\'') ||
                (s.front() == '\"' && s.back() == '\"')) {
                s = s.substr(1, s.length() - 2);
            }
            };

        remove_quotes(clean_field_value);
        remove_quotes(clean_condition_value);

        if (condition_operator == "=") return clean_field_value == clean_condition_value;
        if (condition_operator == "!=") return clean_field_value != clean_condition_value;
        if (condition_operator == ">") return clean_field_value > clean_condition_value;
        if (condition_operator == "<") return clean_field_value < clean_condition_value;
        if (condition_operator == ">=") return clean_field_value >= clean_condition_value;
        if (condition_operator == "<=") return clean_field_value <= clean_condition_value;
    }

    // 默认返回false
    return false;
}

// 实现带条件的select方法
std::vector<Record> Record::select(const std::string& columns, const std::string& table_name, const std::string& condition) {
    std::vector<Record> results;

    // 检查表是否存在
    if (!table_exists(table_name)) {
        throw std::runtime_error("表 '" + table_name + "' 不存在。");
    }

    // 读取表结构以获取所有字段信息
    std::unordered_map<std::string, std::string> table_structure = read_table_structure_static(table_name);
    std::vector<std::string> all_columns = get_column_order(table_name);

    // 确定要查询的列
    std::vector<std::string> selected_columns;
    if (columns == "*") {
        // 查询所有列
        selected_columns = all_columns;
    }
    else {
        // 解析指定的列
        selected_columns = parse_column_list(columns);

        // 验证指定的列是否存在于表中
        for (const auto& col : selected_columns) {
            if (table_structure.find(col) == table_structure.end()) {
                throw std::runtime_error("字段 '" + col + "' 不存在于表 '" + table_name + "' 中。");
            }
        }
    }

    // 创建一个临时Record对象用于解析条件
    Record temp_record;
    temp_record.set_table_name(table_name);
    temp_record.table_structure = table_structure;

    // 解析条件
    if (!condition.empty()) {
        temp_record.parse_condition(condition);
    }

    // 读取记录文件
    std::string trd_filename = table_name + ".trd";
    std::ifstream file(trd_filename);

    if (!file) {
        // 文件不存在或不可读，返回空结果
        return results;
    }

    std::string line;
    // 读取每一行记录
    while (std::getline(file, line)) {
        // 解析记录行，格式：字段1.值1,字段2.值2,...,END
        std::unordered_map<std::string, std::string> record_data;
        std::string field_value_pair;
        std::stringstream ss(line);

        // 处理每个字段值对
        while (std::getline(ss, field_value_pair, ',')) {
            // 如果遇到END标记，结束当前记录的解析
            if (field_value_pair == "END") {
                break;
            }

            // 分割字段名和值
            size_t dot_pos = field_value_pair.find('.');
            if (dot_pos != std::string::npos) {
                std::string field = field_value_pair.substr(0, dot_pos);
                std::string value = field_value_pair.substr(dot_pos + 1);

                // 存储字段和值
                record_data[field] = value;
            }
        }

        // 检查记录是否满足条件
        if (!condition.empty() && !temp_record.matches_condition(record_data)) {
            continue; // 不满足条件，跳过此记录
        }

        // 创建一个新的记录对象
        Record record;
        record.set_table_name(table_name);

        // 添加选中的列和对应的值
        for (const auto& col : selected_columns) {
            record.add_column(col);
            // 如果记录中有该列，则添加值，否则添加NULL或空值
            if (record_data.find(col) != record_data.end()) {
                record.add_value(record_data[col]);
            }
            else {
                record.add_value("NULL");  // 或者其他表示缺失值的标记
            }
        }

        // 将记录添加到结果集
        results.push_back(record);
    }

    return results;
}