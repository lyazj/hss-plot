#pragma once
#include "interface.h"

// Event selection executor.
class EventCutter : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override final { return event_->cut(); }
};

// Event visualization executor.
class EventPlotter : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override final { return event_->plot(); }
};

// Event persistance executor.
class EventSaver : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override final { return event_->save(); }
};

// Event analysis executor.
class EventAnalyzer : public IFlow {
public:
  EventAnalyzer(IEvent *event) : IFlow(event) {
    IFlow *flow = this;
    flow = new EventCutter(flow);
    flow = new EventPlotter(flow);
    flow = new EventSaver(flow);
  }
  virtual bool execute_current_node() const override final { return true; }  // dummy head
};
