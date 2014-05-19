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
  HASHJOIN,
  CROSSPRODUCT
};

struct Join_Graph_Node {
  bool evaluated;
  int size;
  Node_Type type;
  set aliases;
  
  unique_ptr<Operator> table; // only accessible while evaluated == False!
  shared_ptr<Join_Graph_Node> next;
  shared_ptr<Join_Graph_Node> left, right;
  
  Join_Graph_Node(unique_ptr<Operator> table, type) : evaluated(false), type(type) {
    table->open();
    while(table->next()) size++;
    table->close();
    this->table = move(table);
  };
  
  unique_ptr<Operator> evaluate(shared_ptr<Join_Graph_Node> next) {
    /* Move up the tree and evaluate the most upper node */
    if (evaluated) {
      return next->evaluate(next);
    } else {
      evaluated = true;
      this->next = next;
      next.aliases.insert(this->aliases.begin(), this->aliases.end());
      if (!this->next.left) {
	this->next.left = make_shared<Operator>(this);
      } else {
	this->next.right = make_shared<Operator>(this);
      }
      return move(table);
    }
  }
  
  void revert() {
    unique_ptr<Operator> left_table, right_table;
    
    assertion(type != Node_Type::LEAF, "A Leaf cannot be reverted");
    
    if (type == Node_Type::HASHJOIN) {
      left_table = move(((HashJoin) this).left);
      right_table = move(((HashJoin) this).right);
    } else if (type == Node_Type::CROSSPRODUCT) {
      left_table = move(((CrossProduct) this).left);
      right_table = move(((CrossProduct) this).right);
    }
    
    left.table = move(left_table);
    left.evaluated = false;
    right.table = move(right_table);
    right.evaluated = false;
  }
  
  bool operator<(const Join_Graph_Node& n2) const
  {
    return table < n2.table;
  }
};

struct Join_Graph {
  map<string, Join_Graph_Node> leaves;
  
  void add_leaf(string alias, Join_Graph_Node node) {
    set<string> aliases {alias};  
    node.aliases = move(aliases);
    leaves[alias] = move(node);
  };

#endif