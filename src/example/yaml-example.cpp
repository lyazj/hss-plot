#include "yaml-cpp/yaml.h"
#include <iostream>

using namespace std;

int main()
{
  YAML::Node samples = YAML::LoadFile("../example/1L_mc.yaml");
  for(auto categories : samples) {
    cout << "- " << categories.first << endl;
    for(auto prepid : categories.second) {
      cout << "  + " << prepid << endl;
    }
  }
  return 0;
}
