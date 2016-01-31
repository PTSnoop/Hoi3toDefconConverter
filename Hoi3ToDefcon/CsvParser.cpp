//#define BOOST_SPIRIT_DEBUG
//#include <boost/spirit/include/qi.hpp>


#include <vector>
#include <string>
#include <fstream>
#include <streambuf>

#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/string/split.hpp>

/*

namespace qi = boost::spirit::qi;

using Column = std::string;
using Columns = std::vector<Column>;
using CsvLine = Columns;
using CsvFile = std::vector<CsvLine>;

template <typename It>
struct CsvGrammar : qi::grammar<It, CsvFile(), qi::blank_type>
{
	CsvGrammar() : CsvGrammar::base_type(start)
	{
		using namespace qi;

		static const char colsep = ';';

		start = -line % eol;
		line = column % colsep;
		column = quoted | *~char_(colsep);
		quoted = '"' >> *("\"\"" | ~char_('"')) >> '"';

		BOOST_SPIRIT_DEBUG_NODES((start)(line)(column)(quoted));
	}
private:
	qi::rule<It, CsvFile(), qi::blank_type> start;
	qi::rule<It, CsvLine(), qi::blank_type> line;
	qi::rule<It, Column(), qi::blank_type> column;
	qi::rule<It, std::string()> quoted;
};

bool ParseCsv(std::vector< std::vector<std::string> >& FileData, std::string sInText)
{
	CsvGrammar<std::string::const_iterator> Grammar;
	return qi::phrase_parse(sInText.begin(), sInText.end(), Grammar, qi::blank, FileData);
}
*/

bool ParseCsv(std::vector< std::vector<std::string> >& FileData, std::string sInText)
{
	std::vector<std::string> Lines;
	boost::split(Lines, sInText, boost::is_any_of("\r\n"));
	for (std::string sLine : Lines)
	{
		std::vector<std::string> Vars;
		boost::split(Vars, sLine, boost::is_any_of(",;"));
		FileData.push_back(Vars);
	}
	return true;
}

bool ParseCsvFile(std::vector< std::vector<std::string> >& FileData, std::string sPath)
{
	std::ifstream Filestream (sPath);
	std::string sRawFileData ((std::istreambuf_iterator<char>(Filestream)), std::istreambuf_iterator<char>());
	return ParseCsv(FileData, sRawFileData);
}
