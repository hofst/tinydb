#include "operator/Printer.hpp"
#include "Register.hpp"
#include <iostream>
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
Printer::Printer(shared_ptr<Operator> input)
   : input(input),toPrint(this->input->getOutput())
   // Constructor
{
}
//---------------------------------------------------------------------------
Printer::Printer(shared_ptr<Operator> input,vector<shared_ptr<Register>> toPrint)
   : input(input),toPrint(toPrint)
   // Constructor
{
  if (toPrint.size() == 0) {
    this->toPrint = this->input->getOutput();
  }
}
//---------------------------------------------------------------------------
void Printer::open()
   // Open the operator
{
   input->open();
}
//---------------------------------------------------------------------------
bool Printer::next()
   // Get the next tuple
{
   // Produce the next tuple
   if (!input->next())
      return false;

   // Print the entries
   for (unsigned index=0,limit=toPrint.size();index<limit;++index) {
      if (index) cout << ' ';
      const Register& r=*toPrint[index];
      switch (r.getState()) {
         case Register::State::Unbound: cout << "null"; break;
         case Register::State::Int: cout << r.getInt(); break;
         case Register::State::Double: cout << r.getDouble(); break;
         case Register::State::Bool: cout << (r.getBool()?"true":"false"); break;
         case Register::State::String: cout << r.getString(); break;
      }
   }
   cout << endl;

   return true;
}
//---------------------------------------------------------------------------
void Printer::close()
   // Close the operator
{
   input->close();
}
//---------------------------------------------------------------------------
vector<shared_ptr<Register>> Printer::getOutput() const
   // Get all produced values
{
   return input->getOutput();
}
//---------------------------------------------------------------------------
shared_ptr<Register> Printer::getOutput(const std::string& name) const
   // Get one produced value
{
   return input->getOutput(name);
}
//---------------------------------------------------------------------------
