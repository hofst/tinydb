#ifndef H_operator_Chi
#define H_operator_Chi
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include "Register.hpp"
#include <memory>
//---------------------------------------------------------------------------
/// A chi operator
class Chi : public Operator
{
   public:
   /// An operation
   typedef void (*Operation)(const Register& a,const Register& b,Register& result);
   /// Add
   static void Add(const Register& a,const Register& b,Register& result);
   /// Divide
   static void Div(const Register& a,const Register& b,Register& result);
   /// Compare
   static void Equal(const Register& a,const Register& b,Register& result);
   /// Compare
   static void NotEqual(const Register& a, const Register& b, Register& result);
   /// Compare
   static void Less(const Register& a,const Register& b,Register& result);
   /// Compare
   static void LessOrEqual(const Register& a,const Register& b,Register& result);

   private:
   /// The input
   shared_ptr<Operator> input;
   /// The operation
   Operation op;
   /// The input of the operation
   shared_ptr<Register> left,right;
   /// The output of the operation
   shared_ptr<Register> output;

   public:
   /// Constructor. Sample usage: new Chi(move(input),Chi::Add,reg1,reg2);
   Chi(shared_ptr<Operator> input,Operation op,shared_ptr<Register> left,shared_ptr<Register> right);
   /// Destructor
   ~Chi();

   /// Open the operation
   void open();
   /// Get the next tuple
   bool next();
   /// Close the operator
   void close();

   /// Get the produced value
   shared_ptr<Register> getResult() const { return output; }
   /// Get all produced values
   vector<std::shared_ptr<Register>> getOutput() const;
   /// Get one produced value
   shared_ptr<Register> getOutput(const string& name) const;
};
//---------------------------------------------------------------------------
#endif
