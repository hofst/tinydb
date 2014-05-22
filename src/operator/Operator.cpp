#include "operator/Operator.hpp"
//---------------------------------------------------------------------------
Operator::Operator()
   // Constructor
{
}
//---------------------------------------------------------------------------
Operator::~Operator()
   // Destructor
{
}
//---------------------------------------------------------------------------
int Operator::size() {
    int s = 0;
    open();
    while(next()) s++;
    close();  
    return s;
}