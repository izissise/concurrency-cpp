#ifndef __CONCURRENTHPP__
#define __CONCURRENTHPP__

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ConcurrencyQueue.hpp"

template<typename RET, typename F, typename... Types>
inline void promise_set_value(std::promise<RET>& p, F& f, Types&&... ts) {
  p.set_value(f(std::forward<Types>(ts)...));
}

template<typename F, typename... Types>
inline void promise_set_value(std::promise<void>& p, F& f, Types&&... ts) {
  f(std::forward<Types>(ts)...);
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

template<class... Types>
class Concurrent {
 public:
  explicit Concurrent(Types&&... ts) : _done(false),
  _t(std::forward_as_tuple(ts...)),
  _thread([this]() {
  while (!_done) {
    _queue.pop()();
  }}) {
  }

  ~Concurrent() {
    _queue.push([this](){ _done = true; });
    _thread.join();
  }

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

 private:
  bool _done;
  std::tuple<Types...> _t;
  ConcurrencyQueue<std::function<void()>> _queue;
  std::thread _thread;
};

#endif
