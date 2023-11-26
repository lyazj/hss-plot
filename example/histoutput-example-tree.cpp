#include "../include/TreeInput.h"
#include "../include/HistOutput.h"
#include "../include/flow.h"
#include <memory>
#include <iostream>

using namespace std;

class Tree2Hist : public TreeInput, public HistOutput {
public:
  Tree2Hist() : TreeInput("Events"), HistOutput("ak15_phi", "ak15_phi.pdf") {
    add_branch("ak15_phi");
    add_curve("tree");
    set_boundary(0, -3.5, 3.5);
    bin(0);
  }

  virtual bool process() const override {
    fill_curve(0, *(float *)get_branch_data(0), 1.0);
    return true;
  }
};

int main()
{
  unique_ptr<Tree2Hist> event(new Tree2Hist);
  event->add_filename("../example/wzdd-tree.root");
  EventAnalyzer analyzer(event.get());
  analyzer.loop();
  return 0;
}
