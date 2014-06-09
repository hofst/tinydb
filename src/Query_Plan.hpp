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
  shared_ptr<Parser_Result> parser_result;
  shared_ptr<Database> db;
  map<string, shared_ptr<Join_Graph_Node>> join_graph_leaves;
    
  Query_Plan(shared_ptr<Parser_Result> parser_result) : parser_result(parser_result), db(unique_ptr<Database>(new Database)) {
    db->open(parser_result->db);
    
    init_join_graph_leaves();
  }
  
  void init_join_graph_leaves () {
    /* Inits the join tree and applies pushed down selections */
    
    cout << endl << "***** Initializing Join Graph Leaves and pushing down Selections *****" << endl;

    // Create Leaves
    for (auto alias : parser_result->aliases) {
      join_graph_leaves[alias] = shared_ptr<Join_Graph_Node>(new LEAF(parser_result, db, alias));
    }
  
    // Push down selections
    for (auto binding : parser_result->constant_bindings) {
      auto child = join_graph_leaves[binding.attr.alias];
      join_graph_leaves[binding.attr.alias] = shared_ptr<Join_Graph_Node>(new CSELECT(parser_result, db, child, binding));
    }
  }
     
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> n1, shared_ptr<Join_Graph_Node> n2) {
    auto attr_bindings = parser_result->find_attr_bindings(n1->aliases(), n2->aliases());
    
    shared_ptr<Join_Graph_Node> n;
    
    if (attr_bindings.size() == 0) { 
      // Crossproduct
      n = shared_ptr<Join_Graph_Node>(new CROSSPRODUCT(parser_result, db, n1, n2));
    } else {
      // Hashjoin
      auto binding = *attr_bindings.begin();
      attr_bindings.erase(binding);
      n = shared_ptr<Join_Graph_Node>(new HASHJOIN(parser_result, db, n1, n2, binding));
    }
    
    // Selects
    for (auto binding : attr_bindings) {
      n = shared_ptr<Join_Graph_Node>(new ASELECT(parser_result, db, n, binding));
    }
    
    return n;
  }
  
  shared_ptr<Join_Graph_Node> cp_sub() {
    cout << endl << "***** Creating cp_sub Query Plan *****" << endl; 
      map<string, shared_ptr<Join_Graph_Node>> B;
      auto R = parser_result->aliases;
      
      for (auto r : R) {
	B[r] = join_graph_leaves[r];
      }
      
      for (unsigned i=1; i<pow(2,R.size()); i++) {
	auto S = int_to_set(parser_result->aliases, i);
	
	for (auto p : partitions(S)) {
	  auto S1 = p.first;
	  auto S2 = p.second;
	  auto p1 = B[set_representation(S1)];
	  auto p2 = B[set_representation(S2)];
	  auto P = join(p1, p2);
	  if (B.find(set_representation(S)) == B.end() || B[set_representation(S)]->get_size() > P->get_size()) {
	    B[set_representation(S)] = P;
	  }
	}
      }
      
      /* Print B */
      cout << "DP Table:" << endl; 
      for (auto b : B) {
	cout << b.first << b.second->representation() << " [" << b.second->get_size() << "]" << endl;
      }
      
      return B[set_representation(R)];
  }
  
  /*
  shared_ptr<Join_Graph_Node> goo() {
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
  */
  
  shared_ptr<Join_Graph_Node> canonical_optimized() {
    cout << endl << "***** Creating Logically Optimized Canonical Query Plan *****" << endl;
    
    set<shared_ptr<Join_Graph_Node>> join_graph_roots;
    for (auto n : join_graph_leaves) join_graph_roots.insert(n.second);
    
    while (join_graph_roots.size() > 1) {
      auto n1 = *join_graph_roots.begin();
      auto n2 = *(++join_graph_roots.begin());
      auto n = join(n1, n2);
      join_graph_roots.insert(n);
      join_graph_roots.erase(n1);
      join_graph_roots.erase(n2);
    }
    
    return *join_graph_roots.begin();
  }
  
};

#endif