#define TOML_IMPLEMENTATION
#include <cstdlib>
#include <iostream>
#include <config.hpp>
#include <strutil.hpp>
#include <filesystem>

using std::string;

Config::Config(){
  if (!std::filesystem::exists(this->getCacheDir())) {
    std::cout << "TabAUR cache folder was not found, Creating folder at " << this->getCacheDir() << "!" << std::endl;
    std::filesystem::create_directory(this->getCacheDir());
  }
	
  if (!std::filesystem::exists(this->getConfigDir())) {
    std::cout << "TabAUR config folder was not found, Creating folder at " << this->getConfigDir() << "!" << std::endl;
    std::filesystem::create_directory(this->getConfigDir());
    std::ofstream configFile(this->getConfigDir() + "/config.toml");
    configFile.close();
  }
  string filename = this->getConfigDir() + "/config.toml";
  loadConfigFile(filename);
}

string Config::getHomeCacheDir() {
    char* dir = std::getenv("XDG_CACHE_HOME");
    if (dir != NULL && std::filesystem::exists(std::string(dir))) {
        std::string str_dir(dir);
        return hasEnding(str_dir, "/") ? str_dir.substr(0, str_dir.rfind('/')) : str_dir;
    } else
        return std::string(std::getenv("HOME")) + "/.cache";
}

string Config::getCacheDir() {
	std::optional<std::string> cacheDir = this->tbl["cacheDir"].value<std::string>();
	if (cacheDir && std::filesystem::exists(cacheDir.value()))
		return cacheDir.value();
	// if no custom cache dir found, make it up
	
	return this->getHomeCacheDir() + "/TabAUR";
}

string Config::getHomeConfigDir() {
    char* dir = std::getenv("XDG_CONFIG_HOME");
    if (dir != NULL && std::filesystem::exists(std::string(dir))) {
        std::string str_dir(dir);
        return hasEnding(str_dir, "/") ? str_dir.substr(0, str_dir.rfind('/')) : str_dir;
    } else
        return std::string(std::getenv("HOME")) + "/.config";
}

string Config::getConfigDir() {
    return this->getHomeConfigDir() + "/TabAUR";
}

void Config::loadConfigFile(string filename) {
    try
    {
        this->tbl = toml::parse_file(filename);
    }
    catch (const toml::parse_error& err)
    {
        std::cerr << "Parsing failed:\n" << err << "\n";
        exit(-1);
    }
}
