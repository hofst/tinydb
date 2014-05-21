#include "Parser.hpp"
#include "Query_Plan.hpp"

using namespace std;

int main() {
  auto parser_result = Parser_Result(string("SELECT s.matrnr,s.name,v.titel FROM studenten s,hoeren h,vorlesungen v WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer"),
				     string("data/uni"));
//   auto parser_result = Parser_Result(string("SELECT * FROM lineitem l,orders o,customer c WHERE l.l_orderkey=o.o_orderkey AND o.o_custkey=c.c_custkey AND c.c_name=Customer#000014993"),
//   				       string("data/tpch/tpch"));

  
  auto query_plan = Query_Plan(parser_result);
  //parser_result.build_query_graph();
  //query_plan.apply_canonical_optimized();
  query_plan.apply_goo();
  query_plan.output_result();
  query_plan.get_join_graph()->print();
}

int test() {
  Database db;
  db.open("data/uni");
  unique_ptr<Tablescan> a (new Tablescan(db.getTable("studenten")));
  set<string> s {"a"};
  
  shared_ptr<Join_Graph_Node> n(new Join_Graph_Node(move(a), s, Node_Type::LEAF));
  shared_ptr<Join_Graph_Node> n2 = n;
  unique_ptr<Operator> c = move(n->table);
  cout << n.use_count() << endl;
  n2.reset();
  n.reset();
  
  cout << n.use_count() << endl;
  return 0;
}