#include "fs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <new>
#include <algorithm>

using namespace std;

namespace {

int DT_MAP(unsigned char d_type)
{
  switch(d_type) {
    case DT_BLK: return ListDir::DT_BLK;
    case DT_CHR: return ListDir::DT_CHR;
    case DT_DIR: return ListDir::DT_DIR;
    case DT_FIFO: return ListDir::DT_FIFO;
    case DT_LNK: return ListDir::DT_LNK;
    case DT_REG: return ListDir::DT_REG;
    case DT_SOCK: return ListDir::DT_SOCK;
    case DT_UNKNOWN: return ListDir::DT_UNKNOWN;
  }
  throw runtime_error("unknown d_type: " + to_string(d_type));
}

vector<long long> parse_numbers(const string &str)
{
  vector<long long> nums;
  size_t pos = 0;
  while(pos < str.size()) {
    pos = str.find_first_of("0123456789", pos);
    if(pos == str.npos) break;
    size_t end = str.find_first_not_of("0123456789", pos);
    if(end == str.npos) end = str.size();
    nums.push_back(stoll(str.substr(pos, end - pos)));
    pos = end;
  }
  return nums;
}

}  // namespace

const int ListDir::DT_BLK     =   1;
const int ListDir::DT_CHR     =   2;
const int ListDir::DT_DIR     =   4;
const int ListDir::DT_FIFO    =   8;
const int ListDir::DT_LNK     =  16;
const int ListDir::DT_REG     =  32;
const int ListDir::DT_SOCK    =  64;
const int ListDir::DT_UNKNOWN = 128;
const int ListDir::DT_ALL     = 255;

ListDir::ListDir(const string &dirpath_in, int accept)
{
  if(dirpath_in.empty()) {
    throw invalid_argument("empty dirpath");
  }

  DIR *dir = opendir(dirpath_in.c_str());
  if(dir == NULL) {
    throw runtime_error(string("opendir: ") +
        dirpath_in + ": " + strerror(errno));
  }
  shared_ptr<DIR> dir_guard(dir, [](DIR *d) { closedir(d); });

  vector<string> names;
  for(;;) {
    errno = 0;
    struct dirent *ent = readdir(dir);
    if(ent == NULL) {
      if(errno == 0) break;
      throw runtime_error(string("readdir: ") +
          dirpath_in + ": " + strerror(errno));
    }
    if(DT_MAP(ent->d_type) & accept) {
      names.push_back(ent->d_name);
    }
  }

  dirpath = dirpath_in;
  entnames = std::move(names);
}

vector<string> ListDir::get_full_names() const
{
  string prefix = dirpath;
  if(prefix.back() != '/') {
    prefix += '/';
  }

  vector<string> names;
  names.reserve(entnames.size());
  for(const string &name : entnames) {
    names.emplace_back(prefix + name);
  }
  return names;
}

ListDir &ListDir::sort_by_numbers()
{
  vector<pair<vector<long long>, size_t>> keys;
  keys.reserve(entnames.size());
  size_t i = 0;
  for(const string &name : entnames) {
    keys.emplace_back(parse_numbers(name), i++);
  }
  sort(keys.begin(), keys.end());
  vector<string> names;
  names.reserve(entnames.size());
  for(const auto &key : keys) {
    names.emplace_back(std::move(entnames[key.second]));
  }
  entnames = std::move(names);
  return *this;
}

Stat::Stat(const char *path)
{
  sbuf = (struct stat *)malloc(sizeof *sbuf);
  if(sbuf == NULL) throw bad_alloc();
  int r = stat(path, sbuf);
  if(r < 0) {
    free(sbuf);
    sbuf = NULL;
    r = errno;
    if(r == ENOENT) return;
    throw runtime_error(path + string(": ") + strerror(r));
  }
}

Stat::~Stat()
{
  free(sbuf);
}

bool Stat::isreg() const
{
  return sbuf && S_ISREG(sbuf->st_mode);
}

bool Stat::isdir() const
{
  return sbuf && S_ISDIR(sbuf->st_mode);
}

bool Stat::islnk() const
{
  return sbuf && S_ISLNK(sbuf->st_mode);
}

size_t Stat::size() const
{
  return sbuf ? sbuf->st_size : 0;
}

string basename(const string &path)
{
  size_t pos = path.rfind("/");
  if(pos == path.npos) return path;
  return path.substr(pos + 1);
}

pair<string, string> dotsplit(const string &path)
{
  size_t pos = path.rfind(".");
  if(pos == path.npos) return { path, "" };
  return { path.substr(0, pos), path.substr(pos + 1) };
}
