#include "Database.hpp"
#include "operator/Tablescan.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/Selection.hpp"
#include "operator/Projection.hpp"
#include "operator/Distinct.hpp"
#include "operator/Printer.hpp"
#include "operator/Chi.hpp"
#include "operator/HashJoin.hpp"
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>
#include <set>
#include <assert.h>

using namespace std;

vector<string> split_match(string str, string match) {
  /* Extract regex tokens from a string */
  regex rx_match("(" + match + ")", regex_constants::ECMAScript | regex_constants::icase);
  smatch matches;
  vector<string> splits;
  
  while (str != "") {
    regex_search(str, matches, rx_match,  regex_constants::match_flag_type::match_continuous);
    if (!matches[1].length()) {
      str = str.substr(1, str.length() -1);
    } else {
      splits.push_back(matches[1]);
      str = str.substr(matches[1].length(), str.length() - matches[1].length());  
    }
  }
  return splits;
}

struct Attr {
  string name;	// attribute name
  string alias;	// alias for the relation
  
  Attr(string name, string alias) : name(name), alias(alias) {};
  
  Attr(string alias_with_attribute) {
    auto tokens = split_match(alias_with_attribute, "\\w+");
    alias = tokens[0];
    name = tokens[1];
  };
  
  string str() {
    return alias + "." + name;
  }
  
  bool operator<(const Attr& a2) const
  {
    return (alias + name) < (a2.alias + a2.name);
  }
};

struct Constant_Binding {
  Attr attr;
  string constant;
  
  Constant_Binding(Attr attr, string constant) : attr(attr), constant(constant) {};
  
  bool operator<(const Constant_Binding& other) const
  {
    return (attr.alias + attr.name + constant) < (other.attr.alias + other.attr.name + other.constant);
  }
};

struct Attr_Binding {
  Attr attr1;
  Attr attr2;
  
  Attr_Binding(Attr attr1, Attr attr2) : attr1(attr1), attr2(attr2) {};
  
  bool operator<(const Attr_Binding& other) const
  {
    return (attr1.alias + attr1.name + attr2.alias + attr2.name) < (other.attr1.alias + other.attr1.name + other.attr2.alias + other.attr2.name);
  }
  
  string str() {
    return attr1.str() + "=" + attr2.str(); 
  }
};

struct Relation_Binding {
  string r1;
  string r2;
  
  Relation_Binding(string r1, string r2) : r1(r1), r2(r2) {};
  
  bool operator<(const Relation_Binding& other) const
  {
    return (r1+r2) < (other.r1+other.r2);
  }
};

struct Parser_Result {
  set<string> relations;
  set<string> aliases;
  set<Attr> attributes;
  set<Attr> selected_attributes;
  map<string, string> alias_to_relation;  // maps an alias to its relation
  set<Attr_Binding> attr_bindings;  // list of (attribute; attribute)
  set<Relation_Binding> relation_bindings;  // list of (relation; relation) for query graph construction
  set<Constant_Binding> constant_bindings;  // list of (attribute; constant)
  
  Parser_Result() {}
  
  void add_selected_attribute(Attr attr) {
   attributes.insert(attr);
   selected_attributes.insert(attr);
  }
  
  void add_alias_to_relation(string alias, string relation) {
    aliases.insert(alias);
    relations.insert(relation);
    alias_to_relation[alias] = relation;
  }
  
  void add_attr_binding(Attr_Binding attr_binding) {
   attributes.insert(attr_binding.attr1);
   attributes.insert(attr_binding.attr2);
   
   attr_bindings.insert(attr_binding); 
   attr_bindings.insert(Attr_Binding(attr_binding.attr2, attr_binding.attr1)); // add both directions
   
   relation_bindings.insert(move(Relation_Binding(alias_to_relation[attr_binding.attr1.alias], alias_to_relation[attr_binding.attr2.alias]))); 
   relation_bindings.insert(move(Relation_Binding(alias_to_relation[attr_binding.attr2.alias], alias_to_relation[attr_binding.attr1.alias]))); 
  }
  
  void add_constant_binding(Constant_Binding constant_binding) {
   attributes.insert(constant_binding.attr);
   constant_bindings.insert(constant_binding); 
  }
};

void extract_selected_attributes(string& select_clause, Parser_Result& result) {
  /* Extracts a list of attributes from the select_clause */
  if (select_clause == "*") return;
  
  auto aliases_with_attribute = split_match(select_clause, "(\\w|\\.)+");  // list of alias.attribute
  
  cout << "Selected attributes: ";
  for (auto alias_with_attribute : aliases_with_attribute) {
    auto attr = Attr(alias_with_attribute);
    cout << attr.str() << ", ";
    result.add_selected_attribute(attr);
  }
  cout << endl;
}

void extract_alias_relation_mappings(string& from_clause, Parser_Result& result) {
  /* Extracts the mapping alias->relation from the from_clause */
  auto tokens = split_match(from_clause, "\\w+");
  
  cout << "Alias->Relation mappings: ";
  for (unsigned i=0; i<tokens.size(); i+=2) {
    cout << tokens[i+1] << "->" << tokens[i] << ", ";
    result.add_alias_to_relation(tokens[i+1], tokens[i]);
  }
  cout << endl;
}

void extract_bindings(string& where_clause, Parser_Result& result) {
  /* Extracts the bindings from the where_clause */
  auto tokens = split_match(where_clause, "(\\w|\\.)+");
  
  cout << "Bindings: ";
  for (unsigned i=0; i<tokens.size(); i+=3) {
    auto attr1 = Attr(tokens[i]);
    cout << attr1.str() << "=";

    if (tokens[i+1].find(".")!=string::npos) {
      // attribute binding
      auto attr2 = Attr(tokens[i+1]);
      cout << attr2.str() << ", ";
      result.add_attr_binding(Attr_Binding(attr1, attr2));
    } else {
     // constant binding
      cout << tokens[i+1];
      result.add_constant_binding(Constant_Binding(attr1, tokens[i+1]));
    }
  }
  cout << endl;
}

Parser_Result parser(string query) {
  Parser_Result result;
  // regex definitions
  regex rx_sql( "select (\\*|\\w+\\.\\w+(?:,\\w+\\.\\w+)*) from (\\w+ \\w+(?:,\\w+ \\w+)*) where (\\w+\\.\\w+=(?:\\w+\\.\\w+|\\w+)(?: and \\w+\\.\\w+=(?:\\w+\\.\\w+|\\w+))*)",
			regex_constants::ECMAScript | regex_constants::icase);
  smatch matches;
  assert(regex_search(query, matches, rx_sql,  regex_constants::match_flag_type::match_continuous));
  
  // extract main parts of the sql query
  string select_clause = matches[1];
  string from_clause = matches[2];
  string where_clause = matches[3];
  
  // extract selected attributes
  extract_selected_attributes(select_clause, result);
  
  // extract alias->relation mappings
  extract_alias_relation_mappings(from_clause, result);
  
  // extract bindings
  extract_bindings(where_clause, result);
    
  // check if relations and attributes exist
  Database db;
  db.open("data/uni");
  
  for (auto& relation : result.relations) {
      assert(db.hasTable(relation));  // make sure relations exist
  }
  
  for (auto& attr : result.attributes) {
      auto table = db.getTable(result.alias_to_relation[attr.alias]);
      assert(table.findAttribute(attr.name) > -1);  // make sure attributes exist
  }
  
  cout << endl << "***** Sucessfully parsed the following query: *****" << endl << query << endl;
  
  return result;
}

void run_query(Parser_Result parser_result) {
  /* Run a query in form of Parser_Result on tinydb */
  
  cout << endl << "***** Running Query: *****" << endl;
  
  // Open Database
  Database db;
  db.open("data/uni");
  
  // Create TableScans
  map<string, unique_ptr<Tablescan>> alias_to_tables;
  for (string alias : parser_result.aliases) {
    alias_to_tables[alias] = unique_ptr<Tablescan> (new Tablescan(db.getTable(parser_result.alias_to_relation[alias])));
  }
  
  // Create Registers
  map<Attr, const Register*> attr_to_register;
  for (Attr attr : parser_result.attributes) {
    attr_to_register[attr] = alias_to_tables[attr.alias]->getOutput(attr.name);
  }
  
  // Constant Bindings
  map<string, unique_ptr<Operator>> alias_to_filtered_tables;  // basically convert from Tablescan-map to Operator-map
  for (auto& alias_table : alias_to_tables) {
    alias_to_filtered_tables[alias_table.first] = move(alias_table.second);
  }
  
  map<string, Register> equal_constant_registers;
  for (auto& binding : parser_result.constant_bindings) {
    equal_constant_registers[binding.constant] = Register(binding.constant);
    unique_ptr<Chi> filter(new Chi(move(alias_to_filtered_tables[binding.attr.alias]),Chi::Equal,attr_to_register[binding.attr],&equal_constant_registers[binding.constant]));
    const Register* filtered_register=filter->getResult();
    alias_to_filtered_tables[binding.attr.alias] = unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
  }
  
  // Canonical CrossProduct
  unique_ptr<Operator> result;
  for (auto& alias_table : alias_to_filtered_tables) {
    if (!result) {
     result = move(alias_table.second);  // initialize
    } else {
     unique_ptr<Operator> tmp(new CrossProduct(move(alias_table.second), move(result)));
     result = move(tmp);
    }
  }
  
  // Canonical Selection (join bindings)
  for (auto& binding : parser_result.attr_bindings) {
    unique_ptr<Chi> filter(new Chi(move(result),Chi::Equal,attr_to_register[binding.attr1],attr_to_register[binding.attr2]));
    const Register* filtered_register=filter->getResult();
    result = move(unique_ptr<Selection> (new Selection(move(filter),filtered_register)));  
  }
  
  
  // Projections
  vector<const Register*> selected_registers;
  for (auto attr : parser_result.selected_attributes) {
      selected_registers.push_back(attr_to_register[attr]);
  }
  Printer out (move(result), selected_registers);
  
  // Print Result
  out.open();
  while (out.next());
  out.close();
}

void build_query_graph(Parser_Result parser_result) {
  /* Run a query in form of Parser_Result on tinydb */
  
  cout << endl << "***** Building Query Graph: *****" << endl;
  
  // Open Database
  Database db;
  db.open("data/uni");
  
  set<string> nodes = parser_result.relations;
  set<Relation_Binding> edges = parser_result.relation_bindings;
  
  // Output Connected Subgraphs
  cout << endl << "***** Connected Subgraphs: *****" << endl;
  set<set<string>> connected_subgraphs;
  for (auto relation : nodes) {
    connected_subgraphs.insert({relation}); // initialize subgraphs with single nodes
  }
  set<Relation_Binding> passed_edges;
  for (auto relation_binding : edges) {
    if (passed_edges.find(relation_binding) != passed_edges.end()) continue;	 // check if edge already processed
    passed_edges.insert(Relation_Binding(relation_binding.r1, relation_binding.r2));
    passed_edges.insert(Relation_Binding(relation_binding.r2, relation_binding.r1));
    
    // merge subgraphs
    set<string> subgraph1;
    set<string> subgraph2;
    for (auto& subgraph : connected_subgraphs) {
      if (subgraph.find(relation_binding.r1) != subgraph.end()) {
	subgraph1 = subgraph;
      }
      if (subgraph.find(relation_binding.r2) != subgraph.end()) {
	subgraph2 = subgraph;
      }
    }
    set<string> merged_graph = subgraph1;
    merged_graph.insert(subgraph2.begin(), subgraph2.end());
    connected_subgraphs.insert(move(merged_graph));
    connected_subgraphs.erase(subgraph2);
    connected_subgraphs.erase(subgraph1);
  }
  
  for (auto subgraph : connected_subgraphs) {
    for (auto relation : subgraph) {
      cout << relation << ", ";
    }
    cout << endl;
  }
  
  // Output constant bindings
  cout << endl << "***** Bindings that can be pushed down and result cardinalities: *****" << endl;
  for (auto constant_binding : parser_result.constant_bindings) {
   auto t1 = db.getTable(parser_result.alias_to_relation[constant_binding.attr.alias]); 
   auto a1 = t1.getAttribute(constant_binding.attr.name); 
   cout << parser_result.alias_to_relation[constant_binding.attr.alias] << "." << constant_binding.attr.name << "=" << constant_binding.constant << ": ";
   if (a1.getKey()) {
     cout << 1.0;  // A Key yields a cardinality of 1 (or 0)
   } else {
     cout << 1.0 / a1.getUniqueValues() * t1.getCardinality();  // Selectivity times Cardinality yields Result Cardinality of a Selection
   }
   cout << endl;
  }
  
  // Output cardinalities
  cout << endl << "***** Estimated Selectivities *****" << endl;
  for (auto attr_binding : parser_result.attr_bindings) {
    cout << attr_binding.str() << ": ";
    auto t1 = db.getTable(parser_result.alias_to_relation[attr_binding.attr1.alias]); 
    auto t2 = db.getTable(parser_result.alias_to_relation[attr_binding.attr2.alias]); 
    auto a1 = t1.getAttribute(attr_binding.attr1.name); 
    auto a2 = t1.getAttribute(attr_binding.attr2.name);
    /* Estimations as in exercise 02 */
    if (a1.getKey() && a2.getKey()) {
      cout << 1.0 / max(t1.getCardinality(), t2.getCardinality());
    } else if (a1.getKey() && !a2.getKey()) {
      cout << 1.0 / t1.getCardinality();
    } else if (!a1.getKey() && a2.getKey()) {
      cout << 1.0 / t2.getCardinality();
    } else if (!a1.getKey() && !a2.getKey()) {
      cout << 1.0 / max(a1.getUniqueValues(), a2.getUniqueValues());
    }
    cout << endl;
  }
  
}

int main() {
  cout << "***** Starting Parser *****" << endl;
  auto parser_result = parser(string("SELECT s.matrnr,s.name,v.titel FROM studenten s,hoeren h,vorlesungen v,professoren p WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer"));
  //run_query(parser_result);
  build_query_graph(parser_result);
}

//---------------------------------------------------------------------------
