// Csv.cpp
#include "Csv.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<std::vector<double>> loadCsv(const std::string& path) {
    std::vector<std::vector<double>> rows;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open " << path << "\n";
        return rows;
    }
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        std::vector<double> row;
        while (std::getline(ss, cell, ',')) {
            row.push_back(std::stod(cell));
        }
        rows.push_back(row);
    }
    return rows;
}