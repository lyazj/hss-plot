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

  // Process current event.
  // Returns: true to pass current event to subsequent viewers, false otherwise.
  virtual bool process() { return true; }

  // Pass current event to subsequent viewers.
  virtual void proceed() { }

  // Process current and subsequent events.
  virtual void loop() { while(next()) if(process()) proceed(); }
};
