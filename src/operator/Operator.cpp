#include "operator/Operator.hpp"
//---------------------------------------------------------------------------
Operator::Operator() : size_buffer(-1)
   // Constructor
{
}
//---------------------------------------------------------------------------
Operator::~Operator()
   // Destructor
{
}
//---------------------------------------------------------------------------
int Operator::size(int threshold) {
    if (size_buffer == -1) {
      size_buffer = 0;
      open();
      while(next() && (threshold == -1 || size_buffer <= threshold))
	size_buffer++;
      close(); 
    }
    return size_buffer;
}