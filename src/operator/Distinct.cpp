#include "operator/Distinct.hpp"
#include <iostream>
#include "Register.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
Distinct::Distinct(shared_ptr<Operator> input, vector<shared_ptr<Register>> output)
   : input(input),output(output)
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
  bool n;
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
vector<shared_ptr<Register>> Distinct::getOutput() const
   // Get all produced values
{
   return output;
}
//---------------------------------------------------------------------------
