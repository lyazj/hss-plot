#pragma once
#include "interface.h"
#include <stddef.h>

// Event in TTree from TFile.
class TreeEvent : virtual public IEvent {
public:
  TreeEvent(const char *name);
  ~TreeEvent();
  const char *get_name() const { return name_; }

  // Current position.
  const char *get_filename() const;
  size_t get_ifilename() const;
  size_t get_local_index() const;
  size_t get_global_index() const;

  // File sources.
  size_t add_filename(const char *);
  size_t get_nfilename() const;
  const char *get_filename(size_t) const;

  // Select branches to read.
  size_t add_branch(const char *);
  size_t get_nbranch() const;
  const char *get_branch(size_t) const;

  // Get branch data and metadata.
  void *get_branch_data(size_t, size_t *nelem = nullptr) const;
  size_t get_branch_elem_size(size_t) const;
  size_t get_branch_nelem_max(size_t) const;

  // Step forward.
  virtual bool next() override;

protected:
  const char *name_;
  class Detail; Detail *detail_;
};
