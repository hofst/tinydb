#include "Database.hpp"
#include "operator/Tablescan.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/Selection.hpp"
#include "operator/Projection.hpp"
#include "operator/Distinct.hpp"
#include "operator/Printer.hpp"
#include "operator/Chi.hpp"
#include <operator/HashJoin.hpp>
#include <iostream>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
int main()
{
   Database db;
   db.open("data/uni");
   Table& studenten=db.getTable("studenten");

   shared_ptr<Tablescan> scan(new Tablescan(studenten));
   shared_ptr<Register> semester=scan->getOutput("semester");
   shared_ptr<Register> name=scan->getOutput("name");

   // find all students where semester num. is not 2
   shared_ptr<Register> two (new Register); two->setInt(2);
   shared_ptr<Chi> chi(new Chi(scan,Chi::NotEqual,semester,two));
   
   shared_ptr<Register> chiResult=chi->getResult();

   shared_ptr<Selection> select(new Selection(chi,chiResult));
   shared_ptr<Projection> project(new Projection(select,{name}));

   Printer out(project);

   out.open();
   while (out.next());
   out.close();
}
//---------------------------------------------------------------------------
