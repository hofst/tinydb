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
  map<string, shared_ptr<Join_Graph_Node>> join_graphs;
  
  map<Attr, const Register*> attr_to_register;
  map<string, Register> equal_constant_registers; // must remain in scope or output will be empty!
  
  unique_ptr<Database> db;
  
  unique_ptr<Operator> result;
  
  Query_Plan(Parser_Result parser_result) : parser_result(parser_result), db(unique_ptr<Database>(new Database)) {
    db->open(parser_result.db);
    
    init_join_tree();
  }
  
  void init_join_tree () {
    /* Inits the join tree and applies pushed down selections */
    
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
    for (auto alias : parser_result.aliases) {
      shared_ptr<Join_Graph_Node> leaf(new Join_Graph_Node(move(alias_to_tables[alias])), alias, Node_Type::LEAF);
      join_graphs[alias] = leaf;
    }
    
    for (auto& binding : parser_result.constant_bindings) {
      equal_constant_registers[binding.constant] = Register(binding.constant);
      unique_ptr<Chi> filter(new Chi(move(join_graphs[binding.attr.alias]),Chi::Equal,attr_to_register[binding.attr],&equal_constant_registers[binding.constant]));
      const Register* filtered_register=filter->getResult();
      join_graphs[binding.attr.alias] = unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
    }
  }
     
  void output_result() {
    cout << endl << "***** Running Query: *****" << endl;
    
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
  
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> n1, shared_ptr<Join_Graph_Node> n2) {
    shared_ptr<Join_Graph_Node> tmp_node = n1->join(n2);
    join_graphs.erase(n1);
    join_graphs.erase(n2);
    join_graphs[tmp_node->left.aliases + tmp_node->right.aliases] = tmp_node;
    return tmp_node;
  }
  
  void unjoin(shared_ptr<Join_Graph_Node> n) {
    n->unjoin();
    join_graphs[tmp_node->left.aliases] = tmp_node->left;
    join_graphs[tmp_node->right.aliases] = tmp_node->right;
    join_graphs.erase(n);
    return tmp_node;
  }
  
  void apply_goo() {
    cout << endl << "***** Creating GOO Query Plan with Crossproducts*****" << endl;  
    
    while (join_graphs.size() > 1) {
      pair<shared_ptr<Join_Graph_Node>, shared_ptr<Join_Graph_Node>> best_pair;
      int best_size;
      
      set<shared_ptr<Join_Graph_Node>> node_pairs = all_pairs_in_set<shared_ptr<Join_Graph_Node>>(map_values<string, shared_ptr<Join_Graph_Node>>(join_graphs));
      
      for (auto node_pair : node_pairs) {
	shared_ptr<Join_Graph_Node> tmp_node = join(node_pair.first, node_pair.second);
	
	if (!best_pair || best_size > tmp_node->size) {
	 best_pair = node_pair;
	 best_size = tmp_node->size;
	}
	unjoin(tmp_node);
      }
      join(best_pair.first, best_pair.second);
    }
    
    result = move(unjoined_nodes.begin().evaluate());
  }
  
  void apply_canonical() {
    cout << endl << "***** Creating Canonical Query Plan *****" << endl;
    
    while (join_graphs.size() > 1) {
      join(join_graphs.begin().second, join_graphs.begin().next().second);
    }
    
    result = move(unjoined_nodes.begin().evaluate());
};

#endif