// Hoi3ToDefcon.cpp : Defines the entry point for the console application.
//

#define VERSION "v1.0"

#include "stdafx.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/filesystem.hpp>

#include "CImg.h"
using namespace cimg_library;

#include "Utils.h"
#include "Log.h"
#include "Object.h"
#include "Parser.h"
#include "CsvParser.h"
#include "GetEdges.h"

#include "Configuration.h"

auto real_rand = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), mt19937(12345));

bool GetProvincePositions(std::map< int, std::pair<double, double> >& ProvincePositions)
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path Hoi3MapPositions = Config.GetModdedHoi3File("map/positions.txt");
	if (Hoi3MapPositions.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 map/positions.txt";
		return false;
	}

	Object* positions = doParseFile(Hoi3MapPositions.string().c_str());
	if (nullptr == positions)
	{
		LOG(LogLevel::Error) << "Could not read " << Hoi3MapPositions.string();
		return false;
	}

	std::vector<Object*> positionLeaves = positions->getLeaves();
	for (Object* Leaf : positionLeaves)
	{
		std::string sProvinceId = Leaf->getKey();
		std::string x = "";
		std::string y = "";

		for (auto Subleaf : Leaf->getLeaves())
		{
			for (auto Subsubleaf : Subleaf->getLeaves())
			{
				if (Subsubleaf->getKey() == "x")
					x = Subsubleaf->getLeaf();

				if (Subsubleaf->getKey() == "y")
					y = Subsubleaf->getLeaf();

				for (auto Subsubsubleaf : Subsubleaf->getLeaves())
				{
					if (!x.empty() && !y.empty())
						break;

					if (Subsubsubleaf->getKey() == "x")
						x = Subsubsubleaf->getLeaf();

					if (Subsubsubleaf->getKey() == "y")
						y = Subsubsubleaf->getLeaf();
				}
				if (!x.empty() && !y.empty())
					break;
			}
			if (!x.empty() && !y.empty())
				break;
		}

		if (false == is_number(sProvinceId.c_str()))
		{
			LOG(LogLevel::Warning) << "Confused by province number " << sProvinceId << " in Hoi3 map/positions.txt";
			continue;
		}
		ProvincePositions[atoi(sProvinceId.c_str())] = std::make_pair(atof(x.c_str()), atof(y.c_str()));
	}

	return true;
}

bool GetProvinceNumbers(std::map<int, std::string>& Territories, std::map<std::string, int>& TerritoryCounts, std::map<int, double>& CityScore, std::map<int, bool>& Capitals, Object* SaveFile)
{
	std::string sSeed = SaveFile->getLeaf("seed");
	if (is_number(sSeed))
	{
		int iSeed = atoi(sSeed.c_str());
		real_rand = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), mt19937(iSeed));
	}

	std::vector<Object*> Leaves = SaveFile->getLeaves();
	for (Object* Leaf : Leaves)
	{
		std::string sProvinceId = Leaf->getKey();
		if (false == is_number(sProvinceId))
			continue;
		int iProvinceId = atoi(sProvinceId.c_str());

		std::vector<Object*> Owners = Leaf->getValue("controller");
		if (Owners.empty())
			continue;
		std::string sOwner = Owners.at(0)->getLeaf();

		Territories[iProvinceId] = sOwner;
		TerritoryCounts[sOwner]++;

		std::vector<Object*> Capital = Leaf->getValue("capital");
		Capitals[iProvinceId] = (!Capital.empty());

		std::vector<Object*> Points = Leaf->getValue("points");
		std::string sPoints = "0";
		if (!Points.empty())
			sPoints = Points.at(0)->getLeaf();

		double fInfras = 0.0;
		std::vector<Object*> Infra = Leaf->getValue("infra");
		if (!Infra.empty())
		{
			Object* Infras = Infra.at(0);

			std::vector <std::string> InfraTokens = Infras->getTokens();
			if (!InfraTokens.empty())
			{
				fInfras = atof(InfraTokens[0].c_str());
			}
		}

		if (sPoints.empty()) continue;
		double fPoints = atof(sPoints.c_str());
		double fScore = (1.0 + log(fPoints + 1.0)) * (fInfras + 1.0) + real_rand();

		fScore -= 10;
		if (fScore > 0)
		{

			fScore = pow(fScore, 1.5);
			fScore *= 3e5;
			if (Capitals[iProvinceId])
				fScore *= 2;

			CityScore[iProvinceId] = floor(fScore);
		}
	}
	return true;
}

bool GetLocalisation(std::map<int, std::string>& ProvinceNames, std::map<std::string, std::string>& CountryNames)
{
	Configuration& Config = Configuration::Get();

	std::vector< std::vector<std::string> > ProvinceNamesFile;
	boost::filesystem::path ProvinceNamesCsvPath = Config.GetModdedHoi3File("localisation/province_names.csv");
	if (ProvinceNamesCsvPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 localisation/province_names.csv";
		return false;
	}

	if (false == ParseCsvFile(ProvinceNamesFile, ProvinceNamesCsvPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << ProvinceNamesCsvPath.string();
		return false;
	}

	std::vector< std::vector<std::string> > CountryNamesFile;
	boost::filesystem::path CountriesCsvPath = Config.GetModdedHoi3File("localisation/countries.csv");
	if (CountriesCsvPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 localisation/countries.csv";
		return false;
	}

	if (false == ParseCsvFile(CountryNamesFile, CountriesCsvPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << CountriesCsvPath.string();
		return false;
	}

	for (auto NameLine : ProvinceNamesFile)
	{
		if (NameLine.size() < 2) continue;
		std::string NameNumber = NameLine[0].substr(4);
		if (false == is_number(NameNumber))
		{
			continue;
		}
		ProvinceNames[atoi(NameNumber.c_str())] = NameLine[1];
	}

	for (auto NameLine : CountryNamesFile)
	{
		if (NameLine.size() < 2) continue;
		CountryNames[NameLine[0]] = NameLine[1];
	}

	return true;
}

bool CreateCitiesFile(std::vector< pair<int, double> >& SortedCityScores,
	std::map<int, std::string>& Territories,
	std::map<std::string, std::string>& CountryNames,
	std::map<int, std::string>& ProvinceNames,
	std::map< int, std::pair<double, double> >& ProvincePositions,
	std::map<int, bool>& Capitals)
{
	Configuration& Config = Configuration::Get();

	boost::filesystem::path CitiesDatPath = Config.GetOutputPath() / "data" / "earth" / "cities.dat";
	std::ofstream CitiesDat(CitiesDatPath.string().c_str());
	if (!CitiesDat )
	{
		LOG(LogLevel::Error) << "Could not write to " << CitiesDatPath.string();
		return false;
	}

	for (auto i = SortedCityScores.begin(); i != SortedCityScores.end(); ++i)
	{
		int iProvinceId = i->first;
		if (iProvinceId == 0)
			continue;

		std::string sTag = Territories[iProvinceId];
		std::string sCountryName = CountryNames[sTag];
		std::string sProvinceName = ProvinceNames[iProvinceId];
		std::pair<double, double> Pos = ProvincePositions[iProvinceId];

		if (sProvinceName.empty())
			continue;

		std::string sX = std::to_string((360.0*(Pos.first / 5616.0)) - 180.0);
		std::string sY = std::to_string((138.0*(Pos.second / 2160.0)) - 64.0 + 16.0 - 9.0);

		unsigned long long llPopulation = i->second;
		std::string sPopulation = std::to_string(llPopulation);
		bool bIsCapital = Capitals[iProvinceId];

		std::string sCityLine(125, ' ');

		sCityLine.replace(0, sProvinceName.size(), sProvinceName);
		sCityLine.replace(41, sCountryName.size(), sCountryName);
		sCityLine.replace(82, sX.size(), sX);
		sCityLine.replace(96, sY.size(), sY);
		sCityLine.replace(110, sPopulation.size(), sPopulation);
		sCityLine.replace(124, 1, bIsCapital ? "1" : "0");
		CitiesDat << sCityLine << std::endl;
	}

	CitiesDat.close();
	return true;
}

bool GetProvinceColourIds(std::map<ColourTriplet, int>& ColourToId)
{
	Configuration& Config = Configuration::Get();
	std::vector< std::vector<std::string> > Definitions;

	boost::filesystem::path DefinitionCsvPath = Config.GetModdedHoi3File("map/definition.csv");
	if (DefinitionCsvPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 map/definition.csv";
		return false;
	}
	if (false == ParseCsvFile(Definitions, DefinitionCsvPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << DefinitionCsvPath.string();
		return false;
	}

	for (auto Line : Definitions)
	{
		if (false == is_number(Line[0]))
			continue;

		ColourTriplet c(atoi(Line[1].c_str()), atoi(Line[2].c_str()), atoi(Line[3].c_str()));
		ColourToId[c] = atoi(Line[0].c_str());
	}
	return true;
}

bool GetSides(std::vector< std::vector< std::string > >& Sides, Object* SaveFile, std::vector< pair<std::string, int> >& SortedTerritoryCounts)
{
	Configuration& Config = Configuration::Get();
	Sides = std::vector< std::vector< std::string> >(6, std::vector<std::string>());

	switch (Config.GetSuperpowerOption())
	{
		case Superpowers::Custom:
		{
			Sides = std::vector< std::vector<std::string> >(Config.GetCustomSides());
			break;
		}
		case Superpowers::Factions:
		{
			auto FactionsObjects = SaveFile->getValue("faction");
			if (FactionsObjects.empty())
			{
				LOG(LogLevel::Error) << "Could not find factions in the save file.";
				return false;
			}

			std::vector<Object*> Factions = FactionsObjects[0]->getLeaves();
			std::vector<std::string> Added;
			int i = 0;
			for (; i < 3; i++)
			{
				for (auto CountryLeaf : Factions[i]->getLeaves())
				{
					if (CountryLeaf->getKey() == "country")
					{
						Sides[i].push_back(CountryLeaf->getLeaf());
						Added.push_back(CountryLeaf->getLeaf());
					}
				}
			}
			for (auto NextLargest : SortedTerritoryCounts)
			{
				std::string sNextLargest = NextLargest.first;
				if (std::find(Added.begin(), Added.end(), sNextLargest) == Added.end())
				{
					Sides[i].push_back(sNextLargest);
					i++;
					if (i >= 6) break;
				}
			}
			break;
		}
		case Superpowers::Powerful:
		{
			for (int i = 0; i < 6; i++)
			{
				std::string sRelevant = SortedTerritoryCounts[i].first;
				Sides[i].push_back(sRelevant);
			}
			break;
		}
	}
	return true;
}

bool CreateTerritoryMaps(std::vector< std::vector< std::string > >& Sides, std::map<ColourTriplet, int>& ColourToId, std::map<int, std::string>& Territories)
{
	Configuration& Config = Configuration::Get();

	boost::filesystem::path LandbasePath = Config.GetOutputPath() / "data" / "earth" / "land_base.bmp";
	boost::filesystem::path SailablePath = Config.GetOutputPath() / "data" / "earth" / "sailable.bmp";
	boost::filesystem::path CoastlinesPath = Config.GetOutputPath() / "data" / "earth" / "coastlines.bmp";

	CImg<unsigned char> Landbase(LandbasePath.string().c_str());
	if (NULL == Landbase)
	{
		LOG(LogLevel::Error) << "Could not open " + LandbasePath.string();
		return false;
	}

	CImg<unsigned char> Sailable(SailablePath.string().c_str());
	if (NULL == Sailable)
	{
		LOG(LogLevel::Error) << "Could not open " + SailablePath.string();
		return false;
	}

	CImg<unsigned char> Coastlines(CoastlinesPath.string().c_str());
	if (NULL == Coastlines)
	{
		LOG(LogLevel::Error) << "Could not open " + CoastlinesPath.string();
		return false;
	}

	boost::filesystem::path HoiProvinceMapPath = Config.GetModdedHoi3File("map/provinces.bmp");
	if (HoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 map/provinces.bmp";
		return false;
	}

	CImg<unsigned char> ProvinceMapFromFile(HoiProvinceMapPath.string().c_str());
	if (NULL == ProvinceMapFromFile)
	{
		LOG(LogLevel::Error) << "Could not open " + HoiProvinceMapPath.string();
		return false;
	}

	ProvinceMapFromFile.resize(512, 200, 1, 3, 1);
	ProvinceMapFromFile.mirror('y');

	CImg<unsigned char> ProvinceMap(512, 285, 1, 3, 0);
	ProvinceMap.draw_image(0, 26, ProvinceMapFromFile);

	std::vector< std::tuple< std::vector<std::string>, CImg<unsigned char>, CImg<unsigned char> > > TerritoryMaps;
	for (int i = 0; i < 6; i++)
	{
		TerritoryMaps.push_back(
			std::tuple< std::vector<std::string>, CImg<unsigned char>, CImg<unsigned char> >
			(Sides[i], CImg<unsigned char>(512, 285, 1, 3, 0), CImg<unsigned char>(512, 285, 1, 3, 0))
			);
	}

	const unsigned char grey[] = { 128, 128, 128 };

	for (int x = 0; x < ProvinceMap.width(); x++)
	{
		for (int y = 0; y < ProvinceMap.height(); y++)
		{
			ColourTriplet ProvinceColour(
				ProvinceMap(x, y, 0, 0),
				ProvinceMap(x, y, 0, 1),
				ProvinceMap(x, y, 0, 2)
				);

			int iProvinceId = ColourToId[ProvinceColour];
			std::string sTag = Territories[iProvinceId];

			for (int i = 0; i < 6; i++)
			{
				auto Relevants = std::get<0>(TerritoryMaps[i]);
				if (std::find(Relevants.begin(), Relevants.end(), sTag) != Relevants.end())
				{
					if (0 == Landbase(x, y, 0, 0))
						continue;

					std::get<1>(TerritoryMaps[i])(x, y, 0, 0) = 255;
					std::get<1>(TerritoryMaps[i])(x, y, 0, 1) = 255;
					std::get<1>(TerritoryMaps[i])(x, y, 0, 2) = 255;

					if (0 != Coastlines(x, y, 0, 0))
						std::get<2>(TerritoryMaps[i]).draw_circle(x, y, 30, grey);
				}
			}
		}

	}

	for (int i = 0; i < 6; i++)
	{
		std::get<2>(TerritoryMaps[i]) &= Sailable;
	}

	for (int i = 0; i < 6; i++)
	{
		std::get<1>(TerritoryMaps[i]) |= std::get<2>(TerritoryMaps[i]);
	}

	boost::filesystem::path OutputDataEarthPath = Config.GetOutputPath() / "data" / "earth";
	std::vector<boost::filesystem::path> ContinentPaths = {
		OutputDataEarthPath / "africa.bmp",
		OutputDataEarthPath / "europe.bmp",
		OutputDataEarthPath / "northamerica.bmp",
		OutputDataEarthPath / "russia.bmp",
		OutputDataEarthPath / "southamerica.bmp",
		OutputDataEarthPath / "southasia.bmp"
	};

	for (int i = 0; i < 6; i++)
	{
		std::get<1>(TerritoryMaps[i]).blur(1);
		std::get<1>(TerritoryMaps[i]).save(ContinentPaths[i].string().c_str());
	}
	return true;
}

bool CreateInternationalBoundaries(std::map<ColourTriplet, int>& ColourToId, std::map<int, std::string>& Territories)
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path BigHoiProvinceMapPath = Config.GetModdedHoi3File("map/provinces.bmp");
	if (BigHoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi3 map/provinces.bmp .";
		LOG(LogLevel::Error) << "But it was perfectly fine earlier. Now I'm confused.";
		return false;
	}

	CImg<unsigned char> BigProvinceMap(BigHoiProvinceMapPath.string().c_str());
	if (NULL == BigProvinceMap)
	{
		LOG(LogLevel::Error) << "Could not read " + BigHoiProvinceMapPath.string();
		LOG(LogLevel::Error) << "But it was perfectly fine earlier. Now I'm confused.";
		return false;
	}

	BigProvinceMap.mirror('y');

	std::map<std::string, ColourTriplet> TmpNationColours;

	// Redundancy!
	for (int x = 0; x < BigProvinceMap.width(); x++)
	{
		for (int y = 0; y < BigProvinceMap.height(); y++)
		{
			ColourTriplet ProvinceColour(
				BigProvinceMap(x, y, 0, 0),
				BigProvinceMap(x, y, 0, 1),
				BigProvinceMap(x, y, 0, 2)
				);

			int Id = ColourToId[ProvinceColour];

			auto pOwner = Territories.find(Id);
			if (pOwner == Territories.end())
			{
				BigProvinceMap(x, y, 0, 0) = 0;
				BigProvinceMap(x, y, 0, 1) = 0;
				BigProvinceMap(x, y, 0, 2) = 0;
				continue;
			}

			std::string sOwner = pOwner->second;

			auto NewColour = TmpNationColours.find(sOwner);
			if (NewColour != TmpNationColours.end())
			{
				BigProvinceMap(x, y, 0, 0) = std::get<0>(NewColour->second);
				BigProvinceMap(x, y, 0, 1) = std::get<1>(NewColour->second);
				BigProvinceMap(x, y, 0, 2) = std::get<2>(NewColour->second);
			}
			else
			{
				TmpNationColours[sOwner] = ProvinceColour;
			}
		}
	}

	const unsigned char black[] = { 0, 0, 0 };
	BigProvinceMap.draw_fill(0, 0, black);

	// Kludgy floodfills.
	for (int x = 0; x < BigProvinceMap.width(); x++)
	{
		for (int y = 0; y < BigProvinceMap.height(); y++)
		{
			if (BigProvinceMap(x, y, 0, 0) == 0 && BigProvinceMap(x, y, 0, 1) == 0 && BigProvinceMap(x, y, 0, 2) == 0)
				continue;

			if (real_rand() > 0.0002) continue;

			const unsigned char newcolour[] = {
				BigProvinceMap(x, y, 0, 0) + (10.0 * real_rand()) - 5.0,
				BigProvinceMap(x, y, 0, 1) + (10.0 * real_rand()) - 5.0,
				BigProvinceMap(x, y, 0, 2) + (10.0 * real_rand()) - 5.0
			};
			BigProvinceMap.draw_fill(x, y, newcolour);
		}
	}

	//


	std::set< std::tuple<int, int, int> > GotColours;
	GetEdges Edges;
	if (false == Edges.Init(&BigProvinceMap))
	{
		LOG(LogLevel::Warning) << "Could not read country edges. This map will look kinda blank.";
	}
	else
	{
		 Edges.GetAllColours(GotColours);
	}

	boost::filesystem::path InternationalDatPath = Config.GetOutputPath() / "data" / "earth" / "international.dat";
	std::ofstream InternationalDat(InternationalDatPath.string().c_str());

	if (!InternationalDat)
	{
		LOG(LogLevel::Error) << "Could not write to " << InternationalDatPath.string();
		return false;
	}

	bool lastB = false;
	for (auto GotColour : GotColours)
	{
		if (!lastB)
		{
			InternationalDat << "b" << std::endl;
			lastB = true;
		}

		std::vector< std::pair<double, double> > Points;
		Edges.Get(Points, GotColour);
		for (std::pair<double, double>& Point : Points)
		{
			if (Point.first < 0 && Point.second < 0)
			{
				if (!lastB)
					InternationalDat << "b" << std::endl;
				lastB = true;
			}
			else
			{
				double x = ((Point.first / BigProvinceMap.width())*360.0) - 180.0;
				double y = ((-Point.second / BigProvinceMap.height())*141.0) + 82.0;
				InternationalDat << x << " " << y << std::endl;
				lastB = false;
			}
		}
	}

	InternationalDat.close();
	return true;
}

bool CreateModTxt()
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path OutputPath = Config.GetOutputPath();

	std::string sName = OutputPath.filename().string();

	boost::filesystem::path ModTxtPath = Config.GetOutputPath() / "mod.txt";
	std::ofstream ModTxt(ModTxtPath.string().c_str());
	if (!ModTxt)
	{
		LOG(LogLevel::Error) << "Could not write to " << ModTxtPath.string();
		return false;
	}

	ModTxt << "Name         " << "Converted - " << sName << std::endl;
	ModTxt << "Version      " << VERSION << std::endl;
	ModTxt << "Author       " << "PTSnoop" << std::endl;
	ModTxt << "Website      " << "https://github.com/PTSnoop/Hoi3toDefconConverter" << std::endl;
	ModTxt << "Comment      " << "This mod was created using the Hearts Of Iron 3 to Defcon converter." << std::endl;

	ModTxt.close();
	return true;
}

bool CopyModIntoDefcon()
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path OutputPath = Config.GetOutputPath();
	boost::filesystem::path DefconPath = Config.GetDefconPath();

	if (DefconPath.empty())
		return true;

	boost::filesystem::path DefconModPath = DefconPath / "mods";
	if (false == boost::filesystem::exists(DefconModPath))
	{
		if (false == boost::filesystem::create_directories(DefconModPath))
		{
			LOG(LogLevel::Error) << "Could not create directory " << DefconModPath.string();
			return false;
		}
	}

	boost::filesystem::path Name = OutputPath.filename();
	DefconModPath = DefconModPath / Name;

	boost::filesystem::remove_all(DefconModPath);
	if (exists(DefconModPath))
	{
		LOG(LogLevel::Error) << "Could not overwrite preexisting Defcon mod " << DefconModPath.string();
		return false;
	}

	if (false == copyDir(OutputPath, DefconModPath))
	{
		LOG(LogLevel::Error) << "Could not create " << DefconModPath.string();
		return false;
	}

	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	LOG(LogLevel::Info) << "Loading configuration...";
	Configuration& Config = Configuration::Get();
	if (false == Config.Init("configuration.txt"))
	{
		LOG(LogLevel::Error) << "Could not load configuration. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Loading save file " << Config.GetSavePath().string() << " ...";
	Object* SaveFile = doParseFile(Config.GetSavePath().string().c_str());

	if (nullptr == SaveFile)
	{
		LOG(LogLevel::Error) << "Could not load save file. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating mod folder structure...";
	if (false == Config.CreateDirectories())
	{
		LOG(LogLevel::Error) << "Could not create mod folder structure. Exiting.";
		return 1;
	}

	std::map<int, std::string> Territories;
	std::map<std::string, int> TerritoryCounts;
	std::map<int, double> CityScore;
	std::map<int, bool> Capitals;

	LOG(LogLevel::Info) << "Reading save file contents...";
	if (false == GetProvinceNumbers(Territories, TerritoryCounts, CityScore, Capitals, SaveFile))
	{
		LOG(LogLevel::Error) << "Could not understand the save file. Exiting.";
		return 1;
	}

	std::map< int, std::pair<double, double> > ProvincePositions;

	LOG(LogLevel::Info) << "Getting province coordinates...";
	if (false == GetProvincePositions(ProvincePositions))
	{
		LOG(LogLevel::Error) << "Could not get province coordinates. Exiting.";
		return 1;
	}

	std::vector<pair<std::string, int>> SortedTerritoryCounts;
	for (auto itr = TerritoryCounts.begin(); itr != TerritoryCounts.end(); ++itr)
		SortedTerritoryCounts.push_back(*itr);

	sort(
		SortedTerritoryCounts.begin(), 
		SortedTerritoryCounts.end(), 
		[=](pair<std::string, int>& a, pair<std::string, int>& b)
			{ return a.second > b.second; }
	);

	std::vector<pair<int, double>> SortedCityScores;
	for (auto itr = CityScore.begin(); itr != CityScore.end(); ++itr)
		SortedCityScores.push_back(*itr);

	sort(
		SortedCityScores.begin(), 
		SortedCityScores.end(), 
		[=](pair<int, double>& a, pair<int, double>& b)
			{ return a.second > b.second; }
	);

	std::map<ColourTriplet, int> ColourToId;
	LOG(LogLevel::Info) << "Reading the province map...";
	if (false == GetProvinceColourIds(ColourToId))
	{
		LOG(LogLevel::Error) << "Could not understand the province map. Exiting.";
		return 1;
	}

	std::map<int, std::string> ProvinceNames;
	std::map<std::string, std::string> CountryNames;

	LOG(LogLevel::Info) << "Reading names from localization...";
	if (false == GetLocalisation(ProvinceNames, CountryNames))
	{
		LOG(LogLevel::Error) << "No comprende. Exiting.";
		return 1;
	}

	std::vector< std::vector< std::string > > Sides;

	LOG(LogLevel::Info) << "Taking sides...";
	if (false == GetSides(Sides, SaveFile, SortedTerritoryCounts))
	{
		LOG(LogLevel::Error) << "Could not assign sides. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating cities file...";
	if (false == CreateCitiesFile(SortedCityScores, Territories, CountryNames, ProvinceNames, ProvincePositions, Capitals))
	{
		LOG(LogLevel::Error) << "Could not create cities.dat . Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating territory maps...";
	if (false == CreateTerritoryMaps(Sides, ColourToId, Territories))
	{
		LOG(LogLevel::Error) << "Could not create territory maps. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Drawing international boundaries...";
	if (false == CreateInternationalBoundaries(ColourToId, Territories))
	{
		LOG(LogLevel::Error) << "Could not create international.dat . Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating mod information file...";
	if (false == CreateModTxt())
	{
		LOG(LogLevel::Error) << "Could not create mod.txt .";
		return 1;
	}
	
	if (false == Config.GetDefconPath().empty())
	{
		LOG(LogLevel::Info) << "Copying into the Defcon mod directory...";
		if (false == CopyModIntoDefcon())
		{
			LOG(LogLevel::Warning) << "Could not copy the mod into the Defcon directory.";
		}
	}

	LOG(LogLevel::Info) << "Complete!";
	return 0;
}

