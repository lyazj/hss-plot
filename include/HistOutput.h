#pragma once
#include "interface.h"
#include <stddef.h>

// Use saved TCanvas to filename as IEvent destination.
class HistOutput : virtual public IEvent {
public:
  HistOutput(const char *title, const char *filename);
  ~HistOutput();
  const char *get_title() const { return title_; }
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

  // Draw and save histograms.
  bool save() const;

protected:
  const char *title_;
  const char *filename_;
  class Detail; Detail *detail_;
};
