#pragma once
#include "const.h"
#include <fstream>  
#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/filesystem.hpp>    
#include <map>
#include <iostream>

struct  SectionInfo 
{
	SectionInfo(){}
	~SectionInfo() { _section_datas.clear(); }
	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}
	SectionInfo& operator=(const SectionInfo& src)
	{
		if (&src == this)
			return *this;

		this->_section_datas = src._section_datas;
		return *this;
	}

	std::string operator[](const std::string& key)
	{
		if (_section_datas.find(key) == _section_datas.end())
		{
			return "";
		}
		return _section_datas[key];
	}

	std::string GetValue(const std::string& key) {
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		return _section_datas[key];
	}

	std::map<std::string, std::string>  _section_datas;
};

class ConfigMgr
{
public:
	~ConfigMgr()
	{
		_config_map.clear();
	}
	SectionInfo operator[](const std::string& section)
	{
		if (_config_map.find(section) == _config_map.end())
		{
			return SectionInfo();
		}
		return _config_map[section];
	}
	ConfigMgr();

	static ConfigMgr& Inst()
	{
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}

	ConfigMgr(const ConfigMgr& src) {
		_config_map = src._config_map;
	}
	ConfigMgr& operator=(const ConfigMgr& src)
	{
		if (&src == this) return *this;
		this->_config_map = src._config_map;
		return *this;
	}
	std::string GetValue(const std::string& section, const std::string& key);
private:	
	std::map<std::string, SectionInfo> _config_map;
};

