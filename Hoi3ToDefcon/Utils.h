#pragma once

#include <tuple>
#include <string>

#include <boost/filesystem.hpp>

typedef std::tuple<unsigned char, unsigned char, unsigned char> ColourTriplet;

bool is_number(const std::string& s);

bool copyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination);