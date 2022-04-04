#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <algorithm> // vector find
#include <memory>
#include <fstream>
#include <sstream>

#include "transport_elements.h"
#include "routes.h"
#include "requests.h"

#include "json.h"
#include "router.h"
#include "graph.h"


using namespace std;


struct RoutingSettings {
	int bus_wait_time = 0;
	int bus_velocity = 0;
};


struct CanBeJsonNode {
	virtual Json::Node AsNode() const = 0;
};


using JsonMap = map<string, Json::Node>;
using JsonArr = vector<Json::Node>;


struct WaitInRoute : public CanBeJsonNode {
	string_view stop_name;
	int time;
	
	Json::Node AsNode() const override {
		JsonMap node_map;
		node_map["type"] = "Wait"s;
		node_map["stop_name"] = string(stop_name);
		node_map["time"] = time;
		return node_map;
	}
};

struct BusInRoute : public CanBeJsonNode {
	string_view bus_name;
	int stops_count;
	double time;
	
	Json::Node AsNode() const override {
		JsonMap node_map;
		node_map["type"] = "Bus"s;
		node_map["bus"] = string(bus_name);
		node_map["span_count"] = stops_count;
		node_map["time"] = time;
		return node_map;
	}
};


struct FoundRoute : public CanBeJsonNode {
	double time;
	vector<shared_ptr<CanBeJsonNode>> items;
	
	void Add(shared_ptr<CanBeJsonNode> smth) {
		items.push_back(move(smth));
	}
	void Add(const WaitInRoute& w) {
		items.push_back(make_shared<WaitInRoute>(w));
	}
	void Add(const BusInRoute& b) {
		items.push_back(make_shared<BusInRoute>(b));
	}
	
	Json::Node AsNode() const override {
		JsonArr items_in_nodes;
		items_in_nodes.reserve(items.size());
		for(const auto& item_ptr : items) {
			items_in_nodes.push_back(item_ptr->AsNode());
		}
		return items_in_nodes;
	}
};

Json::Node FoundRouteToJsonNode(
	const int request_id,
	const shared_ptr<FoundRoute>& route = nullptr 
) {
	JsonMap json_map;
	json_map["request_id"] = request_id;
	
	if(route) {
		json_map["total_time"] = route->time;
		json_map["items"] = route->AsNode();
	}
	else {
		json_map["error_message"] = "not found"s;
	}
	return json_map;
}


auto GetRouteLength(
	const shared_ptr<BusStop>& lhs, 
	const shared_ptr<BusStop>& rhs
) {
//	cout<<"l:"<< lhs->name <<"; r:"<< rhs->name <<endl;
	if( auto lhs_to_rhs_it = lhs->length_to_stop.find(rhs);
		lhs_to_rhs_it != lhs->length_to_stop.end() ) 
	{
		ComputeStopsOnEarthDistance(
			lhs_to_rhs_it->second, 
			lhs->coords, 
			rhs->coords
		);
		return lhs_to_rhs_it->second;
	}
	return GetRouteLength(rhs, lhs);
}


auto GetRouteTime(
	const shared_ptr<BusStop>& lhs, 
	const shared_ptr<BusStop>& rhs,
	const int velocity
) {
	if( auto lhs_to_rhs_it = lhs->length_to_stop.find(rhs);
		lhs_to_rhs_it != lhs->length_to_stop.end() ) 
	{
		lhs_to_rhs_it->second.GetTimeByVelocity(velocity);
		return lhs_to_rhs_it->second;
	}
	return GetRouteTime(rhs, lhs, velocity);
}


class TransportManager {
	unordered_map<string_view, shared_ptr<BusStop>> stops;
	unordered_map<string_view, shared_ptr<BusRoute>> routes;
	RoutingSettings routing_settings;
	
//	bool router_is_outdated = true;
	unique_ptr<Graph::DirectedWeightedGraph<double>> graph_of_routes = nullptr;
	unique_ptr<Graph::Router<double>> router = nullptr;
	FoundRoute found_routes;	// readable items for Router
	
	Json::Node GetRouteInNode(const string_view bus_id, const int req_id) const {
		if( const auto it = routes.find(bus_id);
			it != routes.end() ) 
		{
			return RouteToJsonNode(req_id, it->second);
		}
		return RouteToJsonNode(req_id);
	}
	
	Json::Node GetBusesInNode(const string_view stop_id, const int req_id) const {
		if( const auto it = stops.find(stop_id);
			it != stops.end() ) 
		{
			return StopsToJsonNode(req_id, it->second);
		}
		return StopsToJsonNode(req_id);
	}
	
	Json::Node GetFoundRouteInNode(
		const string_view stop_from, 
		const string_view stop_to, 
		const int req_id 
	) const {
		const auto start_stop_it = stops.find(stop_from);
		const auto finish_stop_it = stops.find(stop_to);
		if( start_stop_it == stops.end() || 
			finish_stop_it == stops.end() ) 
		{
			return FoundRouteToJsonNode(req_id);
		}
		
		auto found_route = router->BuildRoute(
			start_stop_it->second->id,
			finish_stop_it->second->id
		);
		
		if(found_route) {
			auto found_route_ptr = make_shared<FoundRoute>();
			found_route_ptr->time = found_route->weight;
			
			const auto route_id = found_route->id;
			for (size_t i = 0; i < found_route->edge_count; i++) {
				size_t edgeId = router->GetRouteEdge(route_id, i);
				found_route_ptr->Add(found_routes.items.at(edgeId));
			}
			
			return FoundRouteToJsonNode(req_id, found_route_ptr);
		}
		return FoundRouteToJsonNode(req_id);
	}
	
	
	void CalculateVelocityOnRoutes() {
		for(auto& [stop_name, stop_ptr] : stops) {
			for(auto& [another_stop_ptr, length] : stop_ptr->length_to_stop) {
				length.GetTimeByVelocity(routing_settings.bus_velocity);
			}
		}
	}
	
	void CalculateRoutesLengthAndCurvature() {
		for(auto& [route_id, route_ptr] : routes) {
			auto& stops_in_route = route_ptr->stops;
			RouteVirtualAndRealLength route_length = {0, 0};
			
			for(size_t i = 1; i < stops_in_route.size(); i++) {
				route_length += GetRouteLength(
					stops_in_route.at(i-1), 
					stops_in_route.at(i)
				);
				if(route_ptr->circled) {
					route_length += GetRouteLength(
						stops_in_route.at(i),
						stops_in_route.at(i-1)
					);
				}
			}
			route_ptr->length = route_length.length_by_road;
			route_ptr->curvature = route_length.GetCurvature();
		}
	}
	
	void FillStopsWithBuses() {
		for(auto& [bus_id, route_ptr] : routes) {
			for(auto& stop_ptr : route_ptr->stops) {
				stop_ptr->buses.insert(bus_id);
			}
		}
	}
	
	void InitializeGraphAndRouter() {
		graph_of_routes = make_unique<Graph::DirectedWeightedGraph<double>>(stops.size() * 2);
		
		for(const auto& [stop_name, stop_ptr] : stops) {	// map
			// from every STOP to it's mirror using for entering the BUS
			graph_of_routes->AddEdge({	// Graph::Edge<double>{
				.from = static_cast<Graph::VertexId>(stop_ptr->id), 
				.to = static_cast<Graph::VertexId>(stop_ptr->id + 1), 
				.weight = static_cast<double>(routing_settings.bus_wait_time)
			});
			
			WaitInRoute wait_item;
			wait_item.stop_name = stop_name;
			wait_item.time = routing_settings.bus_wait_time;
			found_routes.Add(wait_item);
		}
		
		// from every STOP's mirror to another STOP on every ROUTE
		for(const auto& [bus_name, bus_ptr] : routes) {	// map
			BusInRoute bus_item;
			bus_item.bus_name = bus_name;
			
			const auto& stops_vec = bus_ptr->stops;	// vector stops from route
			for(size_t i = 0; i < stops_vec.size() - 1; i++) {
				double time = 0;
				
				for(size_t j = i + 1; j < stops_vec.size(); j++) {
					// Calc time from previous stop
					time += GetRouteTime(
						stops_vec.at(j-1), 
						stops_vec.at(j), 
						routing_settings.bus_velocity 
					).time_to_go.value();
					
					graph_of_routes->AddEdge({
						.from = static_cast<Graph::VertexId>(stops_vec.at(i)->id + 1), 
						.to = static_cast<Graph::VertexId>(stops_vec.at(j)->id), 
						.weight = time
					});
					
					bus_item.time = time;
					bus_item.stops_count = j - i;
					found_routes.Add(bus_item);
				}
			}
			
			if(!bus_ptr->circled) {
				// don't wanna break the \t structure
				continue;
			}
			
			// a bit of code copying
			for(int i = stops_vec.size() - 1; i >= 0 ; i--) {
				double time = 0;
				
				for(int j = i - 1; j >= 0 ; j--) {
					// Calc time from previous stop
					time += GetRouteTime(
						stops_vec.at(j+1), 
						stops_vec.at(j), 
						routing_settings.bus_velocity 
					).time_to_go.value();
					
					graph_of_routes->AddEdge({
						.from = static_cast<Graph::VertexId>(stops_vec.at(i)->id + 1), 
						.to = static_cast<Graph::VertexId>(stops_vec.at(j)->id), 
						.weight = time
					});
					
					bus_item.time = time;
					bus_item.stops_count = i - j;
					found_routes.Add(bus_item);
				}
			}
		}
		
		router = make_unique<Graph::Router<double>>(*graph_of_routes);
	}
	
	
	auto FindOrAddStop(const string& stop_name) {
		if( auto found = stops.find(stop_name);
			found != stops.end() )
		{
			return make_pair(found->second, false);
		}
		// reserve second id for STOP mirror
		const auto unique_id = stops.size() * 2; 
		auto bus_ptr = make_shared<BusStop>(stop_name, unique_id);
		
		auto [it, sucess] = stops.insert({bus_ptr->name, bus_ptr});
		return make_pair(it->second, true);
	}
	
	auto FindOrAddRoute(const string& route_id) {
		if( auto found = routes.find(route_id);
			found != routes.end() )
		{
			return make_pair(found->second, false);
		}
		// unique_id = 404 for buses (should it be 410?)
		auto route_ptr = make_shared<BusRoute>(route_id, 404); 
		
		auto [it, sucess] = routes.insert({route_ptr->name, route_ptr});
		return make_pair(it->second, true);
	}
	
	
	void InsertStop(const Json::Node& node) {
		const auto& node_map = node.AsMap();
		
		const string this_stop_name = node_map.at("name").AsString();
		auto [this_stop, is_new] = FindOrAddStop(this_stop_name);	//insertion here
		
		this_stop->coords = {
			.latitude = node_map.at("latitude").AsDouble(), 
			.longitude = node_map.at("longitude").AsDouble()
		};
		
		for(const auto& [stop_name, length_node] : node_map.at("road_distances").AsMap()) {
			const auto [stop_ptr, is_new] = FindOrAddStop(stop_name);
			this_stop->length_to_stop[stop_ptr] = {.length_by_road = length_node.AsInt()};
		}
	}
	
	void InsertBus(const Json::Node& node) {
		const auto& node_map = node.AsMap();
		
		const auto route_name = node_map.at("name").AsString();
		auto [this_route, is_new] = FindOrAddRoute(route_name);	//insertion here
		
		const bool circled = !node_map.at("is_roundtrip").AsBool();
		
		int unique_stops = 0;
		
		for(const auto& stop_name_node : node_map.at("stops").AsArray()) {
			const auto [stop_ptr, is_new] = FindOrAddStop(stop_name_node.AsString());
			
			auto& stops_ref = this_route->stops;
			// nope, no set<stops>
			if( auto found_it = find(stops_ref.begin(), stops_ref.end(), stop_ptr);
				found_it == stops_ref.end() ) 
			{
				unique_stops++;
			}
			
			stops_ref.push_back(stop_ptr);
		}
		
		this_route->circled = circled;
		this_route->stop_count = (circled) ? 
			(this_route->stops.size() * 2 - 1) : this_route->stops.size();
		this_route->unique_stop_count = unique_stops;
	}
	
	void ProcessInputReq(const Json::Node& node) {
		InputRequest req(node);
		switch(req.type) {
			case InputRequest::Type::Bus : return InsertBus(node);
			case InputRequest::Type::Stop : return InsertStop(node);
		}
	}
	
	Json::Node ProcessOutputReq(const Json::Node& node) const {
		OutputRequest req(node);
		switch(req.type) {
			case OutputRequest::Type::ShowRoute : return GetRouteInNode(req.name, req.id);
			case OutputRequest::Type::ShowBuses : return GetBusesInNode(req.name, req.id);
			case OutputRequest::Type::FindRoute : return GetFoundRouteInNode(req.from, req.to, req.id);
		}
	}
	
	
	void SetDatabaseFromJsonMap(const JsonArr& json_arr) {
		for (const auto& node : json_arr) {
			ProcessInputReq(node);
		}
	}
	
	void SetRoutingSettingsFromJsonMap(const JsonMap& json_map) {
		routing_settings = {
			.bus_wait_time = json_map.at("bus_wait_time").AsInt(),
			.bus_velocity = json_map.at("bus_velocity").AsInt()
		};
	}
	
	void ProcessRequestsToStream(const JsonArr& json_arr, ostream& os) const {
		JsonArr reply;
		for (const auto& node : json_arr) {
			reply.push_back(ProcessOutputReq(node));
		}
		os << Json::Node(reply);
	}
	
	void PrintDatabase() const {
		cout<<"Stops:"<<endl;
		for(const auto& [id, stop_prt] : stops) {
			cout<< id <<":"<< *stop_prt.get() <<endl;
		}
		cout<<endl;
		
		cout<<"Routes:"<<endl;
		for(const auto& [id, route_prt] : routes) {
			cout<< id <<":"<< *route_prt.get() <<endl;
		}
		cout<<endl<<endl;
	}
	
	void DatabaseCaclulations() {
		CalculateVelocityOnRoutes();
		CalculateRoutesLengthAndCurvature();
		FillStopsWithBuses();
//		PrintDatabase();
		InitializeGraphAndRouter();
	}
	
public:
	TransportManager(istream& is = cin, ostream& os = cout) {
		const auto doc = Json::Load(is);
		const auto& json_map = doc.GetRoot().AsMap();
		
		SetDatabaseFromJsonMap(json_map.at("base_requests").AsArray());
		SetRoutingSettingsFromJsonMap(json_map.at("routing_settings").AsMap());
		
		DatabaseCaclulations();
		
		ProcessRequestsToStream(json_map.at("stat_requests").AsArray(), os);
	}
	
//	void RequestsToOstream(istream& is = cin, ostream& os = cout) const {
//		const auto doc = Json::Load(is);
//		const auto& json_map = doc.GetRoot().AsMap();
//		
//		ProcessRequestsToStream(json_map.at("stat_requests").AsArray(),	os);
//	}
};

void CompareNodesFromStream(stringstream& lhs, stringstream& rhs) {
	const auto lhs_src = Json::Load(lhs);
	const auto rhs_src = Json::Load(rhs);
	
	if(lhs_src.GetRoot() == rhs_src.GetRoot()) {
		cout<<"Nodes are similar"<<endl;
	}
	else {
		cout<<"Nodes are different"<<endl;
	}
}

int main() {
//	stringstream ss_from_input, ss_to_output, ss_to_compare;
//	
//	if( ifstream file_inp("transport-input2.json");
//		file_inp ) 
//	{
//		ss_from_input << file_inp.rdbuf();
//		file_inp.close();
//	}
//	else 
//		return 1;
//	
//	if( ifstream file_inp("transport-output2.json");
//		file_inp )
//	{
//		ss_to_compare << file_inp.rdbuf();
//		file_inp.close();
//	}
//	else
//		return 2;
	
	TransportManager transManager(cin, cout);	// ss_from_input, ss_to_output
	
//	CompareNodesFromStream(ss_to_output, ss_to_compare);	// broken because routing variants	
//	
//	if( ofstream file_out("output.json");
//		file_out )
//	{
//		file_out << iss_to_output.rdbuf();
//		file_out.close();
//	}
//	else
//		return 2;
}
