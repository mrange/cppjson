# cppjson

cppjson is a [conforming](http://jsonlint.com) [JSON](http://json.org) parser for C++ licensed under
[Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)

Like [minijson_reader](https://github.com/giacomodrago/minijson_reader) cppjson wants to allow
developers parse JSON documents without the overhead of DOM.

For developers that don't mind DOM like parsing of JSON document a simple DOM is provided.

cppjson is based on my work on [MiniJson](https://github.com/mrange/minijson) for F#
(not to be confused with minijson_reader, I discovered minijson_reader after creating MiniJson for F#)
cppjson uses MiniJson as a reference parser (MiniJson has a rather extensive test suite).

Like MiniJson for F# cppjson wants to provide decent error messages, example:
```
Failed to parse input as JSON
{"abc":}
-------^ Pos: 7
Expected: '"', '-', '[', '{', digit, false, null or true
```

cppjson dom parsing
```cpp
#include "cpp_json__document.hpp"

void parse_json (std::wstring const & json)
{
  using namespace cpp_json::document;

  std::size_t         pos     ;
  json_document::ptr  document;
  std::wstring        error   ;

  if (!json_parser::parse (json, pos, document))
  {
    std::wcout
      << L"FAILURE: Pos: " << pos << std::endl
      << error << std::endl
      ;
  }
  else
  {
    auto root = document->root ();
    auto sz = root->size ();
    for (auto iter = 0U; iter < sz; ++iter)
    {
      std::wcout << root->at (iter)->as_string () << std::endl;
    }
  }
}
```

cppjson callback parsing
```cpp
#include "cpp_json__parser.hpp"

struct some_json_context
{
  using string_type = std::wstring    ; // Type of string, typically std::wstring
  using char_type   = wchar_t         ; // Type of char, typically wchar_t
  using iter_type   = wchar_t const * ; // Type of string "iterator", typically wchar_t const *

  // expected* methods are invoked when parser expected a token but didn't find it
  //  Note: certain elements are considered optional so a call to an expected* method might not stop parsing
  void expected_char    (std::size_t pos, char_type ch) noexcept
  void expected_chars   (std::size_t pos, string_type const & chs) noexcept
  void expected_token   (std::size_t pos, string_type const & token) noexcept

  // unexpected* methods are invoked when parser encountered an unexpected token
  //  Note: certain elements are considered optional so a call to an unexpected* method might not stop parsing
  void unexpected_token (std::size_t pos, string_type const & token);

  // Methods used to build string values

  // Clears cached string
  void clear_string ();
  // Appends an 8-bit or 16-bit char to string (depending on char_type)
  void push_char (char_type ch);
  // Appends an 16-bit char to string (allows encoding when char_type is char)
  void push_wchar_t (wchar_t ch);
  // Gets cached string
  string_type const & get_string ();

  // The following methods are invoked when JSON values are discovered

  bool array_begin ();
  bool array_end ();

  bool object_begin ();
  bool member_key (string_type const & s);
  bool object_end ();

  bool bool_value (bool b);

  bool null_value ();

  bool string_value (string_type const & s);

  bool number_value (double d);
};


void parse_json (std::wstring const & json)
{
  using namespace cpp_json::parser;

  auto b = json.c_str ()    ;
  auto e = b + json.size () ;
  json_parser<some_json_context> jp (b, e);

  auto result = jp.try_parse__json ();
  if (!result)
  {
    // Handle error
    // ...
  }

  // Parse successful
  // ...
}
```

# TODO

1. Improve error message test coverage
1. Add test cases for memory allocation
1. Add mutability to json_document
1. Remove C++11 dependency
