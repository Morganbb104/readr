#include <Rcpp.h>
using namespace Rcpp;
#include <fstream>
#include "grisu3.h"

// Defined later to make copyright clearer
template <class Stream>
void stream_delim(Stream& output, const RObject& x, int i, char delim, const std::string& na);

template <class Stream>
void stream_delim_row(Stream& output, const Rcpp::List& x, int i, char delim, const std::string& na) {
  int p = Rf_length(x);

  for (int j = 0; j < p - 1; ++j) {
    stream_delim(output, x.at(j), i, delim, na);
    output << delim;
  }
  stream_delim(output, x.at(p - 1), i, delim, na);

  output << '\n';
}

bool needs_quote(const char* string, char delim, const std::string& na) {
  if (string == na)
    return true;

  for (const char* cur = string; *cur != '\0'; ++cur) {
    if (*cur == '\n' || *cur == '\r' || *cur == '"' || *cur == delim)
      return true;
  }

  return false;
}

template <class Stream>
void stream_delim(Stream& output, const char* string, char delim, const std::string& na) {
  bool quotes = needs_quote(string, delim, na);

  if (quotes)
    output << '"';

  for (const char* cur = string; *cur != '\0'; ++cur) {
    switch (*cur) {
    case '"':
      output << "\"\"";
      break;
    default:
      output << *cur;
    }
  }

  if (quotes)
    output << '"';
}

template <class Stream>
void stream_delim(Stream& output, const List& df, char delim, const std::string& na,
                  bool col_names = true, bool append = false) {
  int p = Rf_length(df);
  if (p == 0)
    return;

  if (col_names) {
    CharacterVector names = as<CharacterVector>(df.attr("names"));
    for (int j = 0; j < p; ++j) {
      stream_delim(output, names, j, delim, na);
      if (j != p - 1)
        output << delim;
    }
    output << '\n';
  }

  RObject first_col = df[0];
  int n = Rf_length(first_col);

  for (int i = 0; i < n; ++i) {
    stream_delim_row(output, df, i, delim, na);
  }
}

// [[Rcpp::export]]
std::string stream_delim(const List& df, const std::string& path, char delim, const std::string& na,
                         bool col_names = true, bool append = false) {

  // From the max_digits10 proposal http://www2.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1822.pdf
  // This will be 17 for IEEE doubles
  const int digits = 2 + std::numeric_limits<double>::digits * 3010/10000;

  if (path == "") {
    std::ostringstream output;

    output.precision(digits);
    stream_delim(output, df, delim, na, col_names, append);
    return output.str();
  } else {
    std::ofstream output(path.c_str(), append ? std::ofstream::app : std::ofstream::trunc);
    if (output.fail()) {
      stop("Failed to open '%s'.", path);
    }
    output.precision(digits);
    stream_delim(output, df, delim, na, col_names, append);
    return "";
  }
}

// =============================================================================
// Derived from EncodeElementS in RPostgreSQL
// Written by: tomoakin@kenroku.kanazawa-u.ac.jp
// License: GPL-2

template <class Stream>
void stream_delim(Stream& output, const RObject& x, int i, char delim, const std::string& na) {
  switch (TYPEOF(x)) {
  case LGLSXP: {
    int value = LOGICAL(x)[i];
    if (value == TRUE) {
      output << "TRUE";
    } else if (value == FALSE) {
      output << "FALSE";
    } else {
      output << na;
    }
    break;
  }
  case INTSXP: {
    int value = INTEGER(x)[i];
    if (value == NA_INTEGER) {
      output << na;
    } else {
      output << value;
    }
    break;
  }
  case REALSXP:{
    double value = REAL(x)[i];
    if (!R_FINITE(value)) {
      if (ISNA(value)) {
        output << na;
      } else if (ISNAN(value)) {
        output << "NaN";
      } else if (value > 0) {
        output << "Inf";
      } else {
        output << "-Inf";
      }
    } else {
      char str[32];
      int len = dtoa_grisu3(value, str);
      output << str;
    }
    break;
  }
  case STRSXP: {
    if (STRING_ELT(x, i) == NA_STRING) {
      output << na;
    } else {
      stream_delim(output, Rf_translateCharUTF8(STRING_ELT(x, i)), delim, na);
    }
    break;
  }
  default:
    Rcpp::stop("Don't know how to handle vector of type %s.",
      Rf_type2char(TYPEOF(x)));
  }
}
