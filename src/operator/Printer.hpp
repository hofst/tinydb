#ifndef H_operator_Printer
#define H_operator_Printer
//---------------------------------------------------------------------------
#include "Operator.hpp"
#include <memory>
//---------------------------------------------------------------------------
/// Prints tuple attributes
class Printer : public Operator
{
   private:
   /// The input
   shared_ptr<Operator> input;
   /// Registers to print
   vector<shared_ptr<Register>> toPrint;

   public:
   /// Constructor
   explicit Printer(shared_ptr<Operator> input);
   /// Constructor
   Printer(shared_ptr<Operator> input, vector<shared_ptr<Register>> toPrint);

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// CLose the operator
   void close();

   /// Get all produced values
   vector<shared_ptr<Register>> getOutput() const;
};
//---------------------------------------------------------------------------
#endif
