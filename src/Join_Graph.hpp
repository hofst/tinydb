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
   
  Join_Graph_Node(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db)
  : parser_result(parser_result), db(db) {};
    
  void print(int depth=0) {
    if (depth == 0) cout << endl << "***** Printing Join Graph *****" << endl;
    
    for (int i=0; i < depth; i++) cout << "\t";
    
    cout << type_str() << ": " << set_representation(aliases()) << " (size:" << get_size() << ")" << endl;
    
    print_rec(depth);
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
	selected_registers.push_back(get_register(attr));
    }
    Printer out (table, selected_registers);

    // Print Result
    out.open();
    while (out.next());
    out.close();
  }
  
  virtual int get_size(int treshold=-1) = 0;
  
  virtual shared_ptr<Register> get_register(Attr attr) = 0;
  
  virtual set<string> aliases() = 0;
  
  virtual void print_rec(int depth=0) = 0;
  
  virtual string representation() = 0;
  
  virtual string type_str() = 0;
  
  virtual shared_ptr<Operator> get_table() = 0;
};

struct LEAF:Join_Graph_Node {
  shared_ptr<Tablescan> table;
  string alias;
  
  LEAF(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, string alias) : Join_Graph_Node(parser_result, db), alias(alias) {}
  
  shared_ptr<Operator> get_table() {
    if (!table) table = shared_ptr<Tablescan> (new Tablescan(db->getTable(parser_result->alias_to_relation[alias])));
    return table;
  }
  
  shared_ptr<Register> get_register(Attr attr) {
    get_table();
    assertion(table->getTable().findAttribute(attr.name) != -1, "Attribute not found");
    return table->getOutput(attr.name);
  }
  
  int get_size(int threshold=-1) {
   (void) threshold;
   get_table();
   return table->getTable().getCardinality(); 
  }
  
  set<string> aliases() {
      return {alias};
  }
  
  void print_rec(int depth=0) {if (depth<0) assertion(false);}
  
  string representation() {return set_representation(aliases());}
  
  string type_str() {return "LEAF";}
};

struct SELECT:Join_Graph_Node {
  shared_ptr<Selection> table;
  shared_ptr<Join_Graph_Node> child;
  
  SELECT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child) : Join_Graph_Node(parser_result, db), child(child) {}
  
  shared_ptr<Register> get_register(Attr attr) {
      return child->get_register(attr);
  }
  
  int get_size(int threshold=-1) {
    return get_table()->size(threshold);
  }
  
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
  
  shared_ptr<Operator> get_table() {
      if (!table) {
	auto child_table = child->get_table();
	auto reg1 = get_register(binding.attr);
	auto reg2 = shared_ptr<Register> (new Register(binding.constant));
      
	shared_ptr<Chi> filter(new Chi(
	  child_table,
	  Chi::Equal,
	  reg1,
	  reg2));
	
	shared_ptr<Register> filtered_register=filter->getResult();
	table = shared_ptr<Selection> (new Selection(filter,filtered_register));
      }
      return table;
  }
};

struct ASELECT:SELECT {
  Attr_Binding binding;
  
  ASELECT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, Attr_Binding binding) : SELECT(parser_result, db, child), binding(binding) {}
  
  shared_ptr<Operator> get_table() {
    if (!table) {
      auto child_table = child->get_table();
      auto reg1 = get_register(binding.attr1);
      auto reg2 = get_register(binding.attr2);
      table = shared_ptr<Selection> (new Selection(child_table, reg1, reg2));
    }
    return table;
  }
};

struct CROSSPRODUCT:Join_Graph_Node {
  shared_ptr<CrossProduct> table;
  shared_ptr<Join_Graph_Node> child;
  shared_ptr<Join_Graph_Node> child2;
  
  CROSSPRODUCT(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2) : Join_Graph_Node(parser_result, db), child(child), child2(child2) {}
  
  virtual shared_ptr<Operator> get_table() {
    if (!table) table = shared_ptr<CrossProduct>(new CrossProduct(child->get_table(), child2->get_table()));
    return table;
  }
  
  shared_ptr<Register> get_register(Attr attr) {
      auto aliases1 = child->aliases();
      if (aliases1.find(attr.alias) != aliases1.end()) return child->get_register(attr);	// register must be in child1
      return child2->get_register(attr);	// register must be in child2
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
  
  virtual int get_size(int threshold=-1) {
    (void) threshold;
    return child->get_size() * child2->get_size();
  }
        
  string representation() {return child->representation() + "x" + child2->representation();}
  
  string type_str() {return "CROSSPRODUCT";}
};

struct HASHJOIN:CROSSPRODUCT {
  shared_ptr<HashJoin> table;
  Attr_Binding binding;
  
  HASHJOIN(shared_ptr<Parser_Result> parser_result, shared_ptr<Database> db, shared_ptr<Join_Graph_Node> child, shared_ptr<Join_Graph_Node> child2, Attr_Binding binding) : CROSSPRODUCT(parser_result, db, child, child2), binding(binding) {}
  
  shared_ptr<Operator> get_table() {
    if (!table) {
      auto child_table1 = child->get_table();
      auto child_table2 = child2->get_table();
      auto reg1 = get_register(binding.attr1);
      auto reg2 = get_register(binding.attr2);;
      table = shared_ptr<HashJoin>(new HashJoin(child_table1, child_table2, reg1, reg2));
    }
    return table;
  }
  
  int get_size(int threshold=-1) {
    return get_table()->size(threshold);
  }
  
  string representation() {return child->representation() + " |x| " + child2->representation();}
  
  string type_str() {return "HASHJOIN";}
};

#endif