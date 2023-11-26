#pragma once

// Inner abstract classes.
class IEvent;
class IFlow;
class ILoader;
class IPlotter;

// Abstract event adapter.
// Subclass it to add data members.
class IEvent {
public:
  virtual ~IEvent() = default;
  virtual bool next() { return false; }
  virtual bool plot() { return false; }
};

// Abstract linear control flow.
// Shortcut logical and evaluation.
class IFlow {
public:
  IFlow(IEvent *event, IFlow *next = nullptr) : event_(event), next_(next) { }
  virtual ~IFlow() = default;

  virtual bool execute_current_node() const = 0;
  bool execute() const { return execute_current_node() && (!next_ || next_->execute()); }

protected:
  IEvent *event_;
  IFlow *next_;
};

// Event loading executor.
class ILoader : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override { return event_->next(); }
};

// Event visualization executor.
class IPlotter : public IFlow {
public:
  using IFlow::IFlow;
  virtual bool execute_current_node() const override { return event_->plot(); }
};
