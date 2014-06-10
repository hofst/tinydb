#include "Database.hpp"
#include "operator/Tablescan.hpp"
#include "operator/CrossProduct.hpp"
#include "operator/Selection.hpp"
#include "operator/Projection.hpp"
#include "operator/Printer.hpp"
#include "operator/HashJoin.hpp"
#include <iostream>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
int main()
{
   Database db;
   db.open("data/uni");
   Table& hoeren=db.getTable("hoeren");
   Table& vorlesungen=db.getTable("vorlesungen");

   shared_ptr<Tablescan> scanHoeren(new Tablescan(hoeren));
   shared_ptr<Tablescan> scanVorlesungen(new Tablescan(vorlesungen));

   shared_ptr<Register> matrnr=scanHoeren->getOutput("matrnr");
   shared_ptr<Register> vorlnr=scanHoeren->getOutput("vorlnr");
   shared_ptr<Register> titel=scanVorlesungen->getOutput("titel");
   shared_ptr<Register> gelesenvon=scanVorlesungen->getOutput("gelesenvon");
   shared_ptr<Register> vorlnr2=scanVorlesungen->getOutput("vorlnr");

   shared_ptr<HashJoin> hj(new HashJoin(scanHoeren,scanVorlesungen, vorlnr, vorlnr2));
   shared_ptr<Projection> project(new Projection(hj,{matrnr, vorlnr, titel}));
   Printer out(project);

   out.open();
   while (out.next());
   out.close();
}
//---------------------------------------------------------------------------
