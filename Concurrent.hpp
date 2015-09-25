#ifndef __CONCURRENTHPP__
#define __CONCURRENTHPP__

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tupleApply.hpp"

#include "ConcurrencyQueue.hpp"

//! An helper function to set the value on a promise even if a function return void
template<typename RET, typename F, typename... Types>
inline void promise_set_value(std::promise<RET>& p, F& f, Types&&... ts) {
  p.set_value(f(std::forward<Types&&>(ts)...));
}

template<typename F, typename... Types>
inline void promise_set_value(std::promise<void>& p, F& f, Types&&... ts) {
  f(std::forward<Types&&>(ts)...);
  p.set_value();
}

template<typename RET, typename F, typename Tuple>
inline void promise_set_value(std::promise<RET>& p, F& f, Tuple&& t) {
  p.set_value(apply_tuple(f, t));
}

template<typename F, typename Tuple>
inline void promise_set_value(std::promise<void>& p, F& f, Tuple&& t) {
  apply_tuple(f, t);
  p.set_value();
}

//! A class that implement a worker thread which allow to share ressources and defer
//! the work on that thread.
//! @tparam Types The ressources types;
template<class... Types>
class Concurrent {
 public:

  //! Concurrent constructor
  //! @param ts The shared ressources values
  explicit Concurrent(Types&&... ts) : _done(false),
  _t(std::forward_as_tuple(ts...)),
  _thread([this]() {
  while (!_done) {
    _queue.pop()();
  }}) {
  }

  //! Concurrent Destructor
  //! It assure that the worker thread is done before destroying the object
  ~Concurrent() {
    _queue.push([this](){ _done = true; });
    _thread.join();
  }

  //! Allow to push new function to execute concurrently on the shared ressources
  //! @param f The function that shall be called (it must take the shared ressource as parameter)
  //! @return An std::future of the result of f
  template<typename F,
  typename RET = typename std::result_of<F(Types...)>::type,
  int ...I>
  auto operator()(F f) -> std::future<RET> {
    auto promise = std::make_shared<std::promise<RET>>();
    auto future = promise->get_future();

    _queue.push([this, f, promise](){
      auto& p = *promise;
      try {
        promise_set_value(p, f, _t);
      } catch (...) {
        p.set_exception(std::current_exception());
      }
    });
    return future;
  }

  //! Set the new shared ressources
  //! @param ts The ressources
  //! @return The previous ressources as a tuple
  std::tuple<Types...> setSharedRessource(Types&&... ts) {
    auto nTuple = std::forward_as_tuple(ts...);
    std::swap(_t, nTuple);
    return nTuple;
  }

  //! Get the shared ressources
  //! @return The ressources as a tuple
  const std::tuple<Types...>& getSharedRessource() const {
    return _t;
  }

 private:
  bool _done;
  std::tuple<Types...> _t;
  ConcurrencyQueue<std::function<void()>> _queue;
  std::thread _thread;
};

#endif
