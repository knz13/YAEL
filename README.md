# Yet Another Event Library (YAEL)

Yael is a header-only c++17 event library with the focus on simplicity and ease of use.

## Usage

All you have to do is include the headers and add a event_launcher and a event_sink to your class

```
#include "include/yael.h"

class MyClass {
public:
    yael::event_sink<void()> onMyEvent() {
      return {m_MyEvent};
    };
    
    yael::event_sink<int(bool)> onMyOtherEvent() {
      return {m_MyOtherEvent};
    };
    
    void someFunction() {
      //it's very easy to emit events...
      
      //void event
      m_MyEvent.EmitEvent();
      
      //non void event -> obtain the result as a vector!
      std::vector<int> results = m_MyOtherEvent.EmitEvent(false);

      //do something with the results...
    }

private:
    //you can have the event be any type you wish and return any other type!
    //(Ps. The return type of the emitted event is a vector of results...)
    yael::event_launcher<void()> m_MyEvent;
    
    yael::event_launcher<int(bool)> m_MyOtherEvent;
    
};
```

And voilà, your event has been created!

```
  .
  .
  .

  MyClass myClassInstance;

  // passing a lambda...
  myClassInstance.onMyEvent().Connect([](){
    "do something..."
  });
  
  // or a free function/member function!
  myClassInstance.onMyEvent().Connect(&SomeFreeFunctionReturningVoid);
  
  // support for capture lambdas!
  int someValue = 10;
  myClassInstance.onMyOtherEvent().Connect([=](bool value){
    // do something here!
    
    return someValue;
  });
  
  
  // getting the id of the connection so you can sever it later if need be...
  
  size_t id = myClassInstance.onMyEvent().Connect([](){});
  
  
  // disconnecting...
  
  myClassInstance.onMyEvent().Disconnect(id);
  
  // you can also poll for how many receivers you have;
  
  size_t receiversCount = myClassInstance.onMyEvent().NumberOfReceivers(); 
```
Yael also provides an event_receiver class, so you can simplify the disconnections! (all disconnections are automatic when the receiver or the launcher dies...)

```
class MyReceiver : public yael::event_receiver {

// put your methods here!

};
```

It's that simple! Lets take a look at how it can be used...

```
  .
  .
  .
  
  // somewhere in your codebase...
  
  MyReceiver receiver;
  MyClass myClassInstance;
  
  //just add the address of the receiver to the Connect method and to the start of your callback...
  myClassInstance.onMyEvent().Connect(&receiver,[](MyReceiver* rec){
      //do something here!
  });
  
  //when either the receiver or launcher gets destroyed, the connection will be severed automatically!
  

```



