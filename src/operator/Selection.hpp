#ifndef H_operator_Selection
#define H_operator_Selection
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include <memory>
//---------------------------------------------------------------------------
/// A selection
class Selection : public Operator
{
   public:
   /// The input
   shared_ptr<Operator> input;
   /// Registers of the condition
   shared_ptr<Register> condition;
   /// Second register for implicit equal tests
   shared_ptr<Register> equal;
   /// Constructor. Condition must be a bool value
   Selection(shared_ptr<Operator> input, shared_ptr<Register> condition);
   /// Constructor. Registers a and b are compared
   Selection(shared_ptr<Operator>, shared_ptr<Register> a, shared_ptr<Register> b);
   /// Destructor
   ~Selection();

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get all produced values
   vector<shared_ptr<Register>> getOutput() const;
};
//---------------------------------------------------------------------------
#endif
