#include "../include/HistOutput.h"
#include "../include/tdrstyle.h"
#include "../include/CMS_lumi.h"
#include <TH1F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <vector>
#include <memory>
#include <string>
#include <utility>

using namespace std;

class HistOutput::Detail {
public:
  vector<vector<pair<double, double>>> data;
  vector<unique_ptr<TH1>> curves;
  vector<const char *> curve_titles;
  vector<unique_ptr<pair<double, double>>> curve_boundaries;
  vector<size_t> curve_nbins;
  unique_ptr<TCanvas> canvas;

  void init_cms_style() const {
    setTDRStyle();
    canvas->SetWindowSize(1200, 900);
    canvas->SetFillColor(0);
    canvas->SetBorderMode(0);
    canvas->SetFrameFillStyle(0);
    canvas->SetFrameBorderMode(0);
    canvas->SetLeftMargin(0.15);
    canvas->SetRightMargin(0.04);
    canvas->SetTopMargin(0.08);
    canvas->SetBottomMargin(0.12);
    canvas->SetTickx(0);
    canvas->SetTicky(0);

    writeExtraText = true;  // if extra text
    extraText = "Preliminary";  // default extra text is "Preliminary"
    //lumi_8TeV = "19.1 fb^{-1}";  // default is "19.7 fb^{-1}"
    //lumi_7TeV = "4.9 fb^{-1}";  // default is "5.1 fb^{-1}"
    lumi_sqrtS = "13 TeV";  // used with iPeriod = 0
                            // e.g. for simulation-only plots (default is an empty string)
  }

  void apply_cms_style() const {
    int iPeriod = 0;  // 1=7TeV, 2=8TeV, 3=7+8TeV, 7=7+8+13TeV, 0=free form (uses lumi_sqrtS)
    // iPos drives the position of the CMS logo in the plot
    // iPos=11 : top-left, left-aligned
    // iPos=33 : top-right, right-aligned
    // iPos=22 : center, centered
    // mode generally :
    //   iPos = 10*(alignement 1/2/3) + position (1/2/3 = left/center/right)
    int iPos = 11;
    CMS_lumi(canvas.get(), iPeriod, iPos);
  }
};

HistOutput::HistOutput(const char *title, const char *filename)
  : title_(title), filename_(filename)
{
  detail_ = new Detail;
  detail_->canvas.reset(new TCanvas);
  detail_->init_cms_style();
}

HistOutput::~HistOutput()
{
  save();
  delete detail_;
}

size_t HistOutput::add_curve(const char *title)
{
  size_t i = detail_->data.size();
  detail_->data.emplace_back();
  detail_->curves.emplace_back();
  detail_->curve_titles.emplace_back(title);
  detail_->curve_boundaries.emplace_back();
  detail_->curve_nbins.push_back(50);
  return i;
}

size_t HistOutput::get_ncurve() const
{
  return detail_->curve_titles.size();
}

const char *HistOutput::get_curve_title(size_t i) const
{
  if(i >= detail_->curve_titles.size()) return nullptr;
  return detail_->curve_titles[i];
}

bool HistOutput::fill_curve(size_t i, double value, double weight) const
{
  if(i >= detail_->data.size()) return false;
  if(detail_->curves[i]) { detail_->curves[i]->Fill(value, weight); return true; }
  detail_->data[i].emplace_back(value, weight);
  return true;
}

bool HistOutput::get_boundary(size_t i, double &lb, double &ub) const
{
  if(i >= detail_->curve_boundaries.size()) return false;
  auto boundary = detail_->curve_boundaries[i].get();
  if(boundary) lb = boundary->first, ub = boundary->second;
  else lb = 0.0, ub = 1.0;  // [XXX]
  return true;
}

bool HistOutput::set_boundary(size_t i, double lb, double ub)
{
  if(i >= detail_->curve_boundaries.size()) return false;
  detail_->curve_boundaries[i].reset(new pair<double, double>(lb, ub));
  return true;
}

bool HistOutput::get_nbin(size_t i, size_t &nbin) const
{
  if(i >= detail_->curve_nbins.size()) return false;
  nbin = detail_->curve_nbins[i];
  return true;
}

bool HistOutput::set_nbin(size_t i, size_t nbin)
{
  if(i >= detail_->curve_nbins.size()) return false;
  detail_->curve_nbins[i] = nbin;
  return true;
}

bool HistOutput::is_binned(size_t i, bool &binned) const
{
  if(i >= detail_->curves.size()) return false;
  binned = (bool)detail_->curves[i];
  return true;
}

bool HistOutput::bin(size_t i)
{
  if(i >= detail_->curves.size()) return false;
  if(detail_->curves[i]) return true;
  double lb, ub;
  get_boundary(i, lb, ub);
  set_boundary(i, lb, ub);
  TH1F *curve = new TH1F("", detail_->curve_titles[i], detail_->curve_nbins[i], lb, ub);
  if(title_) curve->SetXTitle(title_);
  curve->SetYTitle("number");  // [XXX]
  for(const auto &vw : detail_->data[i]) {
    curve->Fill(vw.first, vw.second);
  }
  detail_->data[i] = { };
  detail_->curves[i].reset(curve);
  return true;
}

bool HistOutput::save() const
{
  if(!filename_) return false;
  detail_->canvas->cd();
  for(size_t i = 0; i < get_ncurve(); ++i) {
    const_cast<HistOutput *>(this)->bin(i);
    TH1 *curve = detail_->curves[i].get();
    curve->SetLineColor(i + 1);
    string options = "HIST";
    if(i) options += ",SAME";
    curve->Draw(options.c_str());
  }

  TLegend *legend = detail_->canvas->BuildLegend(0.65, 0.75, 0.95, 0.9);  // [XXX]
  legend->Draw();

  detail_->apply_cms_style();
  detail_->canvas->SetLogy();  // [XXX]
  detail_->canvas->SaveAs(filename_);
  return true;
}
