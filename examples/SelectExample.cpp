#include "Database.hpp"
#include "operator/Tablescan.hpp"
#include "operator/Selection.hpp"
#include <iostream>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
int main()
{
   Database db;
   db.open("data/uni");
   Table& studenten=db.getTable("studenten");

   unique_ptr<Tablescan> scan(new Tablescan(studenten));
   shared_ptr<Register> name=scan->getOutput("name");
   shared_ptr<Register> semester=scan->getOutput("semester");
   shared_ptr<Register> two (new Register); two->setInt(2);
   Selection select(move(scan),semester,two);

   select.open();
   while (select.next())
      cout << name->getString() << endl;
   select.close();
}
//---------------------------------------------------------------------------
