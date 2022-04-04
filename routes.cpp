#include "routes.h"

#include <cmath>
#include <iostream>

double RouteVirtualAndRealLength::GetTimeByVelocity(const int velocity) {
	if(!time_to_go) {
		constexpr double KMperH_to_MperSEC_multiplier = 1000. / 60.;
		time_to_go = static_cast<double>(length_by_road) / 
			(static_cast<double>(velocity) * KMperH_to_MperSEC_multiplier);
	}
	return time_to_go.value();
}

double RouteVirtualAndRealLength::GetCurvature() const {
	return length_by_road / length_on_earth.value();
}

std::ostream& operator<<(std::ostream& os, const SphereCoordinates& c) {
	os<<"{"<< c.latitude <<","<< c.longitude <<"}";
	return os;
}


double CalcDistanceFromCoordinates(
	double lhs_lat, 
	double lhs_lon, 
	double rhs_lat, 
	double rhs_lon 
) {
	const static double PI = 3.1415926535;
	const static double RAD_TO_DEG_MULTIPLIER = PI/180.;
	const static double EARTH_RADIUS_IN_M = 6371000;
	
	rhs_lat *= RAD_TO_DEG_MULTIPLIER;
	rhs_lon *= RAD_TO_DEG_MULTIPLIER;
	lhs_lat *= RAD_TO_DEG_MULTIPLIER;
	lhs_lon *= RAD_TO_DEG_MULTIPLIER;
	
	return acos(
		sin(lhs_lat) * sin(rhs_lat) +
		cos(lhs_lat) * cos(rhs_lat) * cos(std::abs(lhs_lon - rhs_lon)) 
	) * EARTH_RADIUS_IN_M;
}


void ComputeStopsOnEarthDistance(
	RouteVirtualAndRealLength& route_length, 
	const SphereCoordinates& lhs, 
	const SphereCoordinates& rhs
) {
	if(!route_length.length_on_earth) {
		route_length.length_on_earth = CalcDistanceFromCoordinates( 
			lhs.latitude, 
			lhs.longitude, 
			rhs.latitude, 
			rhs.longitude
		);
	}
}

