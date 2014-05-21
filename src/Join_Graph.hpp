#ifndef H_Join_Graph
#define H_Join_Graph

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <boost/iterator/iterator_concepts.hpp>
#include "Utils.hpp"
#include "Parser.hpp"
#include "operator/Operator.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/HashJoin.hpp"


using namespace std;

enum Node_Type {
  LEAF,
  SELECT,
  CROSSPRODUCT
};

struct Join_Graph_Node {
  bool evaluated;
  int size;
  Node_Type type;
  set<string> aliases;
  
  unique_ptr<Operator> table; // only accessible while evaluated == False!
  shared_ptr<Join_Graph_Node> next;
  shared_ptr<Join_Graph_Node> left, right;
  
  Join_Graph_Node(unique_ptr<Operator> table, set<string> aliases, Node_Type type) : evaluated(false), type(type), aliases(aliases) {
    table->open();
    while(table->next()) size++;
    table->close();
    this->table = move(table);
  };
  
  shared_ptr<Join_Graph_Node> unjoin() {
    assertion(type != Node_Type::LEAF, "A Leaf cannot be unjoined");
    shared_ptr<Join_Graph_Node> cross_node(this);
    
    // Before reverting the underlying crossproduct, revert all selects
    while(cross_node->type == Node_Type::SELECT) {
     shared_ptr<Operator> _n (move(cross_node->table));
     shared_ptr<Selection> s = static_pointer_cast<Selection>(_n);
     left->table = unique_ptr<Operator> (move(s->input));
     left->next.reset();
     cross_node = left;
    }
    // now all selects are reverted and there must be a crossproduct
    assertion(cross_node->type == Node_Type::CROSSPRODUCT, "Crossproduct exptected");
    
    shared_ptr<Operator> left_table (move((cross_node)->left->table));
    shared_ptr<CrossProduct> left_table_cross = static_pointer_cast<CrossProduct>(left_table);
    shared_ptr<Operator> right_table (move((cross_node)->right->table));
    shared_ptr<CrossProduct> right_table_cross = static_pointer_cast<CrossProduct>(right_table);
    
    cross_node->left->table = move(left_table_cross->left);
    cross_node->left->evaluated = false;
    cross_node->left->next.reset();
    cross_node->right->table = move(right_table_cross->right);
    cross_node->right->evaluated = false;
    cross_node->right->next.reset();
    return cross_node;
  }
  
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> node1, shared_ptr<Join_Graph_Node> node2, unique_ptr<Operator> table) {
    assertion(!evaluated && !node2->evaluated, "This node can only be evaluated once");
    evaluated = node2->evaluated = true;
    node2->next = next = shared_ptr<Join_Graph_Node>(new Join_Graph_Node(move(table), merge_sets<string>(aliases,node2->aliases), Node_Type::CROSSPRODUCT));
    next->left = node1;
    next->right = node2;
    return next;
  }
  
  shared_ptr<Join_Graph_Node> select(shared_ptr<Join_Graph_Node> node, unique_ptr<Operator> table) {
    assertion(!evaluated, "This node can only be evaluated once");
    evaluated = true;
    next = shared_ptr<Join_Graph_Node>(new Join_Graph_Node(move(table), aliases, Node_Type::SELECT));
    next->left = node;
    return next;
  }  
  
  bool operator<(const Join_Graph_Node& n2) const {
    return table < n2.table;
  }
  
  
};

#endif