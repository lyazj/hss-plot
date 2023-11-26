#pragma once
#include "interface.h"

// Event selection executor.
class EventCutter : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override final { return event_->cut(); }
};

// Event visualization executor.
class EventProcessor : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override final { return event_->process(); }
};
