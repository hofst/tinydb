#include "operator/HashJoin.hpp"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
HashJoin::HashJoin(shared_ptr<Operator> left, shared_ptr<Operator> right, shared_ptr<Register> leftValue, shared_ptr<Register> rightValue)
   : left(left),right(right),leftValue(leftValue),rightValue(rightValue)
   // Constructor
{
   vector<shared_ptr<Register>> lr = this->left->getOutput();
   for (auto iter=lr.begin(),limit=lr.end();iter!=limit;++iter)
      leftRegs.push_back(*iter);
}
//---------------------------------------------------------------------------
HashJoin::~HashJoin()
   // Destructor
{
}
//---------------------------------------------------------------------------
void HashJoin::open()
   // Open the operator
{
   left->open();
   right->open();
   table.clear();
}
//---------------------------------------------------------------------------
bool HashJoin::next()
   // Get the next tuple
{
   // First pass? Hash the left side
   if (table.empty()) {
      while (left->next()) {
         vector<Register> values;
         values.reserve(leftRegs.size());
         for (auto iter=leftRegs.begin(),limit=leftRegs.end();iter!=limit;++iter)
            values.push_back(**iter);
         table.insert(make_pair(*leftValue,move(values)));
      }
      if (table.empty()) return false;
      iter=iterLimit=table.end();
   }

   // Read the right hand side
   while (true) {
      // More matches?
      if (iter!=iterLimit) {
         const vector<Register>& values=(*iter).second;
         auto reader=values.begin();
         for (auto iter2=leftRegs.begin(),limit2=leftRegs.end();iter2!=limit2;++iter2,++reader)
            **iter2=*reader;
         ++iter;
         return true;
      }
      // Examine the next tuple
      if (!right->next()) {
         right->close();
         return false;
      }
      // Probe the hash table
      auto range=table.equal_range(*rightValue);
      iter=range.first; iterLimit=range.second;
   }
}
//---------------------------------------------------------------------------
void HashJoin::close()
   // Close the operator
{
   if (!table.empty()) {
      right->close();
      table.clear();
   }
   left->close();
}
//---------------------------------------------------------------------------
vector<shared_ptr<Register>> HashJoin::getOutput() const
   // Get all produced values
{
   auto result=left->getOutput(),other=right->getOutput();
   for (auto iter=other.begin(),limit=other.end();iter!=limit;++iter)
      result.push_back(*iter);
   return result;
}
//---------------------------------------------------------------------------
shared_ptr<Register> HashJoin::getOutput(const std::string& name) const
   // Get one produced value
{
  if (left->getOutput(name)) return left->getOutput(name);
  return right->getOutput(name);
}
//---------------------------------------------------------------------------
