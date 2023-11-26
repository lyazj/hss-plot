#include "../include/TreeInput.h"
#include <iostream>

using namespace std;

int main()
{
  TreeInput event("Inputs");
  event.add_filename("../example/wzdd-tree.root");
  event.add_filename("../example/wzdd-tree.root");
  size_t b_phi = event.add_branch("ak15_phi");
  while(event.next()) {
    float *phi = (float *)event.get_branch_data(b_phi);
    cout << *phi << endl;
  }
  return 0;
}
