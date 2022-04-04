#include "transport_elements.h"

#include <iostream>
#include <iomanip>	// setprecision

#include "routes.h"
#include "json.h"
#include "help.h"

std::ostream& operator<<(std::ostream& os, const BusStop& stop) {
	os << stop.coords <<" :"<< stop.buses <<"; ";
	for(const auto& [bus_ptr, length] : stop.length_to_stop) {
		os << bus_ptr->name <<":"<< length.length_by_road <<",";
	}
	os<<".";
	return os;
}


std::ostream& operator<<(std::ostream& os, const BusRoute& route) {
	os << route.stop_count <<" stops on route, " 
		<< route.unique_stop_count <<" unique stops, " 
		<< route.length <<" route length, "
		<< std::setprecision(6) << route.curvature <<" curvature";
	return os;
}


Json::Node RouteToJsonNode(
	const int request_id,
	const std::shared_ptr<BusRoute>& route 
) {
	std::map<std::string, Json::Node> json_map;
	json_map["request_id"] = request_id;
	
	if(route) {
		json_map["route_length"] = route->length;
		json_map["curvature"] = route->curvature;
		json_map["stop_count"] = route->stop_count;
		json_map["unique_stop_count"] = route->unique_stop_count;
	}
	else {
		json_map["error_message"] = "not found";
	}
	return json_map;
}


Json::Node StopsToJsonNode(
	const int request_id,
	const std::shared_ptr<BusStop>& stops 
) {
	std::map<std::string, Json::Node> json_map;
	json_map["request_id"] = request_id;
	
	if(stops) {
		std::vector<Json::Node> buses_submap;
		if(!stops->buses.empty()) {
			buses_submap.reserve(stops->buses.size());
			for(const auto& bus_name : stops->buses) {
				buses_submap.push_back(std::string(bus_name));
			}
		}
		json_map["buses"] = buses_submap;
	}
	else {
		json_map["error_message"] = "not found";
	}
	return json_map;
}
