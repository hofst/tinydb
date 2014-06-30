#include "Parser.hpp"
#include "Query_Plan.hpp"

using namespace std;

int main() {
  //shared_ptr<Parser_Result> parser_result(new Parser_Result(string("SELECT s.matrnr,s.name,v.titel,p.name,vor.nachfolger FROM studenten s,hoeren h,vorlesungen v,professoren p,voraussetzen vor WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer AND p.persnr=v.gelesenvon AND vor.vorgaenger=v.vorlnr"), string("data/uni")));
  shared_ptr<Parser_Result> parser_result(
    new Parser_Result(string(
      "SELECT * FROM lineitem l,orders o,customer c,partsupp ps,part p WHERE l.l_orderkey=o.o_orderkey AND o.o_custkey=c.c_custkey AND c.c_name=Customer#000014993 AND l.l_partkey=ps.ps_partkey AND p.p_partkey=ps.ps_partkey AND p.p_brand=Brand#41"
    ), string("data/tpch/tpch")));
  
  auto query_plan = Query_Plan(parser_result);
  auto join_graph = query_plan.quick_pick_multi();
  //auto join_graph = query_plan.dp_sub();
  //auto join_graph = query_plan.canonical_optimized();
  //auto join_graph = query_plan.goo();
  join_graph->print();
  join_graph->output_result();
}
