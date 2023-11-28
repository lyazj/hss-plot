#include "CMS_lumi.h"
#include "CategorizedTreeInput.h"
#include "HistOutput.h"
#include <yaml-cpp/yaml.h>
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

class Tree2Hist : public CategorizedTreeInput, public HistOutput {
public:
  Tree2Hist(const char *yamlpath, double lb, double ub)
    : CategorizedTreeInput("Events", yamlpath), HistOutput("HssVSQCD", "number", get_output_filename(lb, ub).c_str()) {
    add_branch("ak15_ParTMDV2_Hss");        // 0
    add_branch("ak15_ParTMDV2_QCDbb");      // 1
    add_branch("ak15_ParTMDV2_QCDb");       // 2
    add_branch("ak15_ParTMDV2_QCDcc");      // 3
    add_branch("ak15_ParTMDV2_QCDc");       // 4
    add_branch("ak15_ParTMDV2_QCDothers");  // 5
    size_t ncategory = get_ncategory();
    for(size_t i = 0; i < ncategory; ++i) {
      add_curve(get_category(i).c_str());
    }
    set_boundary(lb, ub);
    bin();
    set_logy(true);
    set_legend_pos(0.3, 0.3, 0.15, 0.15);
  }

  ~Tree2Hist() { optimize(); }

  virtual void process() override {
    // Compute weight of current event.
    YAML::Node sample;
    get_sample_configuration(&sample);
    double weight = sample["xs"].as<double>() * 100.0e3 / sample["nevent"].as<double>();

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
    this->fill_curve(get_icategory(), HssVSQCD, weight);
  }

private:
  static string get_output_filename(double lb, double ub) {
    return "HssVSQCD_" + to_string(lb) + "_" + to_string(ub) + ".pdf";
  }

  void optimize() const {
    size_t nbin = get_nbin();
    if(nbin == 0) return;

    size_t ncurve = get_ncurve();
    double s_total, b_total = 0.0;
    vector<YAML::Node> categories(ncurve);
    vector<Double_t *> integrals(ncurve);
    vector<Double_t> totals(ncurve);
    for(size_t i = 0; i < ncurve; ++i) {
      get_category_configuration(i, &categories[i]);
      integrals[i] = get_curve(i)->GetIntegral();
      totals[i] = get_curve(i)->GetSumOfWeights();
      if(categories[i]["is_signal"].as<bool>()) s_total = integrals[i][nbin] * totals[i];
      else b_total += integrals[i][nbin] * totals[i];
    }

    vector<tuple<double, double, double, double>> significance;
    significance.reserve(nbin - 1);
    for(size_t left_last_bin = 0; left_last_bin < nbin; ++left_last_bin) {
      double s_left, b_left = 0.0;
      for(size_t i = 0; i < ncurve; ++i) {
        if(categories[i]["is_signal"].as<bool>()) s_left = integrals[i][left_last_bin] * totals[i];
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
  if(argc < 5) {
    cerr << "usage: " << program_invocation_short_name << " <categorization-yaml>"
         << " <lower-bound> <upper-bound> <dir-to-root-files> [ <more-dir> ... ]" << endl;
    return 1;
  }
  lumi_sqrtS = dotsplit(basename(argv[1])).first.c_str();

  Tree2Hist eviewer(argv[1], stod(argv[2]), stod(argv[3]));
  for(int i = 4; i < argc; ++i) {
    ListDir lsrst(argv[i], ListDir::DT_ALL & ~ListDir::DT_DIR);
    lsrst.sort_by_numbers();
    for(const string &name : lsrst.get_full_names()) eviewer.add_filename(name.c_str());
  }
  eviewer.loop();
  return 0;
}
