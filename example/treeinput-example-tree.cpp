#include "../include/TreeInput.h"
#include <iostream>

using namespace std;

int main()
{
  TreeInput eviewer("Events");
  eviewer.add_filename("../example/wzdd-tree.root");
  eviewer.add_filename("../example/wzdd-tree.root");
  size_t b_phi = eviewer.add_branch("ak15_phi");
  while(eviewer.next()) {
    float *phi = (float *)eviewer.get_branch_data(b_phi);
    cout << *phi << endl;
  }
  return 0;
}
