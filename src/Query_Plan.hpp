#ifndef H_Query_plan
#define H_Query_plan

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
#include <iostream>
#include "Parser.hpp"
#include "Utils.hpp"
#include "Join_Graph.hpp"

struct Query_Plan {
  Parser_Result parser_result;
  map<Attr, const Register*> attr_to_register;
  map<string, unique_ptr<Operator>> alias_to_filtered_tables;  // holds the filtered table scans
  
  map<string, Register> equal_constant_registers; // must remain in scope or output will be empty!
  unique_ptr<Database> db;
  
  unique_ptr<Operator> result;
  
  Query_Plan(Parser_Result parser_result) : parser_result(parser_result), db(unique_ptr<Database>(new Database)) {
    db->open(parser_result.db);
    
    run_pushed_down_selections();
  }
  
  void run_pushed_down_selections () {
    /* Prepares alias_to_filtered_tables by applying pushed down selectections on table scans */
    
    cout << endl << "***** Applying pushed down selections *****" << endl;
    
    // Create TableScans
    map<string, unique_ptr<Tablescan>> alias_to_tables;
    for (string alias : parser_result.aliases) {
      alias_to_tables[alias] = unique_ptr<Tablescan> (new Tablescan(db->getTable(parser_result.alias_to_relation[alias])));
    }
    
    // Create Registers
    for (Attr attr : parser_result.attributes) {
      attr_to_register[attr] = alias_to_tables[attr.alias]->getOutput(attr.name);
    }
    
    // Constant Bindings
    // basically convert from Tablescan-map to Operator-map
    for (auto& alias_table : alias_to_tables) {
      alias_to_filtered_tables[alias_table.first] = move(alias_table.second);
    }
    
    
    for (auto& binding : parser_result.constant_bindings) {
      equal_constant_registers[binding.constant] = Register(binding.constant);
      unique_ptr<Chi> filter(new Chi(move(alias_to_filtered_tables[binding.attr.alias]),Chi::Equal,attr_to_register[binding.attr],&equal_constant_registers[binding.constant]));
      const Register* filtered_register=filter->getResult();
      alias_to_filtered_tables[binding.attr.alias] = unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
    }
  }
    
  void run() {
    /* Run a query in form of Parser_Result on tinydb */
       
    cout << endl << "***** Running Query: *****" << endl;
    
    // assuming an algorithm was already applied
    
    output_result();
  }
  
  void output_result() {
    assertion(!!result, "No Query-Plan existing. Cannot output result until algorithm (like GOO) was applied");
    
    // Apply Projections
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
  
  void apply_goo() {
    cout << endl << "***** Creating GOO Query Plan with Crossproducts*****" << endl;
    
    // initialize join_graph
    Join_Graph join_graph;
    for (auto alias : parser_result.aliases) {
     join_graph.add_leaf(alias, Join_Graph_Node(move(alias_to_filtered_tables[alias]))); 
    }
    
    
    auto unjoined_nodes = join_graph.leaves;
    while (unjoined_nodes.size() > 1) {
      shared_ptr<Join_Graph_Node> best_node; // (join/crossproduct)
      for (auto node_pair : all_pairs_in_set<Join_Graph_Node>(unjoined_nodes)) {
	shared_ptr<Join_Graph_Node> tmp_node;
	
	unique_ptr<Operator> tmp_result;
	auto attribute_bindings = parser_result.find_attribute_bindings(node_pair.first.aliases, node_pair.second.aliases); // TODO
	if (attribute_bindings.size() == 0) {
	  // Crossproduct
	  tmp_result =  unique_ptr<Operator> (new CrossProduct(move(node_pair.first.table), move(node_pair.second.table)));
	} else {
	 // Join
	  auto binding = attribute_bindings.pop // TODO
	  tmp_result =  unique_ptr<Operator> (new HashJoin(move(node_pair.first.table), move(node_pair.second.table), //TODO ));
	  
	  apply remaining selects //TODO
	}
	
	tmp_node = Join_Graph_Node(move(join));
	if (!best_node) {
	 best_node = tmp_node;
	} else {
	 if (best_node.size < tmp_node.size) {
	   tmp_node.revert();
	 } else {
	   best_node.revert();
	   best_node = tmp_node;
	 }
	} 
      }
    }
    
    result = move(unjoined_nodes.begin().evaluate());
  }
  
  void apply_canonical() {
    cout << endl << "***** Creating Canonical Query Plan *****" << endl;
    
    // choose first table on which to join
    string first_alias = alias_to_filtered_tables.begin()->first;
    result = move(alias_to_filtered_tables[first_alias]);
    alias_to_filtered_tables.erase(first_alias);
    
    // Canonical CrossProduct
    for (auto& alias_table : alias_to_filtered_tables) {
      result = unique_ptr<Operator> (new CrossProduct(move(alias_table.second), move(result)));
    }
    
    // Canonical Selection (join bindings)
    for (auto& binding : parser_result.attr_bindings) {
      unique_ptr<Chi> filter(new Chi(move(result),Chi::Equal,attr_to_register[binding.attr1],attr_to_register[binding.attr2]));
      const Register* filtered_register=filter->getResult();
      result = move(unique_ptr<Selection> (new Selection(move(filter),filtered_register)));  
    }
  }

};

#endif