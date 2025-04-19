#ifndef DEBUG_H
#define DEBUG_H

#include <QString>

class debug {
public:
	static void printTB(const std::string& tbPath);
	static void printDB(const std::string& dbPath);
    static void printTDF(const std::string& tdfPath);
	static void printTIC(const std::string& ticPath);
};

#endif // DEBUG_H
