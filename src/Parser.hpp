#ifndef H_Parser
#define H_Parser

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
  string query;
  string db;
  
  set<string> relations;
  set<string> aliases;
  set<Attr> attributes;
  set<Attr> selected_attributes;
  map<string, string> alias_to_relation;  // maps an alias to its relation
  set<Attr_Binding> attr_bindings;  // list of (attribute; attribute)
  set<Relation_Binding> relation_bindings;  // list of (relation; relation) for query graph construction
  set<Constant_Binding> constant_bindings;  // list of (attribute; constant)
  
  Parser_Result (string query, string db) : query(query), db(db)  {
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
    extract_selected_attributes(select_clause);
    
    // extract alias->relation mappings
    extract_alias_relation_mappings(from_clause);
    
    // extract bindings
    extract_bindings(where_clause);
      
    // check if relations and attributes exist
    Database database;
    database.open(db);
    
    for (auto& relation : relations) {
	assert(database.hasTable(relation));  // make sure relations exist
    }
    
    for (auto& attr : attributes) {
	auto table = database.getTable(alias_to_relation[attr.alias]);
	assert(table.findAttribute(attr.name) > -1);  // make sure attributes exist
    }
    
    cout << endl << "***** Sucessfully parsed the following query: *****" << endl << query << endl;
  }
  
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
  
  void extract_selected_attributes(string& select_clause) {
    /* Extracts a list of attributes from the select_clause */
    if (select_clause == "*") return;
    
    auto aliases_with_attribute = split_match(select_clause, "(\\w|\\.)+");  // list of alias.attribute
    
    cout << "Selected attributes: ";
    for (auto alias_with_attribute : aliases_with_attribute) {
      auto attr = Attr(alias_with_attribute);
      cout << attr.str() << ", ";
      add_selected_attribute(attr);
    }
    cout << endl;
  }

  void extract_alias_relation_mappings(string& from_clause) {
    /* Extracts the mapping alias->relation from the from_clause */
    auto tokens = split_match(from_clause, "\\w+");
    
    cout << "Alias->Relation mappings: ";
    for (unsigned i=0; i<tokens.size(); i+=2) {
      cout << tokens[i+1] << "->" << tokens[i] << ", ";
      add_alias_to_relation(tokens[i+1], tokens[i]);
    }
    cout << endl;
  }

  void extract_bindings(string& where_clause) {
    /* Extracts the bindings from the where_clause */
    auto tokens = split_match(where_clause, "(\\w|\\.|#)+");
    
    cout << "Bindings: ";
    for (unsigned i=0; i<tokens.size(); i+=3) {
      auto attr1 = Attr(tokens[i]);
      cout << attr1.str() << "=";

      if (tokens[i+1].find(".")!=string::npos) {
	// attribute binding
	auto attr2 = Attr(tokens[i+1]);
	cout << attr2.str() << ", ";
	add_attr_binding(Attr_Binding(attr1, attr2));
      } else {
      // constant binding
	cout << tokens[i+1];
	add_constant_binding(Constant_Binding(attr1, tokens[i+1]));
      }
    }
    cout << endl;
  }
};

#endif