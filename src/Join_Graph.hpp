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
  string aliases;
  
  unique_ptr<Operator> table; // only accessible while evaluated == False!
  shared_ptr<Join_Graph_Node> next;
  shared_ptr<Join_Graph_Node> left, right;
  
  Join_Graph_Node(unique_ptr<Operator> table, string aliases, type) : evaluated(false), aliases(aliases), type(type) {
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
  
  void unjoin() {
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
  
  shared_ptr<Join_Graph_Node> join(shared_ptr<Join_Graph_Node> node2) {
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
  }
  
  
  bool operator<(const Join_Graph_Node& n2) const
  {
    return table < n2.table;
  }
  
  
};

#endif