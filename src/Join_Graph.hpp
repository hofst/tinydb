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
  string repr;
  
  unique_ptr<Operator> table; // only accessible while evaluated == False!
  shared_ptr<Join_Graph_Node> next;
  shared_ptr<Join_Graph_Node> left, right;
  
  Join_Graph_Node(unique_ptr<Operator> t, set<string> aliases, Node_Type type, shared_ptr<Join_Graph_Node> l = shared_ptr<Join_Graph_Node>(), shared_ptr<Join_Graph_Node> r = shared_ptr<Join_Graph_Node>())
  : evaluated(false), size(0), type(type), aliases(aliases), table(move(t)), left(l), right(r) {
    size = table->size();
    repr = representation();
  };
  
  ~Join_Graph_Node() {
    cout << "destructed: " << type_str() << " " << set_representation(aliases) << endl;
  }
  
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> node1, shared_ptr<Join_Graph_Node> node2, unique_ptr<Operator> table) {
    assertion(!node1->evaluated, "Nodes can only get evaluated once: " + set_representation(node1->aliases));
    assertion(!node2->evaluated, "Nodes can only get evaluated once: " + set_representation(node2->aliases));
    node1->evaluated = node2->evaluated = true;
    shared_ptr<Join_Graph_Node> new_node (new Join_Graph_Node(move(table), merge_sets<string>(node1->aliases,node2->aliases), Node_Type::CROSSPRODUCT, node1, node2));
    node2->next = new_node;
    node1->next = new_node;
    return new_node;
  }
  
  shared_ptr<Join_Graph_Node> select(shared_ptr<Join_Graph_Node> node, unique_ptr<Operator> table) {
    assertion(!node->evaluated, "Nodes can only get evaluated once: " + set_representation(node->aliases));
    node->evaluated = true;
    shared_ptr<Join_Graph_Node> new_node(new Join_Graph_Node(move(table), aliases, Node_Type::SELECT, node));
    node->next = new_node;
    return new_node;
  }
  
  void print(int depth=0) {
    if (depth > 5) return;
    if (depth == 0) cout << endl << "***** Printing Join Graph *****" << endl;
    
    for (int i=0; i < depth; i++) cout << "\t";
    cout << type_str() << ": " << set_representation(aliases) << " (" << size << ")" << "   [" << this << "]" << endl;
    
    if (type==Node_Type::SELECT) {
      left->print(depth+1);
    } else if (type==Node_Type::CROSSPRODUCT) {
      left->print(depth+1);
      right->print(depth+1);
    }
  }
  
  string representation() {
    if (type == Node_Type::LEAF) {
      return set_representation(aliases);
    } else if (type == Node_Type::CROSSPRODUCT) {
      return left->representation() + "x" + right->representation();
    } else if (type == Node_Type::SELECT) {
      return "#(" + left->representation() + ")";
    }
    assertion(false);
    return "";
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
    return repr < n2.repr;
  }
  
  string aliases_str() {
   return set_representation(aliases); 
  }
  
  
};

#endif