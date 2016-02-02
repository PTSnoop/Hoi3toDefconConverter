#include "Configuration.h"

#include "Parser.h"
#include "Utils.h"

Configuration::Configuration()
: m_SavePath()
, m_Hoi3Path()
, m_Hoi3ModPath()
, m_DefconPath()
, m_BaseModPath()
, m_OutputPath()
{
	m_Sides = std::vector< std::vector< std::string> >( 6, std::vector<std::string>() );
}

bool Configuration::Init(std::string sConfigPath)
{
	std::string sSave = "";
	std::string sName = "";
	std::string sHoi3Dir = "";
	std::string sHoi3ModDir = "";
	std::string sDefconDir = "";
	std::string sSuperpowerOption = "";

	Object* ConfigFile = doParseFile(sConfigPath.c_str());
	std::vector<Object*> Configs = ConfigFile->getValue("configuration");
	Object* Config = Configs.at(0);
	for (auto ConfigLeaf : Config->getLeaves())
	{
		if (ConfigLeaf->getKey() == "save")
			sSave = ConfigLeaf->getLeaf();
		else if (ConfigLeaf->getKey() == "HOI3directory")
			sHoi3Dir = ConfigLeaf->getLeaf();
		else if (ConfigLeaf->getKey() == "HOI3ModDirectory")
			sHoi3ModDir = ConfigLeaf->getLeaf();
		else if (ConfigLeaf->getKey() == "DEFCONdirectory")
			sDefconDir = ConfigLeaf->getLeaf();
		else if (ConfigLeaf->getKey() == "superpowers")
		{
			sSuperpowerOption = ConfigLeaf->getLeaf();
			if (sSuperpowerOption == "custom")
				m_SuperpowerOption = Superpowers::Custom;
			else if (sSuperpowerOption == "factions")
				m_SuperpowerOption = Superpowers::Factions;
			else
				m_SuperpowerOption = Superpowers::Powerful;
		}
		else if (ConfigLeaf->getKey().substr(0, 4) == "side")
		{
			int index = atoi(ConfigLeaf->getKey().substr(4).c_str())-1;
			if (index > 5) continue;

			for (std::string Tag : ConfigLeaf->getTokens())
			{
				m_Sides[index].push_back(Tag);
			}
		}
	}

	m_SavePath = sSave;
	m_Hoi3Path = sHoi3Dir;
	m_DefconPath = sDefconDir;
	m_Hoi3ModPath = sHoi3ModDir;

	return true;
}

bool Configuration::CreateDirectories()
{
	std::string sName = m_SavePath.stem().string();

	m_BaseModPath = "basemod";
	m_OutputPath = "output";
	boost::filesystem::create_directories(m_OutputPath);

	m_OutputPath = m_OutputPath / sName;
	boost::filesystem::remove_all(m_OutputPath);
	copyDir(m_BaseModPath, m_OutputPath);

	return true;
}

boost::filesystem::path Configuration::GetModdedHoi3File(boost::filesystem::path TargetPath)
{
	boost::filesystem::path PathThatExists = GetHoi3ModPath() / TargetPath;
	if (boost::filesystem::exists(PathThatExists))
	{
		return PathThatExists;
	}
	PathThatExists = GetHoi3Path() / TargetPath;
	if (boost::filesystem::exists(PathThatExists))
	{
		return PathThatExists;
	}
	return "";
}

boost::filesystem::path Configuration::GetSavePath() { return m_SavePath; }
boost::filesystem::path Configuration::GetHoi3Path() { return m_Hoi3Path; }
boost::filesystem::path Configuration::GetHoi3ModPath() { return m_Hoi3ModPath; }
boost::filesystem::path Configuration::GetBaseModPath() { return m_BaseModPath; }
boost::filesystem::path Configuration::GetOutputPath() { return m_OutputPath; }
Superpowers Configuration::GetSuperpowerOption() { return m_SuperpowerOption; };
std::vector< std::vector<std::string> > Configuration::GetCustomSides() { return m_Sides; };