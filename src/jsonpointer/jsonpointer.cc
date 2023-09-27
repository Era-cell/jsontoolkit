#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include "stringify.h"

#include <functional>  // std::reference_wrapper
#include <iterator>    // std::cbegin, std::cend, std::prev
#include <memory>      // std::allocator
#include <type_traits> // std::is_same_v
#include <utility>     // std::move

namespace {
template <template <typename T> typename Allocator, typename V>
auto traverse(V &document,
              typename sourcemeta::jsontoolkit::GenericPointer<
                  typename V::Char, typename V::CharTraits,
                  Allocator>::const_iterator begin,
              typename sourcemeta::jsontoolkit::GenericPointer<
                  typename V::Char, typename V::CharTraits,
                  Allocator>::const_iterator end) -> V & {
  using Pointer = sourcemeta::jsontoolkit::GenericPointer<
      typename V::Char, typename V::CharTraits, Allocator>;
  // Make sure types match
  static_assert(
      std::is_same_v<typename Pointer::Value, std::remove_const_t<V>>);

  std::reference_wrapper<V> current{document};

  // Evaluation of a JSON Pointer begins with a reference to the root
  // value of a JSON document and completes with a reference to some value
  // within the document.  Each reference token in the JSON Pointer is
  // evaluated sequentially.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  for (auto iterator = begin; iterator != end; ++iterator) {
    if (iterator->is_property()) {
      // If the currently referenced value is a JSON object, the new
      // referenced value is the object member with the name identified by
      // the reference token.  The member name is equal to the token if it
      // has the same number of Unicode characters as the token and their
      // code points are byte-by-byte equal.  No Unicode character
      // normalization is performed.
      // See https://www.rfc-editor.org/rfc/rfc6901#section-4
      current = current.get().at(iterator->to_property());
    } else {
      // If the currently referenced value is a JSON array, the reference
      // token MUST contain [...] characters comprised of digits (see ABNF
      // below; note that leading zeros are not allowed) that represent an
      // unsigned base-10 integer value, making the new referenced value the
      // array element with the zero-based index identified by the
      // token.
      // See https://www.rfc-editor.org/rfc/rfc6901#section-4
      current = current.get().at(iterator->to_index());
    }
  }

  return current.get();
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto get(const JSON &document, const Pointer &pointer) -> const JSON & {
  return traverse<std::allocator, const JSON>(document, std::cbegin(pointer),
                                              std::cend(pointer));
}

auto get(JSON &document, const Pointer &pointer) -> JSON & {
  return traverse<std::allocator, JSON>(document, std::cbegin(pointer),
                                        std::cend(pointer));
}

auto set(JSON &document, const Pointer &pointer, const JSON &value) -> void {
  if (pointer.empty()) {
    document.into(value);
    return;
  }

  JSON &current{traverse<std::allocator, JSON>(document, std::cbegin(pointer),
                                               std::prev(std::cend(pointer)))};
  const auto last{pointer.back()};
  // Handle the hyphen as a last constant
  // If the currently referenced value is a JSON array, the reference
  // token [can be ] the single character "-", making the new referenced value
  // the (nonexistent) member after the last array element.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  if (current.is_array() && last.is_hyphen()) {
    current.push_back(value);
  } else if (last.is_property()) {
    current.at(last.to_property()).into(value);
  } else {
    current.at(last.to_index()).into(value);
  }
}

auto set(JSON &document, const Pointer &pointer, JSON &&value) -> void {
  if (pointer.empty()) {
    document.into(value);
    return;
  }

  JSON &current{traverse<std::allocator, JSON>(document, std::cbegin(pointer),
                                               std::prev(std::cend(pointer)))};
  const auto last{pointer.back()};
  // Handle the hyphen as a last constant
  // If the currently referenced value is a JSON array, the reference
  // token [can be ] the single character "-", making the new referenced value
  // the (nonexistent) member after the last array element.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  if (current.is_array() && last.is_hyphen()) {
    current.push_back(value);
  } else if (last.is_property()) {
    current.at(last.to_property()).into(std::move(value));
  } else {
    current.at(last.to_index()).into(std::move(value));
  }
}

auto stringify(const Pointer &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<JSON::Char, JSON::CharTraits, std::allocator>(pointer, stream);
}

} // namespace sourcemeta::jsontoolkit