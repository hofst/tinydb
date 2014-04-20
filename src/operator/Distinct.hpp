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
   std::unique_ptr<Operator> input;
   /// The output
   std::vector<const Register*> output;
   std::unordered_set<std::string> known_values;

   public:
   /// Constructor
   Distinct(std::unique_ptr<Operator>&& input,const std::vector<const Register*>& output);
   /// Destructor
   ~Distinct();

   /// Open the operator
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get all produced values
   std::vector<const Register*> getOutput() const;
};
//---------------------------------------------------------------------------
#endif
