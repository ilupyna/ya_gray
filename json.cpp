#include "json.h"

#include <iostream>

using namespace std;

namespace Json {
	
	Document::Document(Node root) : root(move(root)) {}
	
	const Node& Document::GetRoot() const {
		return root;
	}
	
	Node LoadNode(istream& input);
	
	Node LoadArray(istream& input) {
		vector<Node> result;
		
		for (char c; input >> c && c != ']'; ) {
			if (c != ',') {
				input.putback(c);
			}
			result.push_back(LoadNode(input));
		}
		
		return Node(move(result));
	}
	
	Node LoadNumber(istream& input) {
		const bool is_negative = [&] {
			if(input.peek() == '-') {
				input.get();	// '-'
				return true;
			}
			return false;
		}();
		
		int result = 0;
		while (isdigit(input.peek())) {
			result *= 10;
			result += input.get() - '0';
		}
		result *= (is_negative) ? -1 : 1;
		
		if(input.peek() == '.') {
			input.get(); // '.'
			
			double result_double = result;
			int power_minus = 1;
			while (isdigit(input.peek())) {
				power_minus *= 10;
				result_double += double(input.get() - '0') / power_minus;
			}
			return Node(result_double);
		}
		return Node(result);
	}
	
	Node LoadBool(istream& input) {
		string line;
    	while (isalpha(input.peek())) {
			line += input.get();
		}
		return (line == "true") ? 
			Node(true) : Node(false);
	}

	Node LoadString(istream& input) {
		string line;
		getline(input, line, '"');
		return Node(move(line));
	}
	
	Node LoadDict(istream& input) {
		map<string, Node> result;
		for (char c; input >> c && c != '}'; ) {
			if (c == ',') {
				input >> c;
			}
			string key = LoadString(input).AsString();
			input >> c;
			result.emplace(move(key), LoadNode(input));
		}
		return Node(move(result));
	}
	
	Node LoadNode(istream& input) {
		char c;
		input >> c;
		
		if (c == '[') {
			return LoadArray(input);
		} else if (c == '{') {
			return LoadDict(input);
		} else if (c == '"') {
			return LoadString(input);
		}
		else if(c == 't' || c == 'f') {
			input.putback(c);
			return LoadBool(input);
		}
		else {
			input.putback(c);
			return LoadNumber(input);
		}
	}

	Document Load(istream& input) {
		return Document{LoadNode(input)};
	}
	
	struct NodeToStream {
		ostream& os;
		
		void operator() (const std::vector<Node>& arr_node) {
			os<<"[";
			const auto last_it = prev(arr_node.end());
			for(auto it = arr_node.begin(); it != arr_node.end(); ++it) {
				std::visit(NodeToStream{os}, it->GetBase());
				if(it != last_it) {
					os<<", ";
				}
			}
			os<<"]";
		}
		void operator() (const std::map<std::string, Node>& map_node) {
			os<<"{";
			const auto last_it = prev(map_node.end());
			for(auto it = map_node.begin(); it != map_node.end(); ++it) {
				os<<"\""<< it->first << "\": ";
				std::visit(NodeToStream{os}, it->second.GetBase());
				if(it != last_it) {
					os<<", ";
				}
			}
			os<<"}";
		}
		void operator() (const std::string& str_node) {
			os<<"\""<< str_node << "\"";
		}
		void operator() (const bool bool_node) {
			os<< std::boolalpha << bool_node;
		}
		
		template <typename T>
		void operator() (const T& variant_option) {
			os << variant_option;
		}
	};
	
	std::ostream& operator<<(std::ostream& os, const Node& node) {
		std::visit(NodeToStream{os}, node.GetBase());
		return os;
	}
	
	bool operator==(const Node& lhs, const Node& rhs) {
		return lhs.GetBase() == rhs.GetBase();
	}
}
