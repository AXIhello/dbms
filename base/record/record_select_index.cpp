#include "Record.h"
#include "parse/parse.h"
#include "ui/output.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>
#include <set>
#include <utility>

#include <memory>
#include <string>
#include <stdexcept>
#include <cstring>

std::unordered_map<uint64_t, std::unordered_map<std::string, std::string>>
vectorToMap(const std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>& vec) {
    std::unordered_map<uint64_t, std::unordered_map<std::string, std::string>> result;
    for (const auto& [row_id, record] : vec) {
        result[row_id] = record;
    }
    return result;
}

std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>>
Record::selectByIndex(
    const std::unordered_map<uint64_t, std::unordered_map<std::string, std::string>>& filtered,
    const std::vector<std::shared_ptr<Table>>& tables,
    const std::unordered_map<std::string, std::string>& combined_structure,
    bool has_join
) {
    this->table_structure = combined_structure;
    if (!full_condition.empty()) this->parse_condition(full_condition);

    std::set<uint64_t> candidate_ids;
    bool used_index = false;

    std::regex logic_split(R"(\s+(AND|OR)\s+)", std::regex::icase);
    std::sregex_token_iterator token_iter(full_condition.begin(), full_condition.end(), logic_split, { -1 });
    std::sregex_token_iterator end;

    for (; token_iter != end; ++token_iter) {
        std::smatch m;
        std::string expr = token_iter->str();
        std::regex expr_regex(R"(^\s*(\w+(?:\.\w+)?)\s*(=|>=|<=|>|<)\s*(.+?)\s*$)");
        if (!std::regex_match(expr, m, expr_regex)) continue;

        std::string field = m[1];
        std::string op = m[2];
        std::string val = m[3];

        for (const auto& table : tables) {
            for (const auto& idx : table->getIndexes()) {
                if (_stricmp(idx.field[0], field.c_str()) == 0){
                    BTree* btree = table->getBTreeByIndexName(idx.name);
                    if (!btree) continue;

                    used_index = true;

                    if (op == "=") {
                        FieldPointer* ptr = btree->find(val);
                        if (ptr) candidate_ids.insert(ptr->recordPtr.row_id);
                    }
                    else if (op == ">" || op == ">=" || op == "<" || op == "<=") {
                        std::string low = (op == ">" || op == ">=") ? val : "";
                        std::string high = (op == "<" || op == "<=") ? val : "\x7f"; // approximate max string

                        std::vector<FieldPointer> result;
                        btree->findRange(low, high, result);
                        for (auto& fp : result) {
                            if ((op == ">" && fp.fieldValue > val) ||
                                (op == ">=" && fp.fieldValue >= val) ||
                                (op == "<" && fp.fieldValue < val) ||
                                (op == "<=" && fp.fieldValue <= val)) {
                                candidate_ids.insert(fp.recordPtr.row_id);
                            }
                        }
                    }

                    break; // 已找到字段的索引，跳出当前表的索引查找
                }
            }
        }
    }

    std::vector<std::pair<uint64_t, std::unordered_map<std::string, std::string>>> result;

    if (used_index) {
        for (uint64_t row_id : candidate_ids) {
            auto it = filtered.find(row_id);
            if (it != filtered.end() && this->matches_condition(it->second, has_join)) {
                result.emplace_back(row_id, it->second);
            }
        }
    }
    else {
        for (const auto& [row_id, rec] : filtered) {
            if (full_condition.empty() || this->matches_condition(rec, has_join)) {
                result.emplace_back(row_id, rec);
            }
        }
    }

    return result;
}
