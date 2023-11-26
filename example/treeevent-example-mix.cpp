#include "../include/TreeEvent.h"
#include <iostream>

using namespace std;

int main()
{
  TreeEvent event("Events");
  event.add_filename("../example/wzdd-nano.root");
  event.add_filename("../example/wzdd-tree.root");
  size_t b_pdgid = event.add_branch("GenPart_pdgId");
  while(event.next()) {
    size_t ngenpar;
    int *pdgid = (int *)event.get_branch_data(b_pdgid, &ngenpar);
    cout << ngenpar << '\t' << event.get_branch_nelem_max(b_pdgid) << '\t';
    for(size_t i = 0; i < ngenpar; ++i) cout << pdgid[i] << ' ';
    cout << endl;
  }
  return 0;
}
