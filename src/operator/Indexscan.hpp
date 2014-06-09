#ifndef H_operator_IndexScan
#define H_operator_IndexScan
//---------------------------------------------------------------------------
#include "operator/Operator.hpp"
#include "Table.hpp"
//---------------------------------------------------------------------------
/// An indexscan operator
class Indexscan : public Operator
{
   private:
   /// The buffer size
   static const unsigned bufferSize = 4096;

   /// The table
   Table& table;
   /// The index
   std::map<Register,unsigned>& index;
   /// The iterator over the index
   std::map<Register,unsigned>::const_iterator iter,iterLimit;
   /// The bounds
   shared_ptr<Register> lowerBound, upperBound;
   /// Buffer pointers
   unsigned bufferStart,bufferStop;
   /// Construction helper
   string buf;
   /// The output
   vector<shared_ptr<Register>> output;
   /// A small buffer
   char buffer[bufferSize];

   public:
   /// Constructor
   Indexscan(Table& scale,unsigned indexAttribute, shared_ptr<Register> lowerBounds, shared_ptr<Register> upperBounds);
   /// Destructor
   ~Indexscan();

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
