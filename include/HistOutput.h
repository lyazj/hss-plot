#pragma once
#include "interface.h"
#include <stddef.h>

// Use saved TCanvas to filename as IEvent destination.
class HistOutput : virtual public IEvent {
public:
  HistOutput(const char *xtitle, const char *ytitle, const char *filename);
  ~HistOutput();
  const char *get_xtitle() const { return xtitle_; }
  const char *get_ytitle() const { return ytitle_; }
  const char *get_filename() const { return filename_; }

  // Curves in the same plot.
  size_t add_curve(const char *title);
  size_t get_ncurve() const;
  const char *get_curve_title(size_t) const;
  bool fill_curve(size_t, double value, double weight = 1.0) const;
  virtual bool process() const override = 0;

  // Boundary and binning control.
  bool get_boundary(size_t, double &, double &) const;
  bool set_boundary(size_t, double, double);
  bool get_nbin(size_t, size_t &) const;
  bool set_nbin(size_t, size_t);
  bool is_binned(size_t, bool &) const;
  bool bin(size_t);

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

  // Draw and save histograms.
  bool save() const;

protected:
  const char *xtitle_;
  const char *ytitle_;
  const char *filename_;
  struct { double xl, xh, yl, yh; } legend_pos_;
  bool logx_, logy_;
  class Detail; Detail *detail_;
};
