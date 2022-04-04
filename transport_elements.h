#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <set>

#include <memory>

#include <iostream>

#include "routes.h"
#include "json.h"

struct TransportElement {
	const std::string name;
	const int id;
	
	TransportElement(std::string s, int unique_number)
		: name(s), id(unique_number) {}
};


struct BusStop : public TransportElement {
	SphereCoordinates coords;
	
	std::unordered_map<
		std::shared_ptr<BusStop>, 
		RouteVirtualAndRealLength
	> length_to_stop;
	std::set<std::string_view> buses;
	
	BusStop(std::string s, int unique_number) 
		: TransportElement(s, unique_number) {}
};

std::ostream& operator<<(std::ostream&, const BusStop&);


struct BusRoute : public TransportElement {
	std::vector<std::shared_ptr<BusStop>> stops;
	
	int length = 0;
	double curvature = 0;
	
	int stop_count = 0;
	int unique_stop_count = 0;
	bool circled = false;
	
	BusRoute(std::string s, int unique_number) 
		: TransportElement(s, unique_number) {}
};

std::ostream& operator<<(std::ostream&, const BusRoute&);


Json::Node RouteToJsonNode(const int, const std::shared_ptr<BusRoute>& = nullptr);

Json::Node StopsToJsonNode(const int, const std::shared_ptr<BusStop>& = nullptr);

