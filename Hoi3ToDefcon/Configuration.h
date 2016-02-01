#pragma once

#include <string>
#include <boost/filesystem.hpp>

class Configuration
{
public:
	static Configuration& Get() // Singleton
	{
		static Configuration instance;
		return instance;
	}

	bool Init(std::string sConfigPath);
	bool CreateDirectories();

	boost::filesystem::path GetSavePath();
	boost::filesystem::path GetHoi3Path();
	boost::filesystem::path GetHoi3ModPath();
	boost::filesystem::path GetBaseModPath();
	boost::filesystem::path GetOutputPath();

	boost::filesystem::path GetModdedHoi3File(boost::filesystem::path TargetPath);

private:
	Configuration();
	Configuration(Configuration const&);  // Unimplemented
	void operator=(Configuration const&); // Unimplemented

	boost::filesystem::path m_SavePath;
	boost::filesystem::path m_Hoi3Path;
	boost::filesystem::path m_Hoi3ModPath;
	boost::filesystem::path m_DefconPath;
	boost::filesystem::path m_BaseModPath;
	boost::filesystem::path m_OutputPath;

};