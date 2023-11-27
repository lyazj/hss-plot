#include "CMS_lumi.h"
#include "TreeInput.h"
#include "HistOutput.h"
#include "fs.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <TH1.h>

using namespace std;

class Tree2Hist : public TreeInput, public HistOutput {
public:
  Tree2Hist(double lb, double ub) : TreeInput("Events"), HistOutput("HssVSQCD", "number", get_output_filename(lb, ub).c_str()) {
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
    set_boundary(lb, ub);
    bin();
    set_logy(true);
    set_legend_pos(0.3, 0.3, 0.15, 0.15);
  }

  ~Tree2Hist() { optimize(); }

  virtual void process() override {
    // Extract Zqq flavour.
    string input_filename = TreeInput::get_filename();
    int pid = parse_pid(input_filename);
    if(pid < 1 || pid > 4) return;

    // Extract Hss and QCD scores.
    double Hss, QCD = 0.0;
    Hss = *(float *)get_branch_data(0);
    for(size_t i = 1; i <= 5; ++i) QCD += *(float *)get_branch_data(i);

    // Compute Hss significance relevant to QCD.
    if(Hss < 0 || QCD < 0) return;
    double HssVSQCD = QCD / Hss;
    if(std::isnan(HssVSQCD)) return;
    HssVSQCD = 1.0 / (1.0 + HssVSQCD);

    // Submit result.
    this->fill_curve(pid - 1, HssVSQCD);
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

  static string get_output_filename(double lb, double ub) {
    return "HssVSQCD_" + to_string(lb) + "_" + to_string(ub) + ".pdf";
  }

  void optimize() const {
    size_t nbin = get_nbin();
    if(nbin == 0) return;

    double s_total, b_total = 0.0;
    double sf = 1e6 / 7395487;  // [XXX] Scale to 10^6 events (100/fb).
    vector<Double_t *> integrals(4);
    vector<Double_t> totals(4);
    for(size_t i = 0; i < 4; ++i) {
      integrals[i] = get_curve(i)->GetIntegral();
      totals[i] = get_curve(i)->GetEffectiveEntries() * sf;
      if(i == 2) s_total = integrals[i][nbin] * totals[i];
      else b_total += integrals[i][nbin] * totals[i];
    }

    vector<tuple<double, double, double, double>> significance;
    significance.reserve(nbin - 1);
    for(size_t left_last_bin = 0; left_last_bin < nbin; ++left_last_bin) {
      double s_left, b_left = 0.0;
      for(size_t i = 0; i < 4; ++i) {
        if(i == 2) s_left = integrals[i][left_last_bin] * totals[i];
        else b_left += integrals[i][left_last_bin] * totals[i];
      }
      double s_right = s_total - s_left;
      double b_right = b_total - b_left;
      double sig = get_signal_significance(s_right, b_right);
      if(!isfinite(sig)) sig = 0.0;
      significance.emplace_back(sig, get_curve(0)->GetBinLowEdge(left_last_bin + 1), s_right, b_right);
    }
    sort(significance.begin(), significance.end());
    reverse(significance.begin(), significance.end());
    cout << "sig.\tpos.\tsg.\tbg." << endl << scientific << setprecision(1);
    for(const auto &rec : significance) {
      cout << get<0>(rec) << '\t' << get<1>(rec) << '\t' << get<2>(rec) << '\t' << get<3>(rec) << endl;
    }
  }

  static double get_signal_significance(double s, double b) {
    return sqrt(2 * ((s + b) * log(1 + s / b) - s));
  }
};

int main(int argc, char *argv[])
{
  if(argc < 4) {
    cerr << "usage: " << program_invocation_short_name
         << " <lower-bound> <upper-bound> <dir-to-root-files> [ <more-dir> ... ]" << endl;
    return 1;
  }
  lumi_sqrtS = "2018 1L simulation";  // [XXX]

  Tree2Hist eviewer(stod(argv[1]), stod(argv[2]));
  for(int i = 3; i < argc; ++i) {
    ListDir lsrst(argv[i], ListDir::DT_ALL & ~ListDir::DT_DIR);
    lsrst.sort_by_numbers();
    for(const string &name : lsrst.get_full_names()) eviewer.add_filename(name.c_str());
  }
  eviewer.loop();
  return 0;
}
