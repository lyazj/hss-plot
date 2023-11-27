#include "TreeInput.h"
#include <iostream>

using namespace std;

int main()
{
  TreeInput eviewer("Events");
  eviewer.add_filename("../example/wzdd-nano.root");
  eviewer.add_filename("../example/wzdd-tree.root");
  size_t b_pdgid = eviewer.add_branch("GenPart_pdgId");
  while(eviewer.next()) {
    size_t ngenpar;
    int *pdgid = (int *)eviewer.get_branch_data(b_pdgid, &ngenpar);
    cout << ngenpar << '\t' << eviewer.get_branch_nelem_max(b_pdgid) << '\t';
    for(size_t i = 0; i < ngenpar; ++i) cout << pdgid[i] << ' ';
    cout << endl;
  }
  return 0;
}
