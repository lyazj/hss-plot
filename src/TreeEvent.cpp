#include "../include/TreeEvent.h"
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TDataType.h>
#include <TObjArray.h>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <stdlib.h>

using namespace std;

class TreeEvent::Detail {
public:
  vector<string> filenames;
  size_t ifilename;  // index into filenames
  size_t local_index;  // -1 if not opened
  size_t global_index;  // <total events read> if at the end
  unique_ptr<TFile> file;
  unique_ptr<TTree> tree;
  vector<string> branch_names;
  vector<TBranch *> branches;
  vector<unique_ptr<void, function<void(void *)>>> branch_data;
  vector<size_t> branch_current_size;
  vector<size_t> branch_elem_size;
  vector<size_t> branch_nelem_max;

  Int_t GetEntry(Long64_t entry) {
    Int_t total = 0;
    for(size_t i = 0; i < branches.size(); ++i) {
      Int_t current = branches[i]->GetEntry(entry);
      if(current <= 0) return current;
      branch_current_size[i] = current;
      total += current;
    }
    return total;
  }
};

TreeEvent::TreeEvent(const char *name)
  : name_(name)
{
  detail_ = new Detail;
  detail_->ifilename = -1;
  detail_->local_index = -1;
  detail_->global_index = -1;
}

TreeEvent::~TreeEvent()
{
  delete detail_;
}

const char *TreeEvent::get_filename() const
{
  return get_filename(get_ifilename());
}

size_t TreeEvent::get_ifilename() const
{
  return detail_->ifilename;
}

size_t TreeEvent::get_local_index() const
{
  return detail_->local_index;
}

size_t TreeEvent::get_global_index() const
{
  return detail_->global_index;
}

size_t TreeEvent::add_filename(const char *filename)
{
  size_t i = detail_->filenames.size();
  detail_->filenames.push_back(filename);
  return i;
}

size_t TreeEvent::get_nfilename() const
{
  return detail_->filenames.size();
}

const char *TreeEvent::get_filename(size_t i) const
{
  return i >= get_nfilename() ? nullptr : detail_->filenames[i].c_str();
}

size_t TreeEvent::add_branch(const char *filename)
{
  size_t i = detail_->branch_names.size();
  detail_->branch_names.push_back(filename);
  return i;
}

size_t TreeEvent::get_nbranch() const
{
  return detail_->branch_names.size();
}

const char *TreeEvent::get_branch(size_t i) const
{
  return i >= get_nbranch() ? nullptr : detail_->branch_names[i].c_str();
}

void *TreeEvent::get_branch_data(size_t i, size_t *nelem) const
{
  if(i >= detail_->branch_data.size()) return nullptr;
  if(nelem) *nelem = detail_->branch_current_size[i] / detail_->branch_elem_size[i];
  return detail_->branch_data[i].get();
}

static size_t get_branch_elem_size_impl(TBranch *branch)
{
  TClass *c; EDataType e;
  if(branch->GetExpectedType(c, e)) return 0;
  if(c) return sizeof(void *);
  return TDataType::GetDataType(e)->Size();
}

size_t TreeEvent::get_branch_elem_size(size_t i) const
{
  if(i >= detail_->branch_elem_size.size()) return 0;
  return detail_->branch_elem_size[i];
}

static size_t get_branch_nelem_max_impl(TBranch *branch)
{
  TObjArray *leaves = branch->GetListOfLeaves();
  if(leaves->GetEntries() != 1) return 0;  // [XXX] not implemented
  TLeaf *leaf = (TLeaf *)leaves->UncheckedAt(0);
  Int_t count = 0;
  TLeaf *leafcnt = leaf->GetLeafCounter(count);
  if(leafcnt) count = leafcnt->GetMaximum();
  return max(count, (Int_t)0);
}

size_t TreeEvent::get_branch_nelem_max(size_t i) const
{
  if(i >= detail_->branch_nelem_max.size()) return 0;
  return detail_->branch_nelem_max[i];
}

bool TreeEvent::next()
{
  // The most frequent case: step forward.
  if(detail_->tree) {
    if(detail_->GetEntry(detail_->local_index + 1) > 0) {
      ++detail_->local_index;
      ++detail_->global_index;
      return true;
    }

    // Reading failed. Close current file.
    if(detail_->local_index == (size_t)-1) {
      cerr << "Warning: empty tree in file: " << get_filename() << endl;
    }
    detail_->tree.reset();
    detail_->file.reset();
    detail_->local_index = -1;
  }

  // Maybe already at the end.
  if(detail_->ifilename == get_nfilename()) return false;

  // No file opened. Try to open the next file to read.
  for(;;) {
    const char *filename = get_filename(++detail_->ifilename);
    if(filename == nullptr) break;

    unique_ptr<TFile> file(new TFile(filename));
    if(!file->IsOpen()) {
      cerr << "Warning: skipping broken file: " << filename << endl;
      continue;
    }

    unique_ptr<TTree> tree(dynamic_cast<TTree *>(file->Get(name_)));
    if(!tree) {
      cerr << "Warning: skipping empty file: " << filename << endl;
      continue;
    }

    vector<TBranch *> branches;
    vector<unique_ptr<void, function<void(void *)>>> branch_data;
    vector<size_t> branch_current_size;
    vector<size_t> branch_elem_size;
    vector<size_t> branch_nelem_max;

    for(const string &name : detail_->branch_names) {
      TBranch *branch = tree->GetBranch(name.c_str());
      if(!branch) {
        cerr << "Warning: skipping file missing branch " << name << ": " << filename << endl;
        goto CONTINUE;
      }
      size_t elem_size = get_branch_elem_size_impl(branch);
      size_t nelem_max = get_branch_nelem_max_impl(branch);
      if(elem_size == 0 || nelem_max == 0) {
        cerr << "Warning: skipping file with unsupported branch " << name << ": " << filename << endl;
        goto CONTINUE;
      }
      char *buf = (char *)calloc(nelem_max, elem_size);  // zero-initialized, important for TLeafObject
      if(buf == NULL) {
        cerr << "Warning: skipping file with branch " << name << " unallocable: " << filename << endl;
        goto CONTINUE;
      }
      branch->SetAddress(buf);
      branches.emplace_back(branch);
      branch_data.emplace_back(buf, [](void *p) { free(p); });
      branch_current_size.push_back(0);
      branch_elem_size.push_back(elem_size);
      branch_nelem_max.push_back(nelem_max);
    }

    detail_->file = std::move(file);
    detail_->tree = std::move(tree);
    detail_->branches = std::move(branches);
    detail_->branch_data = std::move(branch_data);
    detail_->branch_current_size = std::move(branch_current_size);
    detail_->branch_elem_size = std::move(branch_elem_size);
    detail_->branch_nelem_max = std::move(branch_nelem_max);
    return next();

    CONTINUE: continue;
  }

  // Reach the end.
  ++detail_->global_index;
  detail_->branches.clear();
  detail_->branch_data.clear();
  detail_->branch_current_size.clear();
  detail_->branch_elem_size.clear();
  detail_->branch_nelem_max.clear();
  return false;
}
