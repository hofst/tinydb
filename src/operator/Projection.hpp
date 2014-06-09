#ifndef H_operator_Projection
#define H_operator_Projection
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include <memory>
//---------------------------------------------------------------------------
/// A projection
class Projection : public Operator
{
   private:
   /// The input
   shared_ptr<Operator> input;
   /// The output
   vector<shared_ptr<Register>> output;

   public:
   /// Constructor
   Projection(shared_ptr<Operator> input, vector<shared_ptr<Register>> output);
   /// Destructor
   ~Projection();

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get all produced values
   vector<shared_ptr<Register>> getOutput() const;
   /// Get one produced value
   shared_ptr<Register> getOutput(const string& name) const;
};
//---------------------------------------------------------------------------
#endif
