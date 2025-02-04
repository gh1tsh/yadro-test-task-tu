#ifndef TAPE_DEV_UTILS
#define TAPE_DEV_UTILS

#include <algorithm>
#include <string>

inline bool stringStartsWith(const std::string& t_str,
                             const std::string& t_prefix) {
  return t_str.rfind(t_prefix, 0) == 0;
}

inline std::string splitAfterDelimiter(const std::string& t_str,
                                       char t_delim = ':') {
  return t_str.substr(t_str.find(t_delim) + 1);
}

// https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring

/// trim from start (in place)
inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

/// trim from end (in place)
inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

/// trim from both ends (in place)
inline void trim(std::string& s) {
  rtrim(s);
  ltrim(s);
}

/// trim from start (copying)
inline std::string ltrim_copy(std::string s) {
  ltrim(s);
  return s;
}

/// trim from end (copying)
inline std::string rtrim_copy(std::string s) {
  rtrim(s);
  return s;
}

/// trim from both ends (copying)
inline std::string trim_copy(std::string s) {
  trim(s);
  return s;
}

#endif  // TAPE_DEV_UTILS