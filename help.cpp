#include "help.h"

#include <string_view>
#include <iostream>
#include <optional>
#include <vector>

std::pair<std::string_view, std::optional<std::string_view>> SplitTwoStrict(std::string_view s, std::string_view delimiter) {
	const size_t pos = s.find(delimiter);
	if (pos == s.npos) {
		return {s, std::nullopt};
	}
	else {
		return {s.substr(0, pos), s.substr(pos + delimiter.length())};
	}
}

std::pair<std::string_view, std::string_view> SplitTwo(std::string_view s, std::string_view delimiter) {
	const auto [lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
	return {lhs, rhs_opt.value_or("")};
}

std::string_view ReadToken(std::string_view& s, std::string_view delimiter) {
	const auto [lhs, rhs] = SplitTwo(s, delimiter);
	s = rhs;
	return lhs;
}

std::vector<std::string> GetNLinesFromStream(std::istream& is) {
	size_t Q;
	is >> Q;
	{
		std::string _;
		getline(is, _);
	}
	std::vector<std::string> parsed_elements(Q);
	for(size_t i = 0; i < Q; i++) {
		std::string line;
		std::getline(is, line);
		parsed_elements[i] = std::move(line);
	}
	return parsed_elements;
}
