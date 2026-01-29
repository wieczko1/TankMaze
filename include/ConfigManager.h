#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

class ConfigManager {
private:
    std::map<std::string, std::string> m_configData;

public:
    void loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Blad: Nie mozna otworzyc pliku konfiguracyjnego: " << path << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string key, value;
            if (ss >> key >> value) {
                m_configData[key] = value;
            }
        }
        std::cout << "Konfiguracja wczytana z: " << path << std::endl;
    }

    std::string getString(const std::string& key) const {
        if (m_configData.count(key)) return m_configData.at(key);
        return "";
    }

    int getInt(const std::string& key) const {
        if (m_configData.count(key)) return std::stoi(m_configData.at(key));
        return 0;
    }

    float getFloat(const std::string& key) const {
        if (m_configData.count(key)) return std::stof(m_configData.at(key));
        return 0.0f;
    }
};
