#include "CategorizedTreeInput.h"
#include <yaml-cpp/yaml.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <algorithm>

using namespace std;

class CategorizedTreeInput::Detail {
public:
  YAML::Node yaml;
  unordered_map<string, YAML::Node> category_index;
  unordered_map<string, pair<YAML::Node, YAML::Node /* category */>> sample_index;
  size_t sample_name_minlen;
  size_t sample_name_maxlen;
  YAML::Node *current_category;
  YAML::Node *current_sample;

  void load_samples(const char *yamlpath) {
    yaml = YAML::LoadFile(yamlpath);

    // Index categories.
    size_t icategory = 0;
    for(YAML::Node category : yaml) {
      const string &category_name = category["name"].as<string>();
      if(!category_index.insert({ category_name, category }).second) {
        throw logic_error("duplicate category name: " + category_name);
      }

      // Index samples.
      size_t isample = 0;
      for(YAML::Node sample : category["samples"]) {
        const string &sample_name = sample["name"].as<string>();
        if(!sample_index.insert({ sample_name, { sample, category } }).second) {
          throw logic_error("duplicate sample name: " + sample_name);
        }

        sample_name_minlen = min(sample_name_minlen, sample_name.length());
        sample_name_maxlen = max(sample_name_maxlen, sample_name.length());
        sample["_runtime_id"] = isample;
        sample["_runtime_nevent"] = 0;
      }
      category["_runtime_id"] = icategory;
      category["_runtime_nevent"] = 0;
    }
  }

  void list_samples() const {
    for(const YAML::Node &category : yaml) {
      cout << "- " << category["name"] << endl;
      for(const YAML::Node &sample : category["samples"]) {
        cout << "  + " << sample["name"] << endl;
      }
    }
  }

  bool match_filename(const char *filename_in) {
    string filename = filename_in;
    size_t minlen = min(sample_name_minlen, filename.length());
    size_t maxlen = max(sample_name_maxlen, filename.length());
    for(;;) {
      auto sample_iter = sample_index.find(filename.substr(0, maxlen));
      if(sample_iter != sample_index.end()) {
        current_sample = &sample_iter->second.first;
        current_category = &sample_iter->second.second;
        return true;
      }
      if(maxlen == minlen) break;
      --maxlen;
    }
    return false;
  }
};

CategorizedTreeInput::CategorizedTreeInput(const char *name, const char *yamlpath)
  : TreeInput(name), yamlpath_(strdup(yamlpath))
{
  detail_ = new Detail;
  detail_->sample_name_minlen = -1;
  detail_->sample_name_maxlen = 0;
  detail_->current_category = nullptr;
  detail_->current_sample = nullptr;
  detail_->load_samples(yamlpath_);
  detail_->list_samples();
}

CategorizedTreeInput::~CategorizedTreeInput()
{
  delete detail_;
  free(yamlpath_);
}

bool CategorizedTreeInput::on_open_file()
{
  return detail_->match_filename(get_filename());
}

void CategorizedTreeInput::on_close_file()
{
  size_t nevent = get_local_index();
  (*detail_->current_category)["_runtime_nevent"] = (*detail_->current_category)["_runtime_nevent"].as<size_t>() + nevent;
  (*detail_->current_sample)["_runtime_nevent"] = (*detail_->current_sample)["_runtime_nevent"].as<size_t>() + nevent;
}

size_t CategorizedTreeInput::get_ncategory() const
{
  return detail_->yaml.size();
}

string CategorizedTreeInput::get_category(size_t i) const
{
  if(i >= detail_->yaml.size()) return nullptr;
  return detail_->yaml[i]["name"].as<string>();
}

bool CategorizedTreeInput::get_category_configuration(size_t i,
    YAML::Node *configuration) const
{
  if(i >= detail_->yaml.size()) return false;
  if(configuration) *configuration = detail_->yaml[i];
  return true;
}

bool CategorizedTreeInput::get_category_configuration(const string &category,
    YAML::Node *configuration) const
{
  auto iter = detail_->category_index.find(category);
  if(iter == detail_->category_index.end()) return false;
  if(configuration) *configuration = iter->second;
  return true;
}

size_t CategorizedTreeInput::get_nsample(size_t i) const
{
  if(i >= detail_->yaml.size()) return 0;
  return detail_->yaml[i]["samples"].size();
}

string CategorizedTreeInput::get_sample(size_t icategory, size_t isample) const
{
  if(icategory >= detail_->yaml.size()) return nullptr;
  if(isample >= detail_->yaml[icategory]["samples"].size()) return nullptr;
  return detail_->yaml[icategory]["samples"][isample]["name"].as<string>();
}

bool CategorizedTreeInput::get_sample_configuration(size_t icategory, size_t isample,
    YAML::Node *sample_configuration, YAML::Node *category_configuration) const
{
  if(icategory >= detail_->yaml.size()) return false;
  if(isample >= detail_->yaml[icategory].size()) return false;
  if(sample_configuration) *sample_configuration = detail_->yaml[icategory][isample];
  if(category_configuration) *category_configuration = detail_->yaml[icategory];
  return true;
}

bool CategorizedTreeInput::get_sample_configuration(const string &sample,
    YAML::Node *sample_configuration, YAML::Node *category_configuration) const
{
  auto iter = detail_->sample_index.find(sample);
  if(iter == detail_->sample_index.end()) return false;
  if(sample_configuration) *sample_configuration = iter->second.first;
  if(category_configuration) *category_configuration = iter->second.second;
  return true;
}

string CategorizedTreeInput::get_category() const
{
  YAML::Node *category = detail_->current_category;
  return category ? (*category)["name"].as<string>() : nullptr;
}

string CategorizedTreeInput::get_sample() const
{
  YAML::Node *sample = detail_->current_sample;
  return sample ? (*sample)["name"].as<string>() : nullptr;
}

size_t CategorizedTreeInput::get_icategory() const
{
  YAML::Node *category = detail_->current_category;
  return category ? (*category)["_runtime_id"].as<size_t>() : -1;
}

size_t CategorizedTreeInput::get_isample() const
{
  YAML::Node *sample = detail_->current_sample;
  return sample ? (*sample)["_runtime_id"].as<size_t>() : -1;
}

bool CategorizedTreeInput::get_category_configuration(YAML::Node *configuration) const
{
  YAML::Node *category = detail_->current_category;
  if(!category) return false;
  if(configuration) *configuration = *category;
  return true;
}

bool CategorizedTreeInput::get_sample_configuration(YAML::Node *configuration) const
{
  YAML::Node *sample = detail_->current_sample;
  if(!sample) return false;
  if(configuration) *configuration = *sample;
  return true;
}

size_t CategorizedTreeInput::get_category_nevent(const string &category) const
{
  YAML::Node configuration;
  if(!get_category_configuration(category, &configuration)) return 0;
  return configuration["_runtime_nevent"].as<size_t>();
}

size_t CategorizedTreeInput::get_sample_nevent(const string &sample) const
{
  YAML::Node configuration;
  if(!get_sample_configuration(sample, &configuration)) return 0;
  return configuration["_runtime_nevent"].as<size_t>();
}
