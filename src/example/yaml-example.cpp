#include <yaml-cpp/yaml.h>
#include <iostream>

using namespace std;

int main()
{
  YAML::Node samples = YAML::LoadFile("../example/2018_1L_mc.yaml");
  for(auto categories : samples) {
    cout << "- " << categories["name"] << endl;
    for(auto sample : categories["samples"]) {
      cout << "  + " << sample["name"] << endl;
    }
  }
  return 0;
}
