#include "Configuration.h"

#include "Parser.h"
#include "Log.h"
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

	if (nullptr == ConfigFile)
	{
		LOG(LogLevel::Error) << "Could not open the configuration file.";
		return false;
	}
		

	std::vector<Object*> Configs = ConfigFile->getValue("configuration");

	if (Configs.empty())
	{
		LOG(LogLevel::Error) << "Could not read the configuration file.";
		return false;
	}
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
		else if (ConfigLeaf->getKey().size() == 5)
		{
			if (ConfigLeaf->getKey().substr(0, 4) == "side")
			{
				int index = ConfigLeaf->getKey().at(4) - '0' - 1;
				if (index > 5) continue;

				for (std::string Tag : ConfigLeaf->getTokens())
					m_Sides[index].push_back(Tag);
			}
		}
	}

	m_SavePath = sSave;
	m_Hoi3Path = sHoi3Dir;
	m_DefconPath = sDefconDir;
	m_Hoi3ModPath = sHoi3ModDir;

	// Sanity checks
	if (m_SavePath.empty())
	{
		LOG(LogLevel::Error) << "No Hearts Of Iron 3 save file specified.";
		return false;
	}

	if (m_Hoi3Path.empty())
	{
		LOG(LogLevel::Error) << "No path to Hearts Of Iron 3 specified.";
		return false;
	}

	if (m_DefconPath.empty())
	{
		LOG(LogLevel::Warning) << "No path to DEFCON specified. The created mod will not be copied into place.";
	}

	if (m_SuperpowerOption == Superpowers::Custom)
	{
		int iEmptySides = 0;
		for (int i = 0; i < 6; i++)
		{
			if (m_Sides[i].empty())
			{
				LOG(LogLevel::Warning) << "Custom superpowers chosen, but side " << i << " is empty.";
				iEmptySides++;
			}
		}
		if (iEmptySides == 6)
		{
			LOG(LogLevel::Error) << "This nuclear war has no sides. World peace!";
			return false;
		}
		else if (iEmptySides == 5)
		{
			LOG(LogLevel::Error) << "This nuclear war has only one side. World conquest ensues!";
			return false;
		}
	}
	return true;
}

bool Configuration::CreateDirectories()
{
	std::string sName = m_SavePath.stem().string();

	m_BaseModPath = "basemod";
	m_OutputPath = "output";
	if (false == boost::filesystem::exists(m_OutputPath))
	{
		if (false == boost::filesystem::create_directories(m_OutputPath))
		{
			LOG(LogLevel::Error) << "Could not create directory " << m_OutputPath.string();
			return false;
		}
	}

	m_OutputPath = m_OutputPath / sName;

	boost::filesystem::remove_all(m_OutputPath);
	if (exists(m_OutputPath))
	{
		LOG(LogLevel::Error) << "Could not overwrite preexisting mod " << m_OutputPath.string();
		return false;
	}

	if (false == copyDir(m_BaseModPath, m_OutputPath))
	{
		LOG(LogLevel::Error) << "Could not create directory " << m_OutputPath.string();
		return false;
	}

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