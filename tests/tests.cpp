#include "../include/yael.h"
#include "catch2/catch_test_macros.hpp"


struct TestLauncher {

public:
    yael::event_sink<void()> onVoidEvent() {
        return {m_VoidEvent};
    }
    yael::event_sink<void(int)> onVoidIntEvent(){
        return {m_VoidIntEvent};
    };

    yael::event_sink<int(std::string)> onNonVoidEvent() {
        return {m_NonVoidEvent};
    }


    yael::event_launcher<void()> m_VoidEvent;
    yael::event_launcher<void(int)> m_VoidIntEvent;
    yael::event_launcher<int(std::string)> m_NonVoidEvent;


};

struct TestReceiver : public yael::event_receiver {

};

struct TestReceiverDerived : public TestReceiver {

};
 

TEST_CASE("Event Testing With Receiver"){
    TestLauncher launcher;
    TestReceiver receiver;

    SECTION("Subscribing test"){
        launcher.onVoidEvent().Connect(&receiver,[&](yael::event_receiver* eventReceiver){
            REQUIRE(&receiver == eventReceiver);
        });

        launcher.onVoidIntEvent().Connect(&receiver,[&](yael::event_receiver* eventReceiver,int value){
            REQUIRE(&receiver == eventReceiver);
            REQUIRE(value == 1);
        });
        
        launcher.onNonVoidEvent().Connect(&receiver,[&](yael::event_receiver* eventReceiver,std::string strValue){
            REQUIRE(strValue == "my string");
            REQUIRE(&receiver == eventReceiver);

            return 1;
        });

        SECTION("Launching events"){
            launcher.m_VoidEvent.EmitEvent();
            launcher.m_VoidIntEvent.EmitEvent(1);
            REQUIRE(launcher.m_NonVoidEvent.EmitEvent("my string")[0] == 1);
        }
        SECTION("Unsubscribing With One Receiver") {
            REQUIRE(launcher.onVoidEvent().Disconnect(&receiver));
            REQUIRE(launcher.onVoidIntEvent().Disconnect(&receiver));
            REQUIRE(launcher.onNonVoidEvent().Disconnect(&receiver));
        }

        SECTION("Multiple receivers") {
            TestReceiver receiverTwo;

            REQUIRE(launcher.m_NonVoidEvent.NumberOfReceivers() == 1);

            launcher.onNonVoidEvent().Connect(&receiverTwo,[](auto receiverPointer,std::string val){
                return 2;
            });

            REQUIRE(launcher.m_NonVoidEvent.NumberOfReceivers() == 2);

            std::vector<int> values = launcher.m_NonVoidEvent.EmitEvent("my string");
            
            REQUIRE(values.size() == 2);

            REQUIRE(launcher.onNonVoidEvent().Disconnect(&receiverTwo));

            REQUIRE(launcher.m_NonVoidEvent.NumberOfReceivers() == 1);
            
        }

        SECTION("Mutiple receivers with death of a receiver") {
            {
                TestReceiver receiverTwo;
                REQUIRE(launcher.m_VoidEvent.NumberOfReceivers() == 1);
                launcher.onVoidEvent().Connect(&receiverTwo, [&receiverTwo](auto receiverPointer) {
                    REQUIRE(&receiverTwo == receiverPointer);
                });
                REQUIRE(launcher.m_VoidEvent.NumberOfReceivers() == 2);
                launcher.m_VoidEvent.EmitEvent();
            }
            REQUIRE(launcher.m_VoidEvent.NumberOfReceivers() == 1);

            launcher.m_VoidEvent.EmitEvent();
        }

    }

}


TEST_CASE("Launcher Death") {
    TestReceiver receiver;

    {
        TestLauncher launcher;

        launcher.onVoidEvent().Connect(&receiver, [](auto pointer) {});

        REQUIRE(receiver.IsSubscribedTo(launcher.onVoidEvent()));
        REQUIRE(receiver.NumberOfSubscribedEvents() == 1);
    }

    REQUIRE(receiver.NumberOfSubscribedEvents() == 0);
   

}

TEST_CASE("Launcher Death With Derived Receiver") {
    TestReceiverDerived receiver;

    {
        TestLauncher launcher;

        launcher.onVoidEvent().Connect(&receiver, [](auto rec) {});

        REQUIRE(receiver.IsSubscribedTo(launcher.onVoidEvent()));
        REQUIRE(receiver.NumberOfSubscribedEvents() == 1);
    }

    REQUIRE(receiver.NumberOfSubscribedEvents() == 0);
}

TEST_CASE("Copying Receiver") {
    TestReceiver receiverOne;
    TestReceiver receiverTwo;
    TestLauncher launcher;

    launcher.onVoidEvent().Connect(&receiverOne, [](auto ptr) {});

    REQUIRE(launcher.m_VoidEvent.NumberOfReceivers() == 1);
    REQUIRE(launcher.m_VoidEvent.IsReceiverConnected(&receiverOne));
    REQUIRE(!launcher.m_VoidEvent.IsReceiverConnected(&receiverTwo));
    REQUIRE(receiverOne.IsSubscribedTo(launcher.onVoidEvent()));
    REQUIRE(!receiverTwo.IsSubscribedTo(launcher.onVoidEvent()));

    receiverTwo = receiverOne;

    REQUIRE(receiverTwo.IsSubscribedTo(launcher.onVoidEvent()));
    REQUIRE(launcher.m_VoidEvent.NumberOfReceivers() == 2);


}

TEST_CASE("Free function attachment") {
    TestLauncher launcher;

    size_t id = launcher.onVoidEvent().Connect([](){});

    REQUIRE(launcher.onVoidEvent().NumberOfReceivers() == 1);
    REQUIRE(launcher.onVoidEvent().IsReceiverConnected(id));

    REQUIRE(launcher.onVoidEvent().Disconnect(id));
    REQUIRE(launcher.onVoidEvent().NumberOfReceivers() == 0);

}

TEST_CASE("Many attachments") {
    std::vector<TestReceiver> vec;
    
    {
        TestLauncher launcher;
        

        for(int i = 0;i<70;i++){
            TestReceiver& rec = vec.emplace_back();
            launcher.onVoidEvent().Connect(&rec,[](yael::event_receiver* rec){
                REQUIRE(rec->NumberOfSubscribedEvents() == 1);
            });
        }

        REQUIRE(launcher.onVoidEvent().NumberOfReceivers() == 70);
    }

    for(auto& rec : vec){
        REQUIRE(rec.NumberOfSubscribedEvents() == 0);
    }

}