#ifndef H_Query_plan
#define H_Query_plan

#include <iostream>
#include "Parser.hpp"


struct Query_Plan {
  Parser_Result parser_result;
  
  Query_Plan(Parser_Result parser_result) : parser_result(parser_result) {}
  
  void run() {
    cout << "***** Running Query Plan *****";
  }
  
  void apply_goo() {
    cout << "***** Creating GOO Query Plan *****";
  }
};

#endif