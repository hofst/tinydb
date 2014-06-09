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
  map<string, shared_ptr<Join_Graph_Node>> join_graph_leaves;
  unique_ptr<Database> db;

  
  Query_Plan(Parser_Result parser_result) : parser_result(parser_result), db(unique_ptr<Database>(new Database)) {
    db->open(parser_result.db);
    
    init_join_graph_leaves();
  }
  
  void init_join_graph_leaves () {
    /* Inits the join tree and applies pushed down selections */
    
    cout << endl << "***** Initializing Join Graph Leaves and pushing down Selections *****" << endl;

    // Create Leaves
    for (auto alias : parser_result.aliases) {
      set<string> aliases {alias};
      join_graph_leaves[alias] = shared_ptr<Join_Graph_Node>(new LEAF(this, aliases));
    }
  
    // Push down selections
    for (auto& binding : parser_result.constant_bindings) {
      join_graph_leaves[binding.attr.alias] = shared_ptr<Join_Graph_Node>(new CSELECT(this, aliases));
    }
  }
     
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> n1, shared_ptr<Join_Graph_Node> n2) {
    auto attr_bindings = parser_result.find_attr_bindings(n1->aliases, n2->aliases);
    
    unique_ptr<Operator> table;
    shared_ptr<Join_Graph_Node> n;
    
    if (attr_bindings.size() == 0) { 
      // Crossproduct
      table = unique_ptr<Operator>(new CrossProduct(move(n1->table), move(n2->table)));
      n = n1->join(n1, n2, move(table));
    } else {
      // Hashjoin
      auto attr_binding = *attr_bindings.begin();
      attr_bindings.erase(attr_binding);
      table = unique_ptr<Operator>(new HashJoin(move(n1->table), move(n2->table), attr_to_register[attr_binding.attr1], attr_to_register[attr_binding.attr2]));
      n = n1->join(n1, n2, move(table), Node_Type::HASHJOIN);
    }
    
    // Selects
    for (auto binding : attr_bindings) {
      unique_ptr<Selection> selection (new Selection(move(n->table),attr_to_register[binding.attr1], attr_to_register[binding.attr2]));
      n = n->select(n, move(selection));
    }
    
    join_graph_roots.insert(n);
    join_graph_roots.erase(n1);
    join_graph_roots.erase(n2);
    return n;
  }
  
  void cp_sub() {
    cout << endl << "***** Creating cp_sub Query Plan *****" << endl; 
      map<string, shared_ptr<Join_Graph_Node>> B;
      auto R = parser_result.aliases;
      
      for (int i=1; i<R.size) {
	auto S = int_to_set(parser_result.aliases, i);
	for (auto p : partitions(S)) {
	  S1 = p.first;
	  S2 = p.second;
	  p1 = B[set_representation(S1)];
	  p2 = B[set_representation(S2)];
	  auto P = join(p1, p2);
	  if (B.find(S) == B.end() || B[set_representation(S)].size > P.size) {
	    B[set_representation(S)] = P;
	  }
	}
      }
      
      /* Print B */
      cout << "DP Table:" << endl; 
      for (auto b : B) {
	cout << b.first << b.second->representation() << " [" << b.second->size << "]" << endl;
      }
      
      return B[set_representation(R)];
  }
    
  void goo() {
    cout << endl << "***** Creating GOO Query Plan with Crossproducts *****" << endl;  
       
    while (join_graph_roots.size() > 1) {
      pair<shared_ptr<Join_Graph_Node>, shared_ptr<Join_Graph_Node>> best_pair;
      int best_size = -1;
      
      auto node_pairs = all_pairs_in_set<shared_ptr<Join_Graph_Node>>(join_graph_roots);
      for (auto node_pair : node_pairs) {
	if (best_size==-1 && node_pairs.size() == 1) {
	  // Pair can be choosen immediately
	  best_pair = node_pair;
	  break;
	}
	
	int join_test_size = join_test(node_pair.first, node_pair.second, best_size);
	cout << "GOO Join Tested : " << node_pair.first->aliases_str() << " x " << node_pair.second->aliases_str() << " with size " << join_test_size << endl;
	if (best_size==-1 || best_size > join_test_size) {
	  best_pair = node_pair;
	  best_size = join_test_size;
	}
      }
      cout << ">>> Winner is : " << best_pair.first->aliases_str() << " x " << best_pair.second->aliases_str() << " with size " << best_size << endl;
      join(best_pair.first, best_pair.second);
    }
    
    result = move(get_join_graph()->table);
  }
  
  void canonical_optimized() {
    cout << endl << "***** Creating Logically Optimized Canonical Query Plan *****" << endl;
    
    while (join_graph_roots.size() > 1) {
      join(*join_graph_roots.begin(), *(++join_graph_roots.begin()));
    }
    
    result = move(get_join_graph()->table);
  }
};

#endif