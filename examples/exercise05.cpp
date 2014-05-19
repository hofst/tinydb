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
#include "Parser.hpp"
#include "Query_Plan.hpp"

using namespace std;


void run_query(Parser_Result parser_result) {
  /* Run a query in form of Parser_Result on tinydb */
  
  cout << endl << "***** Running Query: *****" << endl;
  
  // Open Database
  Database db;
  db.open(parser_result.db);
  
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
  db.open(parser_result.db);
  
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
  cout << "***** Running Parser *****" << endl;
  auto parser_result = Parser_Result(string("SELECT s.matrnr,s.name,v.titel FROM studenten s,hoeren h,vorlesungen v,professoren p WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer"), string("data/uni"));
  //auto parser_result = Parser_Result(string("SELECT * FROM lineitem l,orders o,customers c WHERE l.l_orderkey=o.o_orderkey AND o.ocustkey_c.c_custkey AND c.c_name=Customer#000014993"), string("data/tpch/tpch"));
  
  auto query_plan = Query_Plan(parser_result);
  query_plan.apply_goo();
  query_plan.run();
  
  run_query(parser_result);
  build_query_graph(parser_result);
}

//---------------------------------------------------------------------------
