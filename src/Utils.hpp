#ifndef H_Utils
#define H_Utils

#include <iostream>
#include <string>
#include <assert.h>
#include <set>
#include <iterator>
#include "operator/Operator.hpp"

using namespace std;

int pow(int b, int e) {
  int result = 1;
  for (int i=0; i<e; i++) result *= b;
  return result;
}

void assertion(bool condition, string explanation="") {
  if (!condition) {
   cout << endl << "!!!!! " << explanation << " !!!!!" << endl << endl; 
   assert(false);
  }
}

template<typename T>
T set_at(set<T> s, int i) {
  auto it = s.begin();
  advance(it, i);
  return *it;
}

template<typename T>
set<T> intersection(set<T> s1, set<T> s2) {
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

int fak(int n) {
    if (n) return n * fak(n-1);
    return 1;
}

string set_representation(set<string> s) {
  string result;
  for (auto e : s) {
    result += e;
  }
  return result;
}

template<typename T>
set<T> map_values(map<string,T> m) {
  set<T> result;
  for (auto e : m) {
    result.insert(e.second);
  }
  return move(result);
}

template<typename T>
set<T> merge_sets(set<T> s1, set<T> s2) {
  set<T> result;
  result.insert(s1.begin(), s1.end());
  result.insert(s2.begin(), s2.end());
  return move(result);
}

template<typename T>
set<T> int_to_set(set<T> s, int n) {
  /* return subset that is binary encoded as int */
  set<T> result;
  for (int i = 0; i < int(s.size()); i++) {
    if (n & pow(2,i)) {
      result.insert(set_at(s, i));
    }
  }
  return move(result);
}

template<typename T>
set<T> inverse_set(set<T> s, set<T> s1) {
  /* return subset of s when removing s1 */
  for (auto e : s1)  s.erase(e);
  return s;
}

template<typename T>
set<pair<set<T>, set<T>>> partitions(set<T> s) {
  /* return all partitions into 2 subsets */
  set<pair<set<T>, set<T>>> result;
  
  if (s.size() > 1) {	// s must contain at least 2 elements or partition won't be possible
    for (int i = 1; i < (pow(2,s.size())-1) / 2; i++) {  // avoid empty and full subsets or partition won't be possible. Also do not consider order of paris (divide by 2 to omit complements)
      auto s1 = int_to_set<T>(s, i);
      auto s2 = inverse_set<T>(s, s1);
      result.insert(pair<set<T>, set<T>>(s1,s2));
    }
  }
  
  return move(result);
}

#endif