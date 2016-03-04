// Taken from https://www.preney.ca/paul/archives/486
// In the future use std:;apply -> http://en.cppreference.com/w/cpp/experimental/apply

#ifndef __TUPLEAPPLY__
#define __TUPLEAPPLY__

#include <tuple>

// Define holder for indices...
template <std::size_t...>
struct indices;

// Define adding the Nth index...
// Notice one argument, Type, is discarded each recursive call.
// Notice N is added to the end of Indices... .
// Notice N is incremented with the recursion.
template <std::size_t N, typename Indices, typename... Types>
struct make_indices_impl;
template <
  std::size_t N,
  std::size_t... Indices,
  typename Type,
  typename... Types
>
struct make_indices_impl<N, indices<Indices...>, Type, Types...> {
  typedef
    typename make_indices_impl<
      N+1,
      indices<Indices..., N>,
      Types...
    >::type
    type;
};

// Define adding the last index...
// Notice no Type or Types... are left.
// Notice the full solution is emitted into the container.
template <std::size_t N, std::size_t... Indices>
struct make_indices_impl<N, indices<Indices...>> {
  typedef indices<Indices...> type;
};

// Compute the indices starting from zero...
// Notice indices container is empty.
// Notice Types... will be all of the tuple element types.
// Notice this refers to the full solution (i.e., via ::type).
template <std::size_t N, typename... Types>
struct make_indices {
  typedef
    typename make_indices_impl<0, indices<>, Types...>::type
    type;
};




template <
  typename Indices
>
struct apply_tuple_impl;

template <
  template <std::size_t...> class I,
  std::size_t... Indices
>
struct apply_tuple_impl<I<Indices...>> {
  // Rvalue apply_tuple()...
  template <
    typename Op,
    typename... OpArgs,
    template <typename...> class T
  >
  static auto apply_tuple(Op&& op, T<OpArgs...>&& t)
    -> typename std::result_of<Op(OpArgs...)>::type {
    return op(
      std::forward<OpArgs>(std::get<Indices>(t))...);
  }

  // Lvalue apply_tuple()...
  // Notice the added const used with std::forward's OpArgs.
  template <
    typename Op,
    typename... OpArgs,
    template <typename...> class T
  >
  static auto apply_tuple(Op&& op, T<OpArgs...> const& t)
    -> typename std::result_of<Op(OpArgs...)>::type {
    return op(
      std::forward<OpArgs const>(std::get<Indices>(t))...);
  }
};



// Rvalue apply_tuple()...
template <
  typename Op,
  typename... OpArgs,
  typename Indices = typename make_indices<0, OpArgs...>::type,
  template <typename...> class T
>
auto apply_tuple(Op&& op, T<OpArgs...>&& t)
  -> typename std::result_of<Op(OpArgs...)>::type {
  return apply_tuple_impl<Indices>::apply_tuple(
    std::forward<Op>(op),
    std::forward<T<OpArgs...>>(t));
}

// Lvalue apply_tuple()...
// std::forward could have been removed. It was kept
//   - to keep appearance of code close to original
//   - to show that, if kept, const needs to be used
template <
  typename Op,
  typename... OpArgs,
  typename Indices = typename make_indices<0, OpArgs...>::type,
  template <typename...> class T
>
auto apply_tuple(Op&& op, T<OpArgs...> const& t)
  -> typename std::result_of<Op(OpArgs...)>::type {
  return apply_tuple_impl<Indices>::apply_tuple(
    std::forward<Op>(op),
    std::forward<T<OpArgs...> const>(t));
}

#endif
