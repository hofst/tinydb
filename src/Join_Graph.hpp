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
  
  Join_Graph_Node(unique_ptr<Operator> t, set<string> aliases, Node_Type type) : evaluated(false), size(0), type(type), aliases(aliases), table(move(t)) {
    table->open();
    while(table->next()) size++;
    table->close();
  };
  
  ~Join_Graph_Node() {
  }
  
  shared_ptr<Join_Graph_Node> unjoin() {
    assertion(type != Node_Type::LEAF, "A Leaf cannot be unjoined");
    shared_ptr<Join_Graph_Node> cross_node(this);
    
    // Before reverting the underlying crossproduct, revert all selects
    while(cross_node->type == Node_Type::SELECT) {
     shared_ptr<Operator> _n (move(cross_node->table));
     shared_ptr<Selection> s = static_pointer_cast<Selection>(_n);
     cross_node->left->table = unique_ptr<Operator> (move(s->input));
     cross_node->left->next.reset();
     cross_node = cross_node->left;
    }
    // now all selects are reverted and there must be a crossproduct
    assertion(cross_node->type == Node_Type::CROSSPRODUCT, "Crossproduct exptected");
    
    shared_ptr<Operator> _n (move(cross_node->table));
    shared_ptr<CrossProduct> cp = static_pointer_cast<CrossProduct>(_n);
    
    cross_node->left->table = move(cp->left);
    cross_node->left->evaluated = false;   
    cross_node->left->next.reset();
    cross_node->right->table = move(cp->right);
    cross_node->right->evaluated = false;
    cross_node->right->next.reset();
    return cross_node;
  }
  
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> node1, shared_ptr<Join_Graph_Node> node2, unique_ptr<Operator> table) {
    assertion(!node1->evaluated && !node2->evaluated, "Nodes can only get evaluated once");
    node1->evaluated = node2->evaluated = true;
    node1->next = node2->next = shared_ptr<Join_Graph_Node>(new Join_Graph_Node(move(table), merge_sets<string>(node1->aliases,node2->aliases), Node_Type::CROSSPRODUCT));
    node1->next->left = node1;
    node1->next->right = node2;
    return node1->next;
  }
  
  shared_ptr<Join_Graph_Node> select(shared_ptr<Join_Graph_Node> node, unique_ptr<Operator> table) {
    assertion(!evaluated, "This node can only be evaluated once");
    node->evaluated = true;
    node->next = shared_ptr<Join_Graph_Node>(new Join_Graph_Node(move(table), aliases, Node_Type::SELECT));
    node->next->left = node;
    return node->next;
  }
  
  void print(int depth=0) {
    if (depth == 0) cout << endl << "***** Printing Join Graph *****" << endl;
    
    for (int i=0; i < depth; i++) cout << "\t";
    cout << type_str() << ": " << set_representation(aliases) << " (" << size << ")" << endl;
    
    if (type==Node_Type::SELECT) {
      left->print(depth+1);
    } else if (type==Node_Type::CROSSPRODUCT) {
      left->print(depth+1);
      right->print(depth+1);
    }
  }
  
  string type_str() {
    string result;
    if (type==Node_Type::LEAF) {
      result = "Leaf";
    } else if (type==Node_Type::SELECT) {
      result = "Select";
    } else if (type==Node_Type::CROSSPRODUCT) {
      result = "CrossProduct";
    }
    return result;
  }
  
  bool operator<(const Join_Graph_Node& n2) const {
    return table < n2.table;
  }
  
  
};

#endif