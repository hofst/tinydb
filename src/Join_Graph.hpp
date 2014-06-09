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

using namespace std;

struct Join_Graph_Node {
  shared_ptr<Parser_Result> parser_result;
  shared_ptr<Database> db;
  
  int s;  // cached size
  
  Join_Graph_Node(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db)
  : parser_result(parser_result), db(db), s(-1) {};
    
  void print(int depth=0) {
    if (depth == 0) cout << endl << "***** Printing Join Graph *****" << endl;
    
    for (int i=0; i < depth; i++) cout << "\t";
    
    cout << type_str() << ": " << set_representation(aliases()) << " (size:" << get_size() << ")" << "   [" << this << "]" << endl;
    
    print_rec();
  }
  
  bool operator<(const Join_Graph_Node& n2) const {
      return this < &n2 ;
  }
  
  string aliases_str() {
   return set_representation(aliases()); 
  }
  
  void output_result() {
    cout << endl << "***** Running Query: *****" << endl;
    
    auto table = get_table();
    
    // Apply Projections
    vector<shared_ptr<Register>> selected_registers;
    for (auto attr : parser_result->selected_attributes) {
	selected_registers.push_back(table->getOutput(parser_result->alias_to_relation[attr.alias]));
    }
    Printer out (move(table), selected_registers);

    // Print Result
    out.open();
    while (out.next());
    out.close();
  }
  
  int get_size() {
    if (s == -1) s = get_table()->size();
    return s;
  }
  
  virtual set<string> aliases() = 0;
  
  virtual void print_rec(int depth=0) = 0;
  
  virtual string representation() = 0;
  
  virtual string type_str() = 0;
  
  virtual unique_ptr<Operator> get_table() = 0;
};

struct LEAF:Join_Graph_Node {
  string alias;
  
  LEAF(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, string alias) : Join_Graph_Node(parser_result, db), alias(alias) {}
  
  unique_ptr<Operator> get_table() {
    return unique_ptr<Operator> (new Tablescan(db->getTable(parser_result->alias_to_relation[alias])));
  }
  
  set<string> aliases() {
      return {alias};
  }
  
  void print_rec(int depth=0) {if (depth<0) assertion(false);}
  
  string representation() {return set_representation(aliases());}
  
  string type_str() {return "LEAF";}
};

struct SELECT:Join_Graph_Node {
  shared_ptr<Join_Graph_Node> child;
  
  SELECT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child) : Join_Graph_Node(parser_result, db), child(child) {}
  
  set<string> aliases() {
      return child->aliases();
  }
  
  void print_rec(int depth=0) {
    child->print(depth+1);
  }
  
  string representation() {return "#(" + child->representation() + ")";}
  
  string type_str() {return "SELECT";}
};

struct CSELECT:SELECT {
  Constant_Binding binding;
  
  CSELECT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, Constant_Binding binding) : SELECT(parser_result, db, child), binding(binding) {}
  
  unique_ptr<Operator> get_table() {
      auto child_table = child->get_table();
      auto reg1 = child_table->getOutput(parser_result->alias_to_relation[binding.attr.alias]);
      auto reg2 = shared_ptr<Register> (new Register(binding.constant));
    
      unique_ptr<Chi> filter(new Chi(
	move(child_table),
	Chi::Equal,
	reg1,
	reg2));
      
      shared_ptr<Register> filtered_register=filter->getResult();
      return unique_ptr<Operator> (new Selection(move(filter),filtered_register));  
  }
};

struct ASELECT:SELECT {
  Attr_Binding binding;
  
  ASELECT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, Attr_Binding binding) : SELECT(parser_result, db, child), binding(binding) {}
  
  unique_ptr<Operator> get_table() {
      auto child_table = child->get_table();
      auto reg1 = child_table->getOutput(parser_result->alias_to_relation[binding.attr1.alias]);
      auto reg2 = child_table->getOutput(parser_result->alias_to_relation[binding.attr2.alias]);
      return unique_ptr<Operator> (new Selection(move(child_table), reg1, reg2));
  }
};

struct CROSSPRODUCT:Join_Graph_Node {
  shared_ptr<Join_Graph_Node> child;
  shared_ptr<Join_Graph_Node> child2;
  
  CROSSPRODUCT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2) : Join_Graph_Node(parser_result, db), child(child), child2(child2) {}
  
  unique_ptr<Operator> get_table() {
    return unique_ptr<Operator>(new CrossProduct(move(child->get_table()), move(child2->get_table())));
  }
  
  set<string> aliases() {
      auto aliases1 = child->aliases();
      auto aliases2 = child2->aliases();
      aliases1.insert(aliases2.begin(), aliases2.end());
      return aliases1;
  }
  
  void print_rec(int depth=0) {
    child->print(depth+1);
    child2->print(depth+1);
  }
  
  int get_size() {
    if (s == -1) s = child->get_size() * child2->get_size();
    return s;
  }
        
  string representation() {return child->representation() + "x" + child2->representation();}
  
  string type_str() {return "CROSSPRODUCT";}
};

struct HASHJOIN:CROSSPRODUCT {
  Attr_Binding binding;
  
  HASHJOIN(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2, Attr_Binding binding) : CROSSPRODUCT(parser_result, db, child, child2), binding(binding) {}
  
  unique_ptr<Operator> get_table() {
    auto child_table1 = child->get_table();
    auto child_table2 = child->get_table();
    auto reg1 = child_table1->getOutput(parser_result->alias_to_relation[binding.attr1.alias]);
    auto reg2 = child_table2->getOutput(parser_result->alias_to_relation[binding.attr2.alias]);
    return unique_ptr<Operator>(new HashJoin(move(child_table1), move(child_table2), reg1, reg2));
  }
  
  string representation() {return child->representation() + " |x| " + child2->representation();}
  
  string type_str() {return "HASHJOIN";}
};

#endif