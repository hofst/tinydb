#ifndef H_Utils
#define H_Utils

#include <iostream>
#include <string>
#include <assert.h>
#include <set>
#include "operator/Operator.hpp"

using namespace std;

void assertion(bool condition, string explanation="") {
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

#endif