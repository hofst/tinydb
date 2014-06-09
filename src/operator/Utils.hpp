#ifndef H_Utils
#define H_Utils

#include <iostream>
#include <string>
#include <assert.h>
#include <set>
#include <math.h> 

using namespace std;

void assertion(bool condition, string explanation) {
  if (!condition) {
   cout << endl << "!!!!! " << explanation << " !!!!!" << endl << endl; 
   assert(false);
  }
}

template<typename T>
set<T>  intersection(set<T> s1, set<T> s2) {
  set<T> result;
  for (auto _s1 : s1) {
    for (auto _s2 : s2) {
      if (_s1 == _s2) result.insert(move(_s1));
    }
  }
  return move(result);
}

template<typename T>
set<pair<T,T>>  all_pairs_in_set(set<T> s) {
  set<pair<T,T>> result;
  set<T> visited;
  for (auto _s1 : s) {
    visited.insert(_s1);
    for (auto _s2 : s) {
      if (visited.find(_s2) != visited.end()) continue;
      result.insert(move(pair<T,T>(_s1,_s2)));
    }
  }
  return move(result);
}

template<typename T>
set<T> int_to_set(set<T> s, unsigned n) {
  /* return subset that is binary encoded as int */
  set<T> result;
  for (int i = 0; pow(2,i) < n; i++) {
    if (n & pow(2^i)) {
      result.insert(s[i]);
    }
  }
  return move(result);
}

template<typename T>
set<pair<set<T>, set<T>>> partitions(set<T> s) {
  /* return all partitions into 2 subsets */
  set<pair<set<T>, set<T>>> result;
  
  
  
  return move(result);
}

#endif