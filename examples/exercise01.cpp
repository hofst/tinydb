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
#include <assert.h>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace std;
//---------------------------------------------------------------------------
void task1()
{
  cout << endl << "***** Find all students that attended the lectures together with Schopenhauer, excluding Schopenhauer himself. *****" << endl;
  
  // open database
  Database db;
  db.open("data/uni");
  
  // FROM
  Table& s1=db.getTable("studenten");
  Table& h1=db.getTable("hoeren");
  Table& s2=db.getTable("studenten");
  Table& h2=db.getTable("hoeren");

  unique_ptr<Tablescan> scan_s1(new Tablescan(s1));
  unique_ptr<Tablescan> scan_h1(new Tablescan(h1));
  unique_ptr<Tablescan> scan_s2(new Tablescan(s2));
  unique_ptr<Tablescan> scan_h2(new Tablescan(h2));

  // columns
  const Register* s1_name=scan_s1->getOutput("name");
  const Register* s2_name=scan_s2->getOutput("name");
  const Register* s1_matrnr=scan_s1->getOutput("matrnr");
  const Register* h1_matrnr=scan_h1->getOutput("matrnr");
  const Register* s2_matrnr=scan_s2->getOutput("matrnr");
  const Register* h2_matrnr=scan_h2->getOutput("matrnr");
  
  // filter s1 and s2 for (in)equality to "Schopenhauer"
  Register schopenhauer; schopenhauer.setString("Schopenhauer");
  
  unique_ptr<Chi> s1_filter(new Chi(move(scan_s1),Chi::Equal,s1_name,&schopenhauer));
  const Register* s1_filtered_result=s1_filter->getResult();
  unique_ptr<Selection> s1_filtered(new Selection(move(s1_filter),s1_filtered_result));
 
  unique_ptr<Chi> s2_filter(new Chi(move(scan_s2),Chi::NotEqual,s2_name,&schopenhauer));
  const Register* s2_filtered_result=s2_filter->getResult();
  unique_ptr<Selection> s2_filtered(new Selection(move(s2_filter),s2_filtered_result));
  
  // join s1, h1
  unique_ptr<HashJoin> hjoin1(new HashJoin(move(s1_filtered),move(scan_h1), s1_matrnr, h1_matrnr));
  // join s2, h2
  unique_ptr<HashJoin> hjoin2(new HashJoin(move(s2_filtered),move(scan_h2), s2_matrnr, h2_matrnr));
  // cross product of both joins
  unique_ptr<CrossProduct> cp(new CrossProduct(move(hjoin1),move(hjoin2)));
  // Project to s2.name
  unique_ptr<Projection> project(new Projection(move(cp),{s2_name}));
  // distinct s2.name
  unique_ptr<Distinct> dist(new Distinct(move(project),{s2_name}));
  
  // print result
  Printer out(move(dist));
  out.open();
  while (out.next());
  out.close();
}

void task2()
{
  cout << endl << "***** Find all professors whose lectures attended at least two students. *****" << endl;

  // open database
  Database db;
  db.open("data/uni");
  
  // FROM
  Table& p=db.getTable("professoren");
  Table& v=db.getTable("vorlesungen");
  Table& h1=db.getTable("hoeren");
  Table& h2=db.getTable("hoeren");
  
  unique_ptr<Tablescan> scan_p(new Tablescan(p));
  unique_ptr<Tablescan> scan_v(new Tablescan(v));
  unique_ptr<Tablescan> scan_h1(new Tablescan(h1));
  unique_ptr<Tablescan> scan_h2(new Tablescan(h2));

  // columns
  const Register* p_persnr=scan_p->getOutput("persnr");
  const Register* v_gelesenvon=scan_v->getOutput("gelesenvon");
  const Register* h1_vorlnr=scan_h1->getOutput("vorlnr");
  const Register* h2_vorlnr=scan_h2->getOutput("vorlnr");
  const Register* v_vorlnr=scan_v->getOutput("vorlnr");
  const Register* p_name=scan_p->getOutput("name");
  const Register* h1_matrnr=scan_h1->getOutput("matrnr");
  const Register* h2_matrnr=scan_h2->getOutput("matrnr");
  
  // join p, v
  unique_ptr<HashJoin> pv(new HashJoin(move(scan_p),move(scan_v), p_persnr, v_gelesenvon));
  // join pv, h1
  unique_ptr<HashJoin> pvh1(new HashJoin(move(pv),move(scan_h1), v_vorlnr, h1_vorlnr));
  // join pvh1, h2
  unique_ptr<HashJoin> pvh1h2(new HashJoin(move(pvh1),move(scan_h2), v_vorlnr, h2_vorlnr));
  // Both students must be different. Not optimal to do it here, but it works ;)
  unique_ptr<Chi> student_filter(new Chi(move(pvh1h2),Chi::NotEqual,h1_matrnr,h2_matrnr));
  const Register* student_filtered_result=student_filter->getResult();
  unique_ptr<Selection> students_filtered(new Selection(move(student_filter),student_filtered_result));
  // Project to p.name
  unique_ptr<Projection> project(new Projection(move(students_filtered),{p_name}));
  // distinct s2.name
  unique_ptr<Distinct> dist(new Distinct(move(project),{p_name}));
  
  // print result
  Printer out(move(dist));
  out.open();
  while (out.next());
  out.close();

}
/*
enum TOKENS {
 STRING,
 COMMA,
 DOT,
 EQUAL,
 SELECT,
 FROM,
 WHERE
};
class Token {
 public:
   string value;
   regex reg;
   TOKENS type; 
   Token(string reg, TOKENS type) : reg( regex("$(" + reg + ")", regex_constants::ECMAScript | regex_constants::icase)), type(type) {};
};

vector<Token> lexer(string query) {  
  vector<Token> tokens;
   tokens.push_back(Token("[a-zA-Z0-9]+", STRING));
   tokens.push_back(Token("\\,", COMMA));
   tokens.push_back(Token("\\,", DOT));
   tokens.push_back(Token("\\,", EQUAL));
   tokens.push_back(Token("\\,", SELECT));
   tokens.push_back(Token("\\,", FROM));
   tokens.push_back(Token("\\,", WHERE));
}
*/

vector<string> split_match(string str, string match) {
  // extract regex tokens from a string
  regex rx_match("(" + match + ")", regex_constants::ECMAScript | regex_constants::icase);
  smatch matches;
  vector<string> splits;
  
  while (str != "") {
    regex_search(str, matches, rx_match,  regex_constants::match_flag_type::match_continuous);
    if (!matches[1].length()) {
      str = str.substr(1, str.length() -1);
    } else {
      splits.push_back(matches[1]);
      str = str.substr(matches[1].length(), str.length() - matches[1].length());  
    }
  }
  return splits;
}

void parser(string query) {
  // regex definitions
  regex rx_sql( "select (\\*|\\w+(?:,\\w+)*) from (\\w+ \\w+(?:,\\w+ \\w+)*) where (\\w+\\.\\w+=(?:\\w+\\.\\w+|\\w+)(?: and \\w+\\.\\w+=(?:\\w+\\.\\w+|\\w+))*)",
			regex_constants::ECMAScript | regex_constants::icase);
  smatch matches;
  assert(regex_search(query, matches, rx_sql,  regex_constants::match_flag_type::match_continuous));
  
  // extract main parts of the sql query
  string select = matches[1];
  string from = matches[2];
  string where = matches[3];
  
  /* extract tokens from main parts */
  
  map<string, string> attribute_to_alias; // maps an attribute to its aliased relation
  
  // select
  auto selected_attributes = split_match(select, "\\w+");  // list of attributes
  
  // from
  map<string, string> alias_to_relation;  // maps an alias to its relation
  auto tokens = split_match(from, "\\w+");
  for (unsigned i=0; i<tokens.size(); i+=2) {
    alias_to_relation[tokens[i+1]] = tokens[i];
  }
  
  // where
  vector<vector<string>> attribute_bindings;  // first element := attribute; second element := attribute
  vector<vector<string>> constant_bindings;  // first element := attribute; constant
  
  tokens = split_match(where, "(\\w|\\.)+");
  for (unsigned i=0; i<tokens.size(); i+=3) {
    vector<string> binding;
    
    binding.push_back(split_match(tokens[i], "\\w+")[1]);
    attribute_to_alias[split_match(tokens[i], "\\w+")[1]] = split_match(tokens[i], "\\w+")[0];
    
    if (tokens[i+1].find(".")!=string::npos) {
      // attribute binding
      attribute_to_alias[split_match(tokens[i+1], "\\w+")[1]] = split_match(tokens[i+1], "\\w+")[0];
      binding.push_back(split_match(tokens[i+1], "\\w+")[1]);
      attribute_bindings.push_back(binding);
    } else {
     // constant bin#include <assert>ding 
      binding.push_back(tokens[i+1]);
      constant_bindings.push_back(binding);
    }
  }
  
  // check if relations and attributes exist
  Database db;
  db.open("data/uni");
  
  for (auto& it : alias_to_relation) {
      assert(db.hasTable(it.second));  // make sure relations exist
  }
  
  for (auto& it : attribute_to_alias) {
      auto table = db.getTable(alias_to_relation[it.second]);
      assert(table.findAttribute(it.first) > -1);  // make sure attributes exist
  }
  
  cout << endl << "***** sucessfully parsed the following query: *****" << endl << query << endl;
}

int main() {
  task1();
  task2();
  parser(string("SELECT matrnr,name from studenten s,hoeren h where s.matrnr=h.matrnr and s.name=2"));
}

//---------------------------------------------------------------------------
