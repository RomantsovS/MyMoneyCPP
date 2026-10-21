#ifndef MYMONEY_UI_SINGLETON_INSTANCE_H_
#define MYMONEY_UI_SINGLETON_INSTANCE_H_

#include <QJSEngine>
#include <QQmlEngine>

#include "core/check.h"

namespace mymoney::ui {

// D12, option B (ADR-0016): main.cpp creates the C++ objects QML uses and
// registers each one with a Registration. A QML_SINGLETON class's static
// create() - called by the QML engine the first time QML uses the type -
// returns that object through ForEngine(). QML never creates or deletes these
// objects.
template <typename T>
class SingletonInstance {
 public:
  // Makes `instance` the singleton while the Registration lives; it must be
  // created before the engine loads any QML and destroyed after the engine.
  // Not owned: `instance` must outlive the Registration.
  class Registration {
   public:
    explicit Registration(T* instance) {
      core::Check(instance_ == nullptr, "a QML singleton is registered twice");
      instance_ = instance;
    }
    Registration(const Registration&) = delete;
    Registration& operator=(const Registration&) = delete;
    Registration(Registration&&) = delete;
    Registration& operator=(Registration&&) = delete;
    // No dangling pointer is left behind.
    ~Registration() { instance_ = nullptr; }
  };

  static T* ForEngine(const QQmlEngine* engine) {
    core::Check(instance_ != nullptr,
                "a QML singleton was used before main registered it");
    core::Check(engine->thread() == instance_->thread(),
                "a QML singleton must live in the engine's thread");
    // Without this the engine would take ownership and delete it.
    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
    return instance_;
  }

 private:
  static inline T* instance_ = nullptr;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_SINGLETON_INSTANCE_H_
