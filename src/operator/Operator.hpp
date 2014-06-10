#ifndef H_operator_Operator
#define H_operator_Operator
//---------------------------------------------------------------------------
#include <vector>
#include <string>
#include <memory>
using namespace std;
//---------------------------------------------------------------------------
class Register;
//---------------------------------------------------------------------------
/// Operator interface
class Operator
{
   public:
   /// Constructor
   Operator();
   /// Destructor
   virtual ~Operator();

   /// Open the operator
   virtual void open() = 0;
   /// Produce the next tuple
   virtual bool next() = 0;
   /// Close the operator
   virtual void close() = 0;
   
   int size_buffer;
   
   int size(int threshold=-1);

   /// Get all produced values
   virtual vector<shared_ptr<Register>> getOutput() const = 0;
};
//---------------------------------------------------------------------------
#endif
