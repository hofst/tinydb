#ifndef H_operator_CrossProduct
#define H_operator_CrossProduct
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include <memory>
//---------------------------------------------------------------------------
/// A cross product
class CrossProduct : public Operator
{
   private:
   

   public:
   /// The input
   shared_ptr<Operator> left,right;
   /// Read the left side?
   bool readLeft;
   /// Constructor
   CrossProduct(shared_ptr<Operator> left,shared_ptr<Operator> right);
   /// Destructor
   ~CrossProduct();

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get all produced values
   vector<std::shared_ptr<Register>> getOutput() const;
   /// Get one produced value
   shared_ptr<Register> getOutput(const string& name) const;
};
//---------------------------------------------------------------------------
#endif
