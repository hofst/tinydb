#include "Parser.hpp"
#include "Query_Plan.hpp"

using namespace std;

int main() {
  //auto parser_result = Parser_Result(string("SELECT s.matrnr,s.name,v.titel FROM studenten s,hoeren h,vorlesungen v WHERE s.matrnr=h.matrnr AND h.vorlnr=v.vorlnr AND s.name=Schopenhauer"),
	//			     string("data/uni"));
  
  // c_name=Customer#000014993		c_custkey=14993		o_orderkey=420545
  auto parser_result = Parser_Result(string("SELECT * FROM lineitem l,orders o,customer c WHERE l.l_orderkey=o.o_orderkey AND o.o_custkey=c.c_custkey AND c.c_name=Customer#000014993"),
   				       string("data/tpch/tpch"));

  auto query_plan = Query_Plan(parser_result);
  //parser_result.build_query_graph();
  //query_plan.apply_canonical_optimized();
  query_plan.apply_goo();
  query_plan.get_join_graph()->print();
  query_plan.output_result();
}

int simple_test() {
  Database db;
  db.open("data/tpch/tpch");
  
  // FROM
  Table& s1=db.getTable("lineitem");
  unique_ptr<Tablescan> scan_s1(new Tablescan(s1));
  const Register* r1=scan_s1->getOutput("l_orderkey");
  auto r2 = Register("0");
    r2.setInt(420545);
  
  
  unique_ptr<Chi> filter(new Chi(
	move(scan_s1),
	Chi::Equal,
	r1,
	&r2));
    
    const Register* filtered_register=filter->getResult();
    auto table = unique_ptr<Selection> (new Selection(move(filter),filtered_register)); 
    
    
    
  // print result
  Printer out(move(table), {r1});
  out.open();
  while (out.next());
  out.close();
  return 0;
}

