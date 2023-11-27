#pragma once

class EventViewer;

// Abstract sliding event viewer.
class EventViewer {
public:
  virtual ~EventViewer() = default;

  // Move to the next event.
  // The viewer is initialized at the position before the first event.
  // The first successful call to next() switches to the first event.
  // Returns: true if event switching succeeds, false otherwise.
  virtual bool next() { return false; };

  // Determine whether to proceed on current event.
  // Returns: true to proceed, false otherwise.
  virtual bool cut() { return true; }

  // Process current event.
  virtual void process() { }

  // Process current and subsequent events.
  virtual void loop() { while(next()) cut() && (process(), true); }
};
