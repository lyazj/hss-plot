#pragma once
#include "TreeInput.h"
#include <string>

namespace YAML { class Node; }

// Use TTree from multiple TFiles grouped by category as IEvent source.
class CategorizedTreeInput : public TreeInput {
public:
  CategorizedTreeInput(const char *name, const char *yamlpath);
  ~CategorizedTreeInput();
  const char *get_yamlpath() const { return yamlpath_; }

  // Step forward.
  virtual bool on_new_file(const char *) override;
  virtual void on_open_file() override;
  virtual void on_close_file() override;

  // Configuration.
  size_t get_ncategory() const;
  std::string get_category(size_t) const;
  bool get_category_configuration(size_t, YAML::Node *) const;
  bool get_category_configuration(const std::string &, YAML::Node *) const;
  size_t get_nsample(size_t) const;
  std::string get_sample(size_t, size_t) const;
  bool get_sample_configuration(size_t, size_t, YAML::Node *, YAML::Node *category_configuration = nullptr) const;
  bool get_sample_configuration(const std::string &, YAML::Node *, YAML::Node *category_configuration = nullptr) const;

  // Current position.
  std::string get_category() const;
  std::string get_sample() const;
  size_t get_icategory() const;
  size_t get_isample() const;
  bool get_category_configuration(YAML::Node *) const;
  bool get_sample_configuration(YAML::Node *) const;

  // Number of events per category/sample.
  // May NOT be up-to-date before closing a file.
  size_t get_category_nevent(const std::string &) const;
  size_t get_sample_nevent(const std::string &) const;
  size_t get_category_nevent(size_t) const;
  size_t get_sample_nevent(size_t, size_t) const;

protected:
  char *yamlpath_;
  class Detail; Detail *detail_;
};
