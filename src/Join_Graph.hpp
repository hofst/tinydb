#ifndef H_Join_Graph
#define H_Join_Graph

#include <iostream>
#include <string>
#include <set>
#include <map>
#include "Utils.hpp"
#include "Parser.hpp"
#include "operator/Operator.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/HashJoin.hpp"
#include "Query_Plan.hpp"

using namespace std;

struct Join_Graph_Node {
  int size;
  string repr;
  
  Query_Plan* query_plan;
  
  Join_Graph_Node(Query_Plan* query_plan)
  : query_plan(query_plan), size(-1) {
    set_size();
    repr = representation();
  };
    
  void print(int depth=0) {
    if (depth == 0) cout << endl << "***** Printing Join Graph *****" << endl;
    
    for (int i=0; i < depth; i++) cout << "\t";
    
    cout << type_str() << ": " << set_representation(aliases) << " (size:" << size << ")" << "   [" << this << "]" << endl;
    
    print_rec();
  }
  
  bool operator<(const Join_Graph_Node& n2) const {
    if (size < n2.size) {
      return true;
    } else {
      return repr < n2.repr;
    }
  }
  
  string aliases_str() {
   return set_representation(aliases); 
  }
  
  void output_result() {
    cout << endl << "***** Running Query: *****" << endl;
    
    // Apply Projections
    vector<const Register*> selected_registers;
    for (auto attr : query_plan->parser_result.selected_attributes) {
	selected_registers.push_back(query_plan->attr_to_register[attr]);
    }
    Printer out (get_table(), selected_registers);

    // Print Result
    out.open();
    while (out.next());
    out.close();
  }
  
  virtual void set_size() {
    if (size == -1) size = get_table()->size();
  }
  
  virtual set<string> aliases();
  
  virtual void print_rec(int depth=0);
  
  virtual string representation();
  
  virtual string type_str();
  
  virtual unique_ptr<Operator> get_table();
};

struct LEAF:Join_Graph_Node {
  string alias;
  
  LEAF(Query_Plan* query_plan, string alias) : alias(alias){
    Join_Graph_Node(query_plan);
  }
  
  virtual unique_ptr<Tablescan> get_table() {
    return unique_ptr<Tablescan> (new Tablescan(query_plan->db->getTable(query_plan->parser_result.alias_to_relation[alias])));
  }
  
  virtual set<string> aliases() {
      return {alias};
  }
  
  virtual void print_rec(int depth=0) {}
  
  virtual string representation() {return set_representation(aliases);}
  
  virtual string type_str() {return "LEAF";}
};

struct SELECT:Join_Graph_Node {
  shared_ptr<Join_Graph_Node> child;
  
  SELECT(Query_Plan* query_plan, shared_ptr<Join_Graph_Node> child) : child(child) {
    Join_Graph_Node(query_plan);
  }
  
  virtual set<string> aliases() {
      return child->aliases();
  }
  
  virtual void print_rec(int depth=0) {
    left->print(depth+1);
  }
  
  virtual string representation() {return "#(" + left->representation() + ")";}
  
  virtual string type_str() {return "SELECT";}
};

struct CSELECT:SELECT {
  Constant_Binding binding;
  
  CSELECT(Query_Plan* query_plan, shared_ptr<Join_Graph_Node> child, Constant_Binding binding) : binding(binding) {
    SELECT(query_plan, child);
  }
  
  virtual unique_ptr<Selection> get_table() {
      unique_ptr<Chi> filter(new Chi(
	move(child->get_table()),
	Chi::Equal,
	query_plan->attr_to_register[binding.attr],
	&query_plan->equal_constant_registers[binding.constant]));
      
      const Register* filtered_register=filter->getResult();
      return unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
  }
};

struct ASELECT:SELECT {
  Attr_Binding binding;
  
  ASELECT(Query_Plan* query_plan, shared_ptr<Join_Graph_Node> child, Attr_Binding binding) : binding(binding) {
    SELECT(query_plan, child);
  }
  
  virtual unique_ptr<Selection> get_table() {
      return unique_ptr<Selection> (new Selection(move(child->get_table()), query_plan->attr_to_register[binding.attr1], query_plan->attr_to_register[binding.attr2]));
  }
};

struct CROSSPRODUCT:Join_Graph_Node {
  shared_ptr<Join_Graph_Node> child;
  shared_ptr<Join_Graph_Node> child2;
  
  CROSSPRODUCT(Query_Plan* query_plan, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2) : child(child), child2(child2) {
    Join_Graph_Node(query_plan);
  }
  
  virtual unique_ptr<CrossProduct> get_table() {
    return unique_ptr<Operator>(new CrossProduct(move(child->get_table()), move(child2->get_table())));
  }
  
  virtual set<string> aliases() {
      auto aliases1 = child->aliases();
      auto aliases2 = child2->aliases();
      aliases1.insert(aliases2.begin(), aliases2.end());
      return aliases1;
  }
  
  virtual void print_rec(int depth=0) {
    left->print(depth+1);
    right->print(depth+1);
  }
  
  virtual void set_size() {
    if (size == -1) size = child->size * child2->size; 
  }
        
  virtual string representation() {return left->representation() + "x" + right->representation();}
  
  virtual string type_str() {return "CROSSPRODUCT";}
};

struct HASHJOIN:CROSSPRODUCT {
  Attr_Binding attr_binding;
  
  HASHJOIN(Query_Plan* query_plan, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2, Attr_Binding attr_binding) : attr_binding(attr_binding) {
    CROSSPRODUCT(query_plan, child, child2);
  }
  
  virtual unique_ptr<HashJoin> get_table() {
    return unique_ptr<Operator>(new HashJoin(move(child->get_table()), move(child2->get_table()), query_plan->attr_to_register[attr_binding.attr1], query_plan->attr_to_register[attr_binding.attr2]));
  }
  
  virtual string representation() {return left->representation() + " |x| " + right->representation();}
  
  virtual string type_str() {return "HASHJOIN";}
};

#endif