#pragma once
#include "EventViewer.h"
#include <stddef.h>

class TH1;

// Use saved TCanvas to filename as IEvent destination.
class HistOutput : virtual public EventViewer {
public:
  HistOutput(const char *xtitle, const char *ytitle, const char *filename);
  ~HistOutput();
  const char *get_xtitle() const { return xtitle_; }
  const char *get_ytitle() const { return ytitle_; }
  const char *get_filename() const { return filename_; }

  // Curves in the same plot.
  size_t add_curve(const char *title);
  size_t get_ncurve() const;
  TH1 *get_curve(size_t) const;
  const char *get_curve_title(size_t) const;
  bool fill_curve(size_t, double value, double weight = 1.0) const;
  virtual void process() override = 0;

  // Boundary and binning control.
  void get_boundary(double &, double &) const;
  void set_boundary(double, double);
  size_t get_nbin() const;
  void set_nbin(size_t);
  bool is_binned() const;
  void bin();

  // Legend control.
  void get_legend_pos(double &xl, double &xh, double &yl, double &yh)
    { xl = legend_pos_.xl, xh = legend_pos_.xh, yl = legend_pos_.yl, yh = legend_pos_.yh; }
  void set_legend_pos(double xl, double xh, double yl, double yh)
    { legend_pos_ = { xl, xh, yl, yh }; }

  // Axis control.
  void set_logx(bool enable) { logx_ = enable; }
  bool get_logx() { return logx_; }
  void set_logy(bool enable) { logy_ = enable; }
  bool get_logy() { return logy_; }
  void set_rangex(double min, double max) { rangex_ = true; xmin_ = min, xmax_ = max; }
  void set_rangey(double min, double max) { rangey_ = true; ymin_ = min, ymax_ = max; }

  // Draw and save histograms.
  bool save() const;

protected:
  char *xtitle_;
  char *ytitle_;
  char *filename_;
  struct { double xl, xh, yl, yh; } legend_pos_;
  bool logx_, logy_, rangex_, rangey_;
  double xmin_, xmax_, ymin_, ymax_;
  class Detail; Detail *detail_;
};
