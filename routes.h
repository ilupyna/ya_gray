#pragma once

#include <optional>
#include <iostream>

struct RouteVirtualAndRealLength {
	int length_by_road = 0;
	
	std::optional<double> time_to_go = std::nullopt;
	std::optional<double> length_on_earth = std::nullopt;
	
	double GetTimeByVelocity(const int);
	double GetCurvature() const;
	
	RouteVirtualAndRealLength& operator+=(const RouteVirtualAndRealLength& other) {
		if(other.length_on_earth) {
			if(length_on_earth) {
				length_on_earth.value() += other.length_on_earth.value();
			}
			else {
				length_on_earth = other.length_on_earth.value();
			}
		}
		length_by_road += other.length_by_road;
		time_to_go.reset();
		return *this;
	}
	
	template <typename T>
	RouteVirtualAndRealLength& operator*=(const T multiplier) {
		if(length_on_earth) {
			length_on_earth.value() *= multiplier;
		}
		length_by_road *= multiplier;
		time_to_go.reset();
		return *this;
	}
};


struct SphereCoordinates {
	double latitude = 0;
	double longitude = 0;
};

std::ostream& operator<<(std::ostream&, const SphereCoordinates&);

double CalcDistanceFromCoordinates(double, double, double, double);

void ComputeStopsOnEarthDistance(RouteVirtualAndRealLength&, const SphereCoordinates&, const SphereCoordinates&);

