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


struct Query_Plan {
  Parser_Result parser_result;
  
  Query_Plan(Parser_Result parser_result) : parser_result(parser_result) {}
  
  void run() {
    cout << endl << "***** Running Query Plan *****" << endl;
    
    /* Run a query in form of Parser_Result on tinydb */
    
    cout << endl << "***** Running Query: *****" << endl;
    
    // Open Database
    Database db;
    db.open(parser_result.db);
    
    // Create TableScans
    map<string, unique_ptr<Tablescan>> alias_to_tables;
    for (string alias : parser_result.aliases) {
      alias_to_tables[alias] = unique_ptr<Tablescan> (new Tablescan(db.getTable(parser_result.alias_to_relation[alias])));
    }
    
    // Create Registers
    map<Attr, const Register*> attr_to_register;
    for (Attr attr : parser_result.attributes) {
      attr_to_register[attr] = alias_to_tables[attr.alias]->getOutput(attr.name);
    }
    
    // Constant Bindings
    map<string, unique_ptr<Operator>> alias_to_filtered_tables;  // basically convert from Tablescan-map to Operator-map
    for (auto& alias_table : alias_to_tables) {
      alias_to_filtered_tables[alias_table.first] = move(alias_table.second);
    }
    
    map<string, Register> equal_constant_registers;
    for (auto& binding : parser_result.constant_bindings) {
      equal_constant_registers[binding.constant] = Register(binding.constant);
      unique_ptr<Chi> filter(new Chi(move(alias_to_filtered_tables[binding.attr.alias]),Chi::Equal,attr_to_register[binding.attr],&equal_constant_registers[binding.constant]));
      const Register* filtered_register=filter->getResult();
      alias_to_filtered_tables[binding.attr.alias] = unique_ptr<Selection> (new Selection(move(filter),filtered_register));  
    }
    
    // Canonical CrossProduct
    unique_ptr<Operator> result;
    for (auto& alias_table : alias_to_filtered_tables) {
      if (!result) {
      result = move(alias_table.second);  // initialize
      } else {
      unique_ptr<Operator> tmp(new CrossProduct(move(alias_table.second), move(result)));
      result = move(tmp);
      }
    }
    
    // Canonical Selection (join bindings)
    for (auto& binding : parser_result.attr_bindings) {
      unique_ptr<Chi> filter(new Chi(move(result),Chi::Equal,attr_to_register[binding.attr1],attr_to_register[binding.attr2]));
      const Register* filtered_register=filter->getResult();
      result = move(unique_ptr<Selection> (new Selection(move(filter),filtered_register)));  
    }
    
    
    // Projections
    vector<const Register*> selected_registers;
    for (auto attr : parser_result.selected_attributes) {
	selected_registers.push_back(attr_to_register[attr]);
    }
    Printer out (move(result), selected_registers);
    
    // Print Result
    out.open();
    while (out.next());
    out.close();
  }
  
  void apply_goo() {
    cout << endl << "***** Creating GOO Query Plan *****" << endl;
  }

};

#endif