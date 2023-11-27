#pragma once
#include "EventViewer.h"
#include <stddef.h>

// Use TTree from multiple TFiles as IEvent source.
class TreeInput : virtual public EventViewer {
public:
  TreeInput(const char *name);
  ~TreeInput();
  const char *get_name() const { return name_; }

  // Step forward.
  virtual bool next() override;

  // Current position.
  const char *get_filename() const;
  size_t get_ifilename() const;
  size_t get_local_index() const;
  size_t get_global_index() const;

  // File sources.
  // add_filename() should be called before any call to next().
  // The behavior is undefined if file sources change while sliding.
  size_t add_filename(const char *);
  size_t get_nfilename() const;
  const char *get_filename(size_t) const;

  // Select branches to read.
  // add_branch() should be called before any call to next().
  // The behavior is undefined if requested branches change while sliding.
  size_t add_branch(const char *);
  size_t get_nbranch() const;
  const char *get_branch(size_t) const;

  // Get branch data and metadata.
  // get_branch_elem_size() returns size of pointers for class objects.
  // get_branch_elem_size() and get_branch_nelem_max() return 0 on error.
  void *get_branch_data(size_t, size_t *nelem = nullptr) const;
  size_t get_branch_elem_size(size_t) const;
  size_t get_branch_nelem_max(size_t) const;

protected:
  char *name_;
  class Detail; Detail *detail_;
};
