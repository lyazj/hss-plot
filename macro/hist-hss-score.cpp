#include "../include/TreeInput.h"
#include "../include/HistOutput.h"
#include "../include/flow.h"
#include "../include/fs.h"
#include <memory>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

using namespace std;

class Tree2Hist : public TreeInput, public HistOutput {
public:
  Tree2Hist() : TreeInput("Events"), HistOutput("HssVSQCD", "HssVSQCD.pdf") {
    add_branch("ak15_ParTMDV2_Hss");        // 0
    add_branch("ak15_ParTMDV2_QCDbb");      // 1
    add_branch("ak15_ParTMDV2_QCDb");       // 2
    add_branch("ak15_ParTMDV2_QCDcc");      // 3
    add_branch("ak15_ParTMDV2_QCDc");       // 4
    add_branch("ak15_ParTMDV2_QCDothers");  // 5
    add_curve("Wlv_Zdd");  // 0
    add_curve("Wlv_Zuu");  // 1
    add_curve("Wlv_Zss");  // 2
    add_curve("Wlv_Zcc");  // 3
    for(size_t i = 0; i < 4; ++i) set_boundary(i, 0.0, 1.0), bin(i);
  }

  virtual bool process() const override {
    // Extract Zqq flavour.
    string input_filename = TreeInput::get_filename();
    int pid = parse_pid(input_filename);
    if(pid < 1 || pid > 4) return false;

    // Extract Hss and QCD scores.
    double Hss, QCD = 0.0;
    Hss = *(float *)get_branch_data(0);
    for(size_t i = 1; i <= 5; ++i) QCD += *(float *)get_branch_data(i);

    // Compute Hss significance relevant to QCD.
    if(Hss < 0 || QCD < 0) return false;
    double HssVSQCD = QCD / Hss;
    if(isnan(HssVSQCD)) return false;
    HssVSQCD = 1.0 / (1.0 + HssVSQCD);

    // Submit result.
    this->fill_curve(pid - 1, HssVSQCD);
    return true;
  }

private:
  static int parse_pid(string path) {
    // *-<PID>_<ID>_tree.root
    if(path.length() < 12) return 0;
    path.resize(path.length() - 10);

    // *-<PID>_<ID>
    size_t pos = path.rfind('_');
    if(pos == path.npos) return 0;
    path.resize(pos);

    // *-<PID>
    pos = path.rfind('-');
    if(pos == path.npos) return 0;
    path = path.substr(pos + 1);
    return atoi(path.c_str());
  }
};

int main(int argc, const char *argv[])
{
  if(argc == 1) {
    cerr << "usage: " << program_invocation_short_name
         << " <dir-to-root-files> [ <more-dir> ... ]" << endl;
    return 1;
  }

  unique_ptr<Tree2Hist> event(new Tree2Hist);
  for(int i = 1; i < argc; ++i) {
    ListDir lsrst(argv[i]);
    lsrst.sort_by_numbers();
    for(const string &name : lsrst.get_full_names()) event->add_filename(name.c_str());
  }

  EventAnalyzer analyzer(event.get());
  analyzer.loop();
  return 0;
}
