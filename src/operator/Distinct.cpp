#include "operator/Distinct.hpp"
#include <iostream>
#include "Register.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
Distinct::Distinct(unique_ptr<Operator>&& input,const vector<const Register*>& output)
   : input(move(input)),output(output)
   // Constructor
{
}
//---------------------------------------------------------------------------
Distinct::~Distinct()
   // Destructor
{
}
//---------------------------------------------------------------------------
void Distinct::open()
   // Open the operator
{
   input->open();
}
//---------------------------------------------------------------------------
bool Distinct::next()
   // Get the next tuple
{
  bool n ;
  while ((n = input->next()) && known_values.find(output[0]->getString()) != known_values.end());
  if (n)
    known_values.insert(output[0]->getString());
  return n;
}
//---------------------------------------------------------------------------
void Distinct::close()
   // Close the operator
{
   input->close();
}
//---------------------------------------------------------------------------
vector<const Register*> Distinct::getOutput() const
   // Get all produced values
{
   return output;
}
//---------------------------------------------------------------------------
