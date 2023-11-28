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
#include <algorithm>
#include <math.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

class HistOutput::Detail {
public:
  vector<vector<pair<double, double>>> data;
  double data_min, data_max;
  double data_lb, data_ub;
  size_t nbin;
  vector<unique_ptr<TH1>> curves;
  vector<string> curve_titles;
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
  }

  void apply_cms_style() const {
    int iPeriod = 0;  // 1=7TeV, 2=8TeV, 3=7+8TeV, 7=7+8+13TeV, 0=free form (uses lumi_sqrtS)
    // iPos drives the position of the CMS logo in the plot
    // iPos=11 : top-left, left-aligned
    // iPos=33 : top-right, right-aligned
    // iPos=22 : center, centered
    // mode generally :
    //   iPos = 10*(alignement 1/2/3) + position (1/2/3 = left/center/right)
    int iPos = 1;
    CMS_lumi(canvas.get(), iPeriod, iPos);
  }
};

HistOutput::HistOutput(const char *xtitle, const char *ytitle, const char *filename)
  : xtitle_(strdup(xtitle)), ytitle_(strdup(ytitle)), filename_(strdup(filename)), legend_pos_{0.65, 0.95, 0.75, 0.9}
{
  detail_ = new Detail;
  detail_->canvas.reset(new TCanvas);
  detail_->data_min = +INFINITY;
  detail_->data_max = -INFINITY;
  detail_->data_lb = +INFINITY;
  detail_->data_ub = -INFINITY;
  detail_->nbin = 50;
  detail_->init_cms_style();
}

HistOutput::~HistOutput()
{
  save();
  delete detail_;
  free(filename_);
  free(ytitle_);
  free(xtitle_);
}

size_t HistOutput::add_curve(const char *title)
{
  size_t i = detail_->data.size();
  detail_->data.emplace_back();
  detail_->curve_titles.emplace_back(title);
  return i;
}

size_t HistOutput::get_ncurve() const
{
  return detail_->curve_titles.size();
}

TH1 *HistOutput::get_curve(size_t i) const
{
  if(i >= detail_->curves.size()) return nullptr;
  return detail_->curves[i].get();
}

const char *HistOutput::get_curve_title(size_t i) const
{
  if(i >= detail_->curve_titles.size()) return nullptr;
  return detail_->curve_titles[i].c_str();
}

bool HistOutput::fill_curve(size_t i, double value, double weight) const
{
  if(i >= get_ncurve()) return false;
  if(!isfinite(value) || !isfinite(weight)) return false;
  if(is_binned()) { detail_->curves[i]->Fill(value, weight); return true; }
  detail_->data[i].emplace_back(value, weight);
  detail_->data_min = min(detail_->data_min, value);
  detail_->data_max = max(detail_->data_max, value);
  return true;
}

void HistOutput::get_boundary(double &lb, double &ub) const
{
  double data_lb = detail_->data_lb;
  double data_ub = detail_->data_ub;
  if(isfinite(data_lb) && isfinite(data_ub)) { lb = data_lb, ub = data_ub; return; }

  double data_min = detail_->data_min;
  double data_max = detail_->data_max;
  if(isfinite(data_min) && isfinite(data_max)) { lb = data_min, ub = data_max; return; }

  lb = 0.0, ub = 1.0;
}

void HistOutput::set_boundary(double lb, double ub)
{
  detail_->data_lb = lb;
  detail_->data_ub = ub;
}

size_t HistOutput::get_nbin() const
{
  return detail_->nbin;
}

void HistOutput::set_nbin(size_t nbin)
{
  detail_->nbin = nbin;
}

bool HistOutput::is_binned() const
{
  return !detail_->curves.empty();
}

void HistOutput::bin()
{
  if(is_binned()) return;
  double lb, ub;
  get_boundary(lb, ub);
  set_boundary(lb, ub);
  detail_->curves.reserve(get_ncurve());
  for(size_t i = 0; i < get_ncurve(); ++i) {
    TH1F *curve = new TH1F("", get_curve_title(i), get_nbin(), lb, ub);
    if(xtitle_) curve->SetXTitle(xtitle_);
    if(ytitle_) curve->SetYTitle(ytitle_);
    for(const auto &vw : detail_->data[i]) {
      curve->Fill(vw.first, vw.second);
    }
    detail_->data[i] = { };
    detail_->curves.emplace_back(curve);
  }
}

bool HistOutput::save() const
{
  TCanvas *canvas = detail_->canvas.get();
  if(!filename_) return false;
  canvas->cd();
  for(size_t i = 0; i < get_ncurve(); ++i) {
    const_cast<HistOutput *>(this)->bin();
    TH1 *curve = detail_->curves[i].get();
    curve->SetLineColor(i + 1);
    string options = "HIST";
    if(i) options += ",SAME";
    curve->Draw(options.c_str());
  }

  if(get_ncurve() > 1) {
    TLegend *legend = canvas->BuildLegend(legend_pos_.xl, legend_pos_.yl, legend_pos_.xh, legend_pos_.yh);
    legend->Draw();
  }

  detail_->apply_cms_style();
  canvas->SetLogx(logx_);
  canvas->SetLogy(logy_);
  canvas->SaveAs(filename_);
  return true;
}