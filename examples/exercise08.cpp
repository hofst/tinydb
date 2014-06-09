#include "Parser.hpp"
#include "Query_Plan.hpp"

using namespace std;

int main() {
  //shared_ptr<Parser_Result> parser_result(new Parser_Result(string("SELECT s.matrnr,s.name,v.titel FROM studenten s,hoeren h,vorlesungen v WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer"), string("data/uni")));
  //shared_ptr<Parser_Result> parser_result(new Parser_Result(string("SELECT * FROM lineitem l,orders o,customer c WHERE l.l_orderkey=o.o_orderkey AND o.o_custkey=c.c_custkey AND c.c_name=Customer#000014993"), string("data/tpch/tpch")));
  shared_ptr<Parser_Result> parser_result(new Parser_Result(string("SELECT * FROM studenten s,hoeren h WHERE s.matrnr=h.matrnr"), string("data/uni")));
  
  auto query_plan = Query_Plan(parser_result);
  //auto join_graph = query_plan.cp_sub();
  auto join_graph = query_plan.canonical_optimized();
  //join_graph->print();
  join_graph->output_result();
}
