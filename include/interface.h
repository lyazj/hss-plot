#pragma once

class IEvent;
class IFlow;

// Abstract event adapter.
// Subclass it to add data members.
class IEvent {
public:
  virtual ~IEvent() = default;
  virtual bool next() { return false; }
  virtual bool cut() { return true; }
  virtual bool process() const { return true; }
};

// Abstract linear control flow.
// Shortcut logical and evaluation.
class IFlow {
public:
  IFlow(IEvent *event, IFlow *next = nullptr) : event_(event), next_(next) { }
  IFlow(IFlow *prev) : IFlow(prev->event_) { delete prev->next_; prev->next_ = this; }
  virtual ~IFlow() { delete next_; }

  virtual bool execute_current_node() const = 0;
  bool execute() const { return execute_current_node() && (!next_ || next_->execute()); }
  void loop() const { while(event_->next()) execute(); }

protected:
  IEvent *event_;  // not owned
  IFlow *next_;  // owned
};
