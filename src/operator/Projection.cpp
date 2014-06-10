#include "operator/Projection.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
Projection::Projection(shared_ptr<Operator> input,vector<shared_ptr<Register>> output)
   : input(input),output(output)
   // Constructor
{
}
//---------------------------------------------------------------------------
Projection::~Projection()
   // Destructor
{
}
//---------------------------------------------------------------------------
void Projection::open()
   // Open the operator
{
   input->open();
}
//---------------------------------------------------------------------------
bool Projection::next()
   // Get the next tuple
{
   return input->next();
}
//---------------------------------------------------------------------------
void Projection::close()
   // Close the operator
{
   input->close();
}
//---------------------------------------------------------------------------
vector<shared_ptr<Register>> Projection::getOutput() const
   // Get all produced values
{
   return output;
}
//---------------------------------------------------------------------------
