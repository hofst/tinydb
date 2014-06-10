#ifndef H_operator_HashJoin
#define H_operator_HashJoin
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include "Register.hpp"
#include <memory>
#include <unordered_map>
//---------------------------------------------------------------------------
/// A hash join
class HashJoin : public Operator
{
   private:
   /// The registers
   shared_ptr<Register> leftValue, rightValue;
   /// The copy mechanism
   std::vector<shared_ptr<Register>> leftRegs;
   /// The hashtable
   std::unordered_multimap<Register,std::vector<Register>,Register::hash> table;
   /// Iterator
   std::unordered_multimap<Register,std::vector<Register>,Register::hash>::const_iterator iter,iterLimit;

   public:
   /// The input
   shared_ptr<Operator> left,right;
   /// Constructor
   HashJoin(shared_ptr<Operator> left, shared_ptr<Operator> right, shared_ptr<Register> leftValue, shared_ptr<Register> rightValue);
   /// Destructor
   ~HashJoin();

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get all produced values
   vector<std::shared_ptr<Register>> getOutput() const;
};
//---------------------------------------------------------------------------
#endif
