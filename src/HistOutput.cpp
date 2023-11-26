#include "../include/HistOutput.h"
#include <TH1F.h>
#include <TCanvas.h>
#include <vector>
#include <memory>
#include <string>
#include <utility>

using namespace std;

class HistOutput::Detail {
public:
  vector<vector<pair<double, double>>> data;
  vector<unique_ptr<TH1>> curves;
  vector<const char *> curve_names;
  vector<unique_ptr<pair<double, double>>> curve_boundaries;
  vector<size_t> curve_nbins;
};

HistOutput::HistOutput(const char *title, const char *filename)
  : title_(title), filename_(filename)
{
  detail_ = new Detail;
}

HistOutput::~HistOutput()
{
  delete detail_;
}

size_t HistOutput::add_curve(const char *name)
{
  size_t i = detail_->data.size();
  detail_->data.emplace_back();
  detail_->curve_names.emplace_back(name);
  detail_->curve_boundaries.emplace_back();
  detail_->curve_nbins.push_back(50);
  return i;
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
  TH1F *curve = new TH1F("", detail_->curve_names[i], detail_->curve_nbins[i], lb, ub);
  if(title_) curve->SetXTitle(title_);
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
  unique_ptr<TCanvas> canvas(new TCanvas);
  canvas->cd();
  for(size_t i = 0; i < get_ncurve(); ++i) {
    const_cast<HistOutput *>(this)->bin(i);
    detail_->curves[i]->Draw("HIST,SAME");
  }
  canvas->SaveAs(filename_);
  return true;
}
