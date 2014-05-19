#ifndef H_Assertion
#define H_Assertion

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

void assertion(bool condition, string explanation) {
  if (!condition) {
   cout << endl << "!!!!! " << explanation << " !!!!!" << endl << endl; 
   assert(false);
  }
}

#endif