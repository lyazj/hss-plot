#include "CMS_lumi.h"
#include "CategorizedTreeInput.h"
#include "HistOutput.h"
#include "MultiStep.h"
#include <yaml-cpp/yaml.h>
#include "fs.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <TH1.h>

using namespace std;

class TaggerHist : public CategorizedTreeInput, public HistOutput, public MultiStep {
public:
  TaggerHist(const char *yamlpath, double lb, double ub,
      const string &branch_prefix, const string &signal_branch_suffix,
      const string &signal_category, double luminosity, double threshold)
    : CategorizedTreeInput("Events", yamlpath)
    , HistOutput(get_output_ytitle(signal_branch_suffix).c_str(), "number",
        get_output_filename(signal_branch_suffix, lb, ub).c_str())
    , signal_category_(signal_category), luminosity_(luminosity), threshold_(threshold)
  {
    add_branch((branch_prefix + signal_branch_suffix).c_str());  // 0
    add_branch((branch_prefix + "QCDbb").c_str());               // 1
    add_branch((branch_prefix + "QCDb").c_str());                // 2
    add_branch((branch_prefix + "QCDcc").c_str());               // 3
    add_branch((branch_prefix + "QCDc").c_str());                // 4
    add_branch((branch_prefix + "QCDothers").c_str());           // 5
    size_t ncategory = get_ncategory();
    for(size_t i = 0; i < ncategory; ++i) {
      add_curve(get_category(i).c_str(), category_issignal(i));
    }
    set_boundary(lb, ub);
    bin();
    set_logy(true);
    set_legend_pos(0.65, 0.95, 0.75, 0.9);
    set_gridy(true);
  }

  ~TaggerHist() { /* optimize(); */ post_process(); }

  virtual bool process() override {
    // Compute weight of current event.
    double weight = get_sample_weight();

    // Extract Hss and QCD scores.
    double Hss, QCD = 0.0;
    Hss = *(float *)get_branch_data(0);
    for(size_t i = 1; i <= 5; ++i) QCD += *(float *)get_branch_data(i);

    // Compute Hss significance relevant to QCD.
    if(Hss < 0 || QCD < 0) return false;
    double HssVSQCD = QCD / Hss;
    if(std::isnan(HssVSQCD)) return false;
    HssVSQCD = 1.0 / (1.0 + HssVSQCD);

    // Submit result.
    this->fill_curve(get_icategory(), HssVSQCD, weight);
    return HssVSQCD >= threshold_;
  }

  double get_sample_weight() const {
    YAML::Node sample;
    get_sample_configuration(&sample);
    return sample["xs"].as<double>() * 1e3 * luminosity_ / sample["nevent"].as<double>();
  }

  bool category_issignal(const string &category) const { return category == signal_category_; }
  bool category_issignal(size_t i) const { return category_issignal(get_category(i)); }
  bool category_issignal() const { return category_issignal(get_category()); }

private:
  string signal_category_;
  double luminosity_;
  double threshold_;

  static string get_output_ytitle(const string &signal_branch_suffix) {
    return signal_branch_suffix + "VSQCD";
  }

  static string get_output_filename(const string &signal_branch_suffix, double lb, double ub) {
    return get_output_ytitle(signal_branch_suffix) + "_" + to_string(lb) + "_" + to_string(ub) + ".pdf";
  }

  void optimize() const {
    size_t nbin = get_nbin();
    if(nbin == 0) return;

    size_t ncurve = get_ncurve();
    double s_total, b_total = 0.0;
    vector<char> is_signal(ncurve);
    vector<Double_t *> integrals(ncurve);
    vector<Double_t> totals(ncurve);
    for(size_t i = 0; i < ncurve; ++i) {
      is_signal[i] = category_issignal(i);
      integrals[i] = get_curve(i)->GetIntegral();
      totals[i] = get_curve(i)->GetSumOfWeights();
      if(is_signal[i]) s_total = integrals[i][nbin] * totals[i];
      else b_total += integrals[i][nbin] * totals[i];
    }

    vector<tuple<double, double, double, double>> significance;
    significance.reserve(nbin - 1);
    for(size_t left_last_bin = 0; left_last_bin < nbin; ++left_last_bin) {
      double s_left, b_left = 0.0;
      for(size_t i = 0; i < ncurve; ++i) {
        if(is_signal[i]) s_left = integrals[i][left_last_bin] * totals[i];
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

  void post_process() {
    size_t ncurve = get_ncurve();
    double ymax = 0.0;
    for(size_t i = 0; i < ncurve; ++i) {
      // Compute total xs and nb for each category.
      double xs = 0.0;
      size_t nb_orig = 0;
      size_t nb_read = 0;
      size_t nsample = get_nsample(i);
      for(size_t j = 0; j < nsample; ++j) {
        YAML::Node sample; get_sample_configuration(i, j, &sample);
        xs += sample["xs"].as<double>();
        nb_orig += sample["nevent"].as<size_t>();
        nb_read += get_sample_nevent(i, j);
      }
      TH1 *curve = get_curve(i);

      // Add xs and nb info.
      ostringstream oss;
      oss << " (" << nb_read << "/" << nb_orig << " events scaled to "
          << scientific << setprecision(3) << xs << " pb)";
      string title = curve->GetTitle();
      title += oss.str();
      curve->SetTitle(title.c_str());

      // Compute maximum bin height.
      size_t nbin = get_nbin();
      for(size_t j = 1; j <= nbin; ++j) {
        ymax = max(ymax, curve->GetBinContent(j));
      }
    }
    // Adjust y-range.
    set_rangey(0.8 * (signal_category_[1] == 'h' ? 0.001 : 1.0), ymax * 1.2);
  }
};

class KinBDTHist : public HistOutput, public MultiStep {
public:
  KinBDTHist(TaggerHist *tagger, double lb, double ub, double threshold)
    : HistOutput("kinBDT", "number", get_output_filename(lb, ub).c_str())
    , tagger_(tagger), threshold_(threshold)
  {
    ikinBDT_ = tagger->add_branch("kinBDT");
    size_t ncategory = tagger->get_ncategory();
    for(size_t i = 0; i < ncategory; ++i) {
      add_curve(tagger->get_category(i).c_str(), tagger->category_issignal(i));
    }
    set_boundary(lb, ub);
    bin();
    set_logy(false);
    set_legend_pos(0.65, 0.95, 0.75, 0.9);
    set_gridy(true);
  }

private:
  TaggerHist *tagger_;
  double threshold_;
  size_t ikinBDT_;

  static string get_output_filename(double lb, double ub) {
    return "kinBDT_" + to_string(lb) + "_" + to_string(ub) + ".pdf";
  }

  virtual bool process() override {
    // Compute weight of current event.
    double weight = tagger_->get_sample_weight();

    // Extract kinBDT score.
    double kinBDT = *(float *)tagger_->get_branch_data(ikinBDT_);

    // Submit result.
    this->fill_curve(tagger_->get_icategory(), kinBDT, weight);
    return kinBDT >= threshold_;
  }
};

class MassHist : public HistOutput, public MultiStep {
public:
  MassHist(TaggerHist *tagger, double lb, double ub, double threshold)
    : HistOutput("Mass", "number", get_output_filename(lb, ub).c_str())
    , tagger_(tagger), threshold_(threshold)
  {
    iMass_ = tagger->add_branch("ak15_regressed_mass");
    size_t ncategory = tagger->get_ncategory();
    for(size_t i = 0; i < ncategory; ++i) {
      add_curve(tagger->get_category(i).c_str(), tagger->category_issignal(i));
    }
    set_boundary(lb, ub);
    bin();
    set_logy(false);
    set_legend_pos(0.65, 0.95, 0.75, 0.9);
    set_gridy(true);
  }

private:
  TaggerHist *tagger_;
  double threshold_;
  size_t iMass_;

  static string get_output_filename(double lb, double ub) {
    return "Mass_" + to_string(lb) + "_" + to_string(ub) + ".pdf";
  }

  virtual bool process() override {
    // Compute weight of current event.
    double weight = tagger_->get_sample_weight();

    // Extract Mass score.
    double Mass = *(float *)tagger_->get_branch_data(iMass_);

    // Submit result.
    this->fill_curve(tagger_->get_icategory(), Mass, weight);
    return Mass >= threshold_;
  }
};

int main(int argc, char *argv[])
{
  if(argc < 11) {
    cerr << "usage: " << program_invocation_short_name << " <categorization-yaml>"
         << " <lower-bound> <upper-bound> <branch-prefix> <signal-branch-suffix>"
         << " <signal-category> <luminosity> <tagger-threshold> <kinbdt-threshold>"
         << " <dir-to-root-files> [ <more-dir> ... ]"
         << endl;
    return 1;
  }
  lumi_sqrtS = (dotsplit(basename(argv[1])).first + " " + argv[7] + "/fb").c_str();

  TaggerHist tagger_hist(argv[1], stod(argv[2]), stod(argv[3]), argv[4], argv[5], argv[6], stod(argv[7]), stod(argv[8]));
  (&tagger_hist)
    ->then(new KinBDTHist(&tagger_hist, stod(argv[2]), stod(argv[3]), stod(argv[9])))
    ->then(new MassHist(&tagger_hist, stod(argv[2]), stod(argv[3]), 0.0))
    ;
  for(int i = 10; i < argc; ++i) {
    ListDir lsrst(argv[i], ListDir::DT_ALL & ~ListDir::DT_DIR);
    lsrst.sort_by_numbers();
    for(const string &name : lsrst.get_full_names()) tagger_hist.add_filename(name.c_str());
  }
  tagger_hist.loop();
  return 0;
}
