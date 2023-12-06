#include "TreeInput.h"
#include "HistOutput.h"

class Tree2Hist : public TreeInput, public HistOutput {
public:
  Tree2Hist() : TreeInput("Events"), HistOutput("ak15_phi", "number", "../example/ak15_phi.pdf") {
    add_branch("ak15_phi");
    add_curve("tree", true);
    add_curve("tree");
    add_curve("tree");
    set_boundary(-3.5, 3.5);
    bin();
    set_rangey(0.0, 200.0);
  }

  virtual bool process() override {
    fill_curve(0, *(float *)get_branch_data(0), 1.0);
    fill_curve(1, *(float *)get_branch_data(0), 4.0);
    fill_curve(2, *(float *)get_branch_data(0), 2.0);
    return true;
  }
};

int main()
{
  Tree2Hist eviewer;
  eviewer.add_filename("../example/wzdd-tree.root");
  eviewer.loop();
  return 0;
}
