#pragma once

#include <iostream>
#include <variant>
#include <map>
#include <string>
#include <vector>

namespace Json {
	class Node : std::variant<
		std::vector<Node>,
		std::map<std::string, Node>,
		std::string,
		int,
		double,
		size_t,
		bool
	> {
	public:
		using variant::variant;
		
		const variant& GetBase() const {
			return *this;
		}
	
		const auto& AsArray() const {
			return std::get<std::vector<Node>>(*this);
		}
		const auto& AsMap() const {
			return std::get<std::map<std::string, Node>>(*this);
		}
		const auto& AsString() const {
			return std::get<std::string>(*this);
		}
		int AsInt() const {
			return std::get<int>(*this);
		}
		
		double AsDouble() const {
			return std::holds_alternative<double>(*this) ?
				std::get<double>(*this) : std::get<int>(*this);
		}
		bool AsBool() const {
			return std::get<bool>(*this);
		}
	};
	
	std::ostream& operator<<(std::ostream&, const Node&);
	bool operator==(const Node&, const Node&);
	
	class Document {
	public:
		explicit Document(Node root);
		
		const Node& GetRoot() const;
		
	private:
		Node root;
	};
	
	Document Load(std::istream& input);
}
