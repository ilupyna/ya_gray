#pragma once

#include <string>

#include "json.h"


struct InputRequest {
	enum struct Type {
		Stop,
		Bus,
		Error
	};
	const Type type;
	
	InputRequest(const Json::Node&);
};


struct OutputRequest {
	enum struct Type {
		ShowRoute,
		ShowBuses,
		FindRoute,
		Error
	};
	const Type type;
	const int id;
	
	std::string name;
	std::string from, to;
	
	OutputRequest(const Json::Node&);
};
