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
  map<string, Register> equal_constant_registers;
  
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
    for (auto alias : parser_result.aliases) {
      set<string> aliases {alias};
      shared_ptr<Join_Graph_Node> leaf(new Join_Graph_Node(move(alias_to_tables[alias]), aliases, Node_Type::LEAF));
      join_graphs[alias] = leaf;
    }
    
    for (auto& binding : parser_result.constant_bindings) {
      equal_constant_registers[binding.constant] = Register(binding.constant);
      
      unique_ptr<Chi> filter(new Chi(
	move(join_graphs[binding.attr.alias]->table),
	Chi::Equal,
	attr_to_register[binding.attr],
	&equal_constant_registers[binding.constant]));
      
      const Register* filtered_register=filter->getResult();
      join_graphs[binding.attr.alias]->table = unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
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
    // Crossproduct
    unique_ptr<Operator> table (new CrossProduct(move(n1->table), move(n2->table)));
    shared_ptr<Join_Graph_Node> n = n1->join(n1, n2, move(table));
    
    // Selects
    for (auto binding : parser_result.find_attr_bindings(n->left->aliases, n->right->aliases)) {
      unique_ptr<Selection> selection (new Selection(move(n->table),attr_to_register[binding.attr1], attr_to_register[binding.attr2]));
      n = n->select(n, move(selection));  
    }
    
    join_graphs.erase(set_representation(n1->aliases));
    join_graphs.erase(set_representation(n2->aliases));
    join_graphs[set_representation(n->aliases)] = n;
    return n;
  }
  
  void unjoin(shared_ptr<Join_Graph_Node> n) {
    n->unjoin();
    join_graphs[set_representation(n->left->aliases)] = n->left;
    join_graphs[set_representation(n->right->aliases)] = n->right;
    join_graphs.erase(set_representation(n->aliases));
  }
  
  void apply_goo() {
    cout << endl << "***** Creating GOO Query Plan with Crossproducts*****" << endl;  
    
    while (join_graphs.size() > 1) {
      pair<shared_ptr<Join_Graph_Node>, shared_ptr<Join_Graph_Node>> best_pair;
      bool init = false;
      int best_size;
      
      auto node_pairs = all_pairs_in_set<shared_ptr<Join_Graph_Node>>(map_values<shared_ptr<Join_Graph_Node>>(join_graphs));
      
      for (auto node_pair : node_pairs) {
	shared_ptr<Join_Graph_Node> tmp_node = join(node_pair.first, node_pair.second);
	
	if (!init || best_size > tmp_node->size) {
	 init = true;
	 best_pair = node_pair;
	 best_size = tmp_node->size;
	}
	unjoin(tmp_node);
      }
      join(best_pair.first, best_pair.second);
    }
    
    result = move(get_join_graph()->table);
  }
  
  void apply_canonical_optimized() {
    cout << endl << "***** Creating Logically Optimized Canonical Query Plan *****" << endl;
    
    while (join_graphs.size() > 1) {
      join(join_graphs.begin()->second, (++join_graphs.begin())->second);
    }
    
    result = move(get_join_graph()->table);
  }
  
  shared_ptr<Join_Graph_Node> get_join_graph() {
      return join_graphs.begin()->second;
  }
};

#endif