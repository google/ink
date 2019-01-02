/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_SCENE_TYPES_EVENT_DISPATCH_H_
#define INK_ENGINE_SCENE_TYPES_EVENT_DISPATCH_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "third_party/absl/container/flat_hash_map.h"

namespace ink {

// These define RAII base class helpers for event dispatch. The EventDispatch
// maintains a strongly-type list of pointers to the listeners, and can be used
// to call member functions on the listener types via the Send() function.
//
// There are 4 important classes that make up an event system:
//   1) The event provider.
//        - Your class, it's the thing that decides when to send out events.
//        - Owns a shared_ptr<dispatch>.
//   2) The event dispatch.
//        - Defined here.
//        - Specialized on the listener interface.
//   3) The listener interface.
//        - Your class.
//        - Should be pure virtual.
//        - Should use CRTP inheritance from the base listener class defined
//          here. (EventListener<>)
//   4) The listeners.
//        - Your class.
//        - Inherits from the listener interface.
//
// It is important to note that the order in which the listeners are notified is
// non-determinate.
//
// Listeners may be added while dispatching is taking place. However, listeners
// will only receive an event if they were registered when the event was
// dispatched.
//
// Listeners may also be removed while dispatching is taking place. Note,
// however, that the non-deterministic order means that a listener may or may
// not receive the event that resulted in their removal.
//
// Adding and removing the same listener while dispatching is taking place
// results in undefined behavior.
//
// The memory model is:
//  - Listeners have strong pointers to the dispatch
//  - The event producer has a strong pointer to the dispatch
//  - The dispatch has raw pointers to listeners
// Thus:
//  - Listeners DO keep the dispatch alive.
//  - Listeners DO NOT keep the event provider alive.
//  - The event provider DOES NOT keep the event listeners alive.
//  - The event dispatch DOES NOT keep the event listeners alive.
//
// Example:
// class MyEventProvider {
// public:
//   void RegisterListener(MyEventListenerInterface* listener) {
//     listener->RegisterOnDispatch(dispatch_);
//   }
//   void UnregisterListener(MyEventListenerInterface* listener) {
//     listener->UnregisterOnDispatch(dispatch_);
//   }
//   void CauseEvent() {
//     dispatch_->Send(&MyEventListenerInterface::HandleEvent, args...);
//   }
//
// private:
//   std::shared_ptr<MyEventListenerInterface::Dispatch> dispatch_ =
//                      absl::make_shared<MyEventListenerInterface::Dispatch>();
// };
//
// class MyEventListenerInterface
//     : public EventListener<MyEventListenerInterface> {
//   void HandleEvent(...) = 0;
// }
//
// class MyEventListenerA : public MyEventListenerInterface {
//   void HandleEvent(...) override { /* do stuff! */ }
// }
template <typename T>
class EventListener;

template <typename TListener>
class EventDispatch {
 public:
  typedef uint32_t Token;
  static const Token invalid_token = 0;

  EventDispatch() : enabled_(true), next_token_(1) {}

  // Disallow copy and assign.
  EventDispatch(const EventDispatch&) = delete;
  EventDispatch& operator=(const EventDispatch&) = delete;

  static std::shared_ptr<EventDispatch<TListener>> MakeWithListeners(
      const std::vector<TListener*>& listeners) {
    auto res = std::make_shared<EventDispatch<TListener>>();
    for (auto l : listeners) {
      l->EventListener<TListener>::RegisterOnDispatch(res);
    }
    return res;
  }

  template <typename Function, typename... Args>
  void Send(Function func, const Args&... args) const {
    SendFiltered(nullptr, func, args...);
  }

  template <typename Function, typename... Args>
  void SendFiltered(std::function<bool(TListener*)> filter, Function func,
                    const Args&... args) const {
    if (!enabled_) return;
    auto member_func = std::mem_fn(func);

    std::vector<std::pair<Token, TListener*>> copy;
    copy.reserve(listeners_.size());
    std::copy(listeners_.begin(), listeners_.end(), std::back_inserter(copy));
    for (const auto& pair : copy) {
      if (IsTokenValid(pair.first, pair.second) &&
          (!filter || filter(pair.second))) {
        member_func(pair.second, args...);
      }
    }
  }

  size_t size() const { return listeners_.size(); }

  bool Enabled() const { return enabled_; }
  void SetEnabled(bool enabled) { enabled_ = enabled; }

 private:
  bool IsTokenValid(Token token, TListener* expected) const {
    auto ai = listeners_.find(token);
    if (ai == listeners_.end()) return false;
    if (ai->second != expected) return false;
    return true;
  }

  Token RegisterListener(TListener* t) {
    auto res = next_token_++;
    listeners_[res] = t;
    return res;
  }

  void UnregisterListener(Token token) { listeners_.erase(token); }

  bool enabled_;
  absl::flat_hash_map<Token, TListener*> listeners_;
  Token next_token_;

  template <typename T>
  friend class EventListener;
};

// EventListener is meant to be subclassed, with the subclass
// providing callback handlers as appropriate.
// See EventDispatch for usage info
//
// TListener must be:
//  - The same type as the event dispatch TListener
//  - The same type as the class subclassing EventListener
// Ex:
// class MyEventListenerSubclass : EventListener<MyEventListenerSubclass>
template <typename TListener>
class EventListener {
 public:
  typedef EventDispatch<TListener> Dispatch;

  EventListener() {}

  // Disallow copy and assign.
  EventListener(const EventListener&) = delete;
  EventListener& operator=(const EventListener&) = delete;

  virtual ~EventListener() { UnregisterFromAll(); }

  void RegisterOnDispatch(const std::shared_ptr<Dispatch>& dispatch) {
    if (IsRegistered(dispatch)) {
      // already registered, do nothing
      return;
    }

    // Ideally we'd check that EventListener<TListener> is a base class
    // of TListener in a static assert, however TListener is incomplete.
    // Hence the runtime check
    auto t = dynamic_cast<TListener*>(this);
    if (t) {
      auto token = dispatch->RegisterListener(t);
      dispatch_to_token_[dispatch] = token;
    }
  }

  bool IsRegistered(const std::shared_ptr<Dispatch>& dispatch) const {
    return (dispatch_to_token_.count(dispatch) > 0);
  }

  void UnregisterFromAll() {
    auto dtt_copy = dispatch_to_token_;
    for (const auto& ai : dtt_copy) {
      Unregister(ai.first);
    }
  }

  void Unregister(std::shared_ptr<Dispatch> dispatch) {
    auto ai = dispatch_to_token_.find(dispatch);
    if (ai != dispatch_to_token_.end()) {
      ai->first->UnregisterListener(ai->second);
      dispatch_to_token_.erase(ai);
    }
  }

 private:
  absl::flat_hash_map<std::shared_ptr<Dispatch>, typename Dispatch::Token>
      dispatch_to_token_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_EVENT_DISPATCH_H_
