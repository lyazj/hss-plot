#include "TreeInput.h"
#include "HistOutput.h"

class Tree2Hist : public TreeInput, public HistOutput {
public:
  Tree2Hist() : TreeInput("Events"), HistOutput("ak15_phi", "number", "../example/ak15_phi.pdf") {
    add_branch("ak15_phi");
    add_curve("tree");
    set_boundary(-3.5, 3.5);
    bin();
  }

  virtual void process() override {
    fill_curve(0, *(float *)get_branch_data(0), 1.0);
  }
};

int main()
{
  Tree2Hist eviewer;
  eviewer.add_filename("../example/wzdd-tree.root");
  eviewer.loop();
  return 0;
}
