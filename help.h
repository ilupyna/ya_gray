#pragma once

#include <string_view>
#include <iostream>
#include <optional>
#include <vector>
#include <set>

#include <sstream>

std::pair<std::string_view, std::optional<std::string_view>> SplitTwoStrict(std::string_view, std::string_view = " ");
std::pair<std::string_view, std::string_view> SplitTwo(std::string_view, std::string_view = " ");
std::string_view ReadToken(std::string_view&, std::string_view = " ");

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	os<<"{";
	for(const auto& el:v) {
		os<< el <<",";
	}
	os<<"}";
	return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& v) {
//	os<<"{";
	for(const auto& el:v) {
		os<<" "<< el;
	}
//	os<<"}";
	return os;
}

std::vector<std::string> GetNLinesFromStream(std::istream&);
