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

template<typename RET, typename F, typename T>
inline void promise_set_value(std::promise<RET>& p, F& f, T& t) {
  p.set_value(f(t));
}

template<typename F, typename T>
inline void promise_set_value(std::promise<void>& p, F& f, T& t) {
  f(t);
  p.set_value();
}

template<class T>
class Concurrent {
 public:
  explicit Concurrent(T t = T{}) : _done(false), _t(t), _thread([this]() {
  while (!_done) {
    _queue.pop()();
  }}) {
  }

  ~Concurrent() {
    _queue.push([this](){ _done = true; });
    _thread.join();
  }

  template<typename F, typename RET = typename std::result_of<F(T)>::type>
  auto operator()(F f) -> std::future<RET> {
    auto promise = std::make_shared<std::promise<RET>>();
    auto future = promise->get_future();

    _queue.push([this, f, promise](){
      try {
        promise_set_value(*promise, f, _t);
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    });
    return future;
  }

 private:
  bool _done;
  T _t;
  ConcurrencyQueue<std::function<void()>> _queue;
  std::thread _thread;
};

#endif
