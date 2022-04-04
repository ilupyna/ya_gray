#include "requests.h"

#include <unordered_map>
#include <string_view>

const std::unordered_map<std::string_view, InputRequest::Type> STR_TO_INP_REQUEST_TYPE = {
	{"Stop", InputRequest::Type::Stop},
	{"Bus", InputRequest::Type::Bus}
};

InputRequest::Type ConvertInputRequestTypeFromString(const std::string_view type_str) {
//	cout<<"InputReq:"<< type_str <<endl;
	if (const auto it = STR_TO_INP_REQUEST_TYPE.find(type_str);
		it != STR_TO_INP_REQUEST_TYPE.end()) 
	{
		return it->second;
	}
	return InputRequest::Type::Error;
}

InputRequest::InputRequest(const Json::Node& node) 
	: type(ConvertInputRequestTypeFromString(node.AsMap().at("type").AsString()))
{}


const std::unordered_map<std::string_view, OutputRequest::Type> STR_TO_OUT_REQUEST_TYPE = {
	{"Bus", OutputRequest::Type::ShowRoute},
	{"Stop", OutputRequest::Type::ShowBuses},
	{"Route", OutputRequest::Type::FindRoute}
};

OutputRequest::Type ConvertOutputRequestTypeFromString(const std::string_view type_str) {
//	cout<<"OutputReq:"<< type_str <<endl;
	if (const auto it = STR_TO_OUT_REQUEST_TYPE.find(type_str);
		it != STR_TO_OUT_REQUEST_TYPE.end()) 
	{
		return it->second;
	}
	return OutputRequest::Type::Error;
}

OutputRequest::OutputRequest(const Json::Node& node) 
	: type(ConvertOutputRequestTypeFromString(node.AsMap().at("type").AsString())),
		id(node.AsMap().at("id").AsInt())
{
	const auto& node_map = node.AsMap();
	
	if(type == OutputRequest::Type::FindRoute) {
		from = node_map.at("from").AsString();
		to = node_map.at("to").AsString();
	}
	else {
		name = node_map.at("name").AsString();
	}
}

