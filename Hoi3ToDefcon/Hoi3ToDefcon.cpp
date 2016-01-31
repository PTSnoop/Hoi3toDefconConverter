// Hoi3ToDefcon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/filesystem.hpp>

#include "CImg.h"
using namespace cimg_library;

#include "Object.h"
#include "Parser.h"
#include "CsvParser.h"
#include "GetEdges.h"

typedef std::tuple<unsigned char, unsigned char, unsigned char> ColourTriplet;

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

bool copyDir(
	boost::filesystem::path const & source,
	boost::filesystem::path const & destination
	)
{
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			std::cerr << "Source directory " << source.string()
				<< " does not exist or is not a directory." << '\n'
				;
			return false;
		}
		if (fs::exists(destination))
		{
			std::cerr << "Destination directory " << destination.string()
				<< " already exists." << '\n'
				;
			return false;
		}
		// Create the destination directory
		if (!fs::create_directory(destination))
		{
			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n'
				;
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!copyDir(
					current,
					destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename()
					);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
	std::string sSave = "";
	std::string sName = "";
	std::string sHoi3Dir = "";
	std::string sHoi3ModDir = "";
	std::string sDefconDir = "";

	Object* ConfigFile = doParseFile("configuration.txt");
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
	}

	boost::filesystem::path SavePath(sSave);
	boost::filesystem::path Hoi3Path(sHoi3Dir);
	boost::filesystem::path DefconPath(sDefconDir);
	boost::filesystem::path Hoi3ModPath(sHoi3ModDir);

	sName = SavePath.stem().string();

	boost::filesystem::path BaseModPath = "basemod";
	boost::filesystem::path OutputPath = "output";
	boost::filesystem::create_directories(OutputPath);

	OutputPath = OutputPath / sName;
	boost::filesystem::remove_all(OutputPath);
	copyDir(BaseModPath, OutputPath);

	Object* SaveFile = doParseFile(SavePath.string().c_str());
	
	///

	std::map< int, std::pair<double, double> > ProvincePositions;
	boost::filesystem::path Hoi3MapPositions = Hoi3ModPath / "map" / "positions.txt";
	if (false == boost::filesystem::exists(Hoi3MapPositions))
	{
		Hoi3MapPositions = Hoi3Path / "map" / "positions.txt";
	}

	Object* positions = doParseFile(Hoi3MapPositions.string().c_str());
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

		ProvincePositions[atoi(sProvinceId.c_str())] = std::make_pair(atof(x.c_str()), atof(y.c_str()));
	}


	double minX = 9999.9;
	double minY = 9999.9;
	double maxX = -9999.9;
	double maxY = -9999.9;
	for (auto pair : ProvincePositions)
	{
		double x = pair.second.first;
		double y = pair.second.second;

		if (x < minX)
			minX = x;
		if (y < minY)
			minY = y;

		if (x > maxX)
			maxX = x;
		if (y > maxY)
			maxY = y;
	}

	///

	std::map<int, std::string> Territories;
	std::map<std::string, int> TerritoryCounts;
	std::map<int, double> CityScore;
	std::map<int, bool> Capitals;

	std::string sSeed = SaveFile->getLeaf("seed");
	int iSeed = atoi(sSeed.c_str());
	auto real_rand = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), mt19937(iSeed));

	std::vector<Object*> Leaves = SaveFile->getLeaves();
	for (Object* Leaf : Leaves)
	{
		std::string sProvinceId = Leaf->getKey();
		if (false == is_number(sProvinceId))
			continue;
		int iProvinceId = atoi(sProvinceId.c_str());

		std::vector<Object*> Owners = Leaf->getValue("owner");
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



	std::vector<pair<std::string, int>> SortedTerritoryCounts;
	for (auto itr = TerritoryCounts.begin(); itr != TerritoryCounts.end(); ++itr)
		SortedTerritoryCounts.push_back(*itr);

	sort(
		SortedTerritoryCounts.begin(), SortedTerritoryCounts.end(), [=](pair<std::string, int>& a, pair<std::string, int>& b)
	{
		return a.second > b.second;
	}
	);

	std::vector<pair<int, double>> SortedCityScores;
	for (auto itr = CityScore.begin(); itr != CityScore.end(); ++itr)
		SortedCityScores.push_back(*itr);

	sort(
		SortedCityScores.begin(), SortedCityScores.end(), [=](pair<int,double>& a, pair<int,double>& b)
	{
		return a.second > b.second;
	}
	);
	
	std::vector< std::vector<std::string> > ProvinceNamesFile;
	boost::filesystem::path ProvinceNamesCsvPath = Hoi3ModPath / "localisation" / "province_names.csv";
	if (false == boost::filesystem::exists(ProvinceNamesCsvPath))
	{
		ProvinceNamesCsvPath = Hoi3Path / "localisation" / "province_names.csv";
	}
	ParseCsvFile(ProvinceNamesFile, ProvinceNamesCsvPath.string().c_str());

	std::vector< std::vector<std::string> > CountryNamesFile;
	boost::filesystem::path CountriesCsvPath = Hoi3ModPath / "localisation" / "countries.csv";
	if (false == boost::filesystem::exists(CountriesCsvPath))
	{
		CountriesCsvPath = Hoi3Path / "localisation" / "countries.csv";
	}
	ParseCsvFile(CountryNamesFile, CountriesCsvPath.string().c_str());

	std::map<int, std::string> ProvinceNames;
	std::map<std::string, std::string> CountryNames;

	for (auto NameLine : ProvinceNamesFile)
	{
		if (NameLine.size() < 2) continue;
		std::string NameNumber = NameLine[0].substr(4);
		ProvinceNames[atoi(NameNumber.c_str())] = NameLine[1];
	}
	
	for (auto NameLine : CountryNamesFile)
	{
		if (NameLine.size() < 2) continue;
		CountryNames[NameLine[0]] = NameLine[1];
	}

	boost::filesystem::path CitiesDatPath = OutputPath / "data" / "earth" / "cities.dat";
	std::ofstream CitiesDat(CitiesDatPath.string().c_str());

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

		std::string sX = std::to_string((360.0*(Pos.first/5616.0))-180.0);
		std::string sY = std::to_string((138.0*(Pos.second/2160.0))-64.0+16.0-9.0);

		unsigned long long llPopulation = i->second;
		std::string sPopulation = std::to_string(llPopulation);
		bool bIsCapital = Capitals[iProvinceId];

		std::string sCityLine(125,' ');
				
		sCityLine.replace(0, sProvinceName.size(), sProvinceName);
		sCityLine.replace(41, sCountryName.size(), sCountryName);
		sCityLine.replace(82, sX.size(), sX);
		sCityLine.replace(96, sY.size(), sY);
		sCityLine.replace(110, sPopulation.size(), sPopulation);
		sCityLine.replace(124, 1, bIsCapital ? "1" : "0");
		CitiesDat << sCityLine << std::endl;
	}

	CitiesDat.close();

	///

	std::vector< std::vector<std::string> > Definitions;
	std::map<ColourTriplet, int> ColourToId;

	boost::filesystem::path DefinitionCsvPath = Hoi3ModPath / "map" / "definition.csv";
	if (false == boost::filesystem::exists(DefinitionCsvPath))
	{
		DefinitionCsvPath = Hoi3Path / "map" / "definition.csv";
	}

	ParseCsvFile(Definitions, DefinitionCsvPath.string().c_str());

	for (auto Line : Definitions)
	{
		if (false == is_number(Line[0]))
			continue;

		ColourTriplet c(atoi(Line[1].c_str()), atoi(Line[2].c_str()), atoi(Line[3].c_str()));
		ColourToId[c] = atoi(Line[0].c_str());
	}

	///

	boost::filesystem::path LandbasePath = OutputPath / "data" / "earth" / "land_base.bmp";
	boost::filesystem::path SailablePath = OutputPath / "data" / "earth" / "sailable.bmp";
	boost::filesystem::path CoastlinesPath = OutputPath / "data" / "earth" / "coastlines.bmp";

	CImg<unsigned char> Landbase(LandbasePath.string().c_str());
	CImg<unsigned char> Sailable(SailablePath.string().c_str());
	CImg<unsigned char> Coastlines(CoastlinesPath.string().c_str());

	boost::filesystem::path HoiProvinceMapPath = Hoi3ModPath / "map" / "provinces.bmp";
	if (false == boost::filesystem::exists(HoiProvinceMapPath))
	{
		HoiProvinceMapPath = Hoi3Path / "map" / "provinces.bmp";
	}
	CImg<unsigned char> ProvinceMapFromFile(HoiProvinceMapPath.string().c_str());
	ProvinceMapFromFile.resize(512, 200, 1, 3, 1);
	ProvinceMapFromFile.mirror('y');

	CImg<unsigned char> ProvinceMap(512, 285, 1, 3, 0);
	ProvinceMap.draw_image(0, 26, ProvinceMapFromFile);

	/*
	CImgDisplay show(ProvinceMap, "ProvinceMap");

	while (!show.is_closed())
		show.wait();
	*/
	
	std::vector< std::tuple< std::string, CImg<unsigned char>, CImg<unsigned char> > > TerritoryMaps;
	std::vector< std::string > RelevantTags;
	for (int i = 0; i < 6; i++)
	{
		std::string sRelevant = SortedTerritoryCounts[i].first;
		TerritoryMaps.push_back(
			std::tuple< std::string, CImg<unsigned char>, CImg<unsigned char> >
			(sRelevant, CImg<unsigned char>(512, 285, 1, 3, 0), CImg<unsigned char>(512, 285, 1, 3, 0))
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
				if (std::get<0>(TerritoryMaps[i]) == sTag)
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

	std::vector<boost::filesystem::path> ContinentPaths = {
		OutputPath / "data" / "earth" / "africa.bmp",
		OutputPath / "data" / "earth" / "europe.bmp",
		OutputPath / "data" / "earth" / "northamerica.bmp",
		OutputPath / "data" / "earth" / "russia.bmp",
		OutputPath / "data" / "earth" / "southamerica.bmp",
		OutputPath / "data" / "earth" / "southasia.bmp"
	};

	for (int i = 0; i < 6; i++)
	{
		/*
		CImgDisplay show2(std::get<1>(TerritoryMaps[i]), "ProvinceMap");

		while (!show2.is_closed())
			show2.wait();
		*/
		std::get<1>(TerritoryMaps[i]).blur(1);
		std::get<1>(TerritoryMaps[i]).save(ContinentPaths[i].string().c_str());
	}

	///


	boost::filesystem::path BigHoiProvinceMapPath = Hoi3ModPath / "map" / "provinces.bmp";
	if (false == boost::filesystem::exists(BigHoiProvinceMapPath))
	{
		BigHoiProvinceMapPath = Hoi3Path / "map" / "provinces.bmp";
	}
	CImg<unsigned char> BigProvinceMap(BigHoiProvinceMapPath.string().c_str());
	
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

	std::cout << BigProvinceMap.width() << "\t" << BigProvinceMap.height() << std::endl;

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

			std::cout << x << "\t" << y << std::endl;
			const unsigned char newcolour[] = { 
				BigProvinceMap(x, y, 0, 0) + (10.0 * real_rand()) - 5.0, 
				BigProvinceMap(x, y, 0, 1) + (10.0 * real_rand()) - 5.0,
				BigProvinceMap(x, y, 0, 2) + (10.0 * real_rand()) - 5.0
			};
			BigProvinceMap.draw_fill(x, y, newcolour);
		}
	}

	//

	GetEdges Edges;
	Edges.Init(&BigProvinceMap);

	std::set< std::tuple<int, int, int> > GotColours;
	Edges.GetAllColours(GotColours);

	boost::filesystem::path InternationalDatPath = OutputPath / "data" / "earth" / "international.dat";
	std::ofstream InternationalDat(InternationalDatPath.string().c_str());

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

	return 0;
}

