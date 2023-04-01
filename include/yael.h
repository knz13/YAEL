#pragma once
#include <functional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <typeinfo>


namespace yael {
struct event_receiver;


    template<typename T>
    struct event_launcher;
    template<typename T>
    struct event_sink;
    struct event_receiver_attachment_properties {
        std::function<bool(void*, size_t)> m_DeleteFunc;
        std::function<void(void*,event_receiver*)> m_CopyFunc;
    };


    template<typename T>
    struct event_launcher_attachment_properties;

    template<typename R,typename... Args>
    struct event_launcher_attachment_properties<R(Args...)> {
        std::function<R(Args...)> m_ExecutableFunc;
        std::function<void(void*)> m_DisconnectingFunc;
    };
    
    struct event_receiver {

        event_receiver() {
            
        };

        virtual ~event_receiver() {
            if (m_SubscribedEvents.size() == 0) {
                return;
            }
            for (auto& [ptr, prop] : m_SubscribedEvents) {
                prop.m_DeleteFunc(ptr, std::hash<void*>()((void*)this));
            }
        }

        event_receiver(const event_receiver& other) {
            for(auto& [handle,prop] : other.m_SubscribedEvents){
                prop.m_CopyFunc(handle,this);
            }
        };

        template<typename T>
        bool IsSubscribedTo(event_sink<T> sink) {
            return sink.m_Master->IsReceiverConnected(this);
        }
        
        size_t NumberOfSubscribedEvents() const {
            return m_SubscribedEvents.size();
        }


        event_receiver& operator=(const event_receiver& other) {
            for(auto& [handle,prop] : other.m_SubscribedEvents){
                prop.m_CopyFunc(handle,this);
            }
            return *this;
        }

    private:
        std::map<void*, event_receiver_attachment_properties> m_SubscribedEvents;

        template<typename T>
        friend struct event_sink;
        template<typename T>
        friend struct event_launcher;
    };

    template<typename R, typename... Args>
    struct event_sink<R(Args...)>;

    template<typename R, typename... Args>
    struct event_launcher<R(Args...)> {

        event_launcher() {
        };

        virtual ~event_launcher() {
            for (auto& [handle, prop] : m_Receivers) {
                if (prop.m_DisconnectingFunc) {
                    prop.m_DisconnectingFunc(this);
                }
            }
        }

        event_sink<R(Args...)> Sink();

        size_t NumberOfReceivers() const {
            return m_Receivers.size();
        }

        bool IsReceiverConnected(event_receiver* rec) const {
            size_t hash = std::hash<void*>()((void*)rec);
            return m_Receivers.find(hash) != m_Receivers.end();
        }

        bool IsReceiverConnected(size_t hash) const {
            return m_Receivers.find(hash) != m_Receivers.end();
        }

        bool DisconnectReceiver(size_t hash) {
            if (m_Receivers.find(hash) != m_Receivers.end()) {
                m_Receivers.erase(hash);
                return true;
            }
            return false;
        };



        void Clear() {
            m_Receivers.clear();
        }


        auto EmitEvent(Args... args) {
            if constexpr (std::is_same<R,void>::value) {
                for (auto& [handle, properties] : m_Receivers) {
                    if (properties.m_ExecutableFunc) {
                        properties.m_ExecutableFunc(args...);
                    }
                }
            }
            else {
                std::vector<R> vec;
                for (auto& [handle, properties] : m_Receivers) {
                    if (properties.m_ExecutableFunc) {

                        vec.push_back(properties.m_ExecutableFunc(args...));
                    }
                }
                return vec;
            }
        };

        event_launcher<R(Args...)>& operator=(const event_launcher<R(Args...)>& other) {
            this->m_Receivers = other.m_Receivers;
        }

    private:


        std::unordered_map<size_t, event_launcher_attachment_properties<R(Args...)>> m_Receivers;
        size_t m_MyHash = std::hash<void*>()((void*)this);

        template<typename T>
        friend struct event_sink;

    };


    template<typename R, typename... Args>
    struct event_sink<R(Args...)> {
        event_sink(event_launcher<R(Args...)>& sink) : m_Master(&sink) {};



        size_t Connect(std::function<R(Args...)> windowFunc) {
            static int count = 1;
            size_t hash = std::hash<int>()(count);
            count++;

            event_launcher_attachment_properties<R(Args...)> prop;

            prop.m_ExecutableFunc = windowFunc;
            
            m_Master->m_Receivers[hash] = std::move(prop);
            return hash;

        }

        void Connect(event_receiver* key, std::function<R(event_receiver*, Args...)> windowFunc) {

            size_t hash = std::hash<void*>()((void*)key);
            
            auto func = ([=](auto&&... args) {
                return windowFunc(key, std::forward<decltype(args)>(args)...);
            });

            event_launcher_attachment_properties<R(Args...)> masterProperties;

            masterProperties.m_ExecutableFunc = func;
            masterProperties.m_DisconnectingFunc = [=](void* master) {
                key->m_SubscribedEvents.erase(master);
            };

            m_Master->m_Receivers[hash] = std::move(masterProperties);

            event_receiver_attachment_properties prop;

            

            prop.m_CopyFunc = [=](void* master,event_receiver* ptr){
                event_sink<R(Args...)>(*(event_launcher<R(Args...)>*)master).Connect(ptr,windowFunc);
            };

            prop.m_DeleteFunc = [](void* ptr, size_t hash) {
                return ((event_launcher<R(Args...)>*)(ptr))->DisconnectReceiver(hash);
            };

            key->m_SubscribedEvents[(void*)m_Master] = std::move(prop);

        };

        bool Disconnect(size_t hashKey) {
            return m_Master->DisconnectReceiver(hashKey);
        }

        bool IsReceiverConnected(event_receiver* rec) const {
            return m_Master->IsReceiverConnected(rec);
        }

        bool IsReceiverConnected(size_t hash) const {
            return m_Master->IsReceiverConnected(hash);
        }

        size_t NumberOfReceivers() const {
            return m_Master->NumberOfReceivers();
        }

        bool Disconnect(event_receiver* key) {
            if (key == nullptr) {
                return false;
            }
            size_t hash = std::hash<void*>()((void*)key);
            if (m_Master->m_Receivers.find(hash) != m_Master->m_Receivers.end()) {
                m_Master->DisconnectReceiver(hash);
            }
            else {
                return false;
            }
            return true;
        }

    private:



        event_launcher<R(Args...)>* m_Master;


        friend class event_receiver;


    };

    template<typename R, typename... Args>
    event_sink<R(Args...)> event_launcher<R(Args...)>::Sink() {
        return event_sink<R(Args...)>(*this);
    }


};