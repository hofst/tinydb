#ifndef H_operator_Distinct
#define H_operator_Distinct
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include <memory>
#include <unordered_set>
#include <string>
//---------------------------------------------------------------------------
class Distinct : public Operator
{
   private:
   /// The input
   shared_ptr<Operator> input;
   /// The output
   vector<shared_ptr<Register>> output;
   unordered_set<string> known_values;

   public:
   /// Constructor
   Distinct(shared_ptr<Operator> input, std::vector<shared_ptr<Register>> output);
   /// Destructor
   ~Distinct();

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
