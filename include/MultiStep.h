#pragma once
#include "EventViewer.h"

// Event viewers that can have subsequent procedures.
class MultiStep : virtual public EventViewer {
public:
  MultiStep(EventViewer *then = nullptr) : then_(then) { }
  ~MultiStep() { delete then_; }

  // Pass down data and control flow.
  void proceed() override { if(then_ && then_->process()) then_->proceed(); }

  // Modify descendants.
  virtual void set_then(EventViewer *then) { delete then_; then_ = then; }
  EventViewer *then(EventViewer *viewer) { set_then(viewer); return viewer; }
  MultiStep *then(MultiStep *viewer) { set_then(viewer); return viewer; }

protected:
  EventViewer *then_;  // owned by *this
};
