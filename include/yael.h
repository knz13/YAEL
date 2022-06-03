#pragma once
#include <functional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <typeinfo>


namespace yael {
struct event_receiver;
    struct event_receiver_attachment_properties {
        std::function<void(void*, size_t)> m_DeleteFunc;
    };


    template<typename T>
    struct event_launcher;
    template<typename T>
    struct event_sink;

    struct event_receiver {

        virtual ~event_receiver() {
            if (m_SubscribedEvents.size() == 0) {
                return;
            }
            auto it = m_SubscribedEvents.begin();
            while (it != m_SubscribedEvents.end()) {
                it->second.m_DeleteFunc(it->first, std::hash<void*>()((void*)this));
                m_SubscribedEvents.erase(it);
                it = m_SubscribedEvents.begin();
            }
        }

        template<typename T>
        bool IsSubscribedTo(event_sink<T> sink) {
            return sink.m_Master->IsReceiverConnected(this);
        }
        
        size_t NumberOfSubscribedEvents() const {
            return m_SubscribedEvents.size();
        }


        event_receiver& operator=(const event_receiver& other) {
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
    struct event_launcher<R(Args...)> {

        event_launcher() {
        };

        virtual ~event_launcher() {
        }

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
                for (auto& [handle, func] : m_Receivers) {
                    if (func) {
                        (*func.get())(args...);
                    }
                }
            }
            else {
                std::vector<R> vec;
                for (auto& [handle, func] : m_Receivers) {
                    if (func) {
                        vec.push_back((*func.get())(args...));
                    }
                }
                return vec;
            }
        };

        event_launcher<R(Args...)>& operator=(const event_launcher<R(Args...)>& other) {
            return *this;
        }

    private:
        std::unordered_map<size_t, std::shared_ptr<std::function<R(Args...)>>> m_Receivers;
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

            std::function<R(Args...)>* func = new std::function<R(Args...)>(windowFunc);
            m_Master->m_Receivers[hash] = std::make_shared<std::function<R(Args...)>>(*func);
            return hash;

        }

        void Connect(event_receiver* key, std::function<R(event_receiver*, Args...)> windowFunc) {

            size_t hash = std::hash<void*>()((void*)key);
            auto deleter = [=](std::function<R(Args...)>* func) {
                key->m_SubscribedEvents.erase((void*)m_Master);
                delete func;
            };

            auto func = new std::function<R(Args...)>([=](auto... args) {
                return windowFunc(key, args...);
            });
            m_Master->m_Receivers[hash] = std::shared_ptr<std::function<R(Args...)>>(func, deleter);

            event_receiver_attachment_properties prop;
           

            prop.m_DeleteFunc = [](void* ptr, size_t hash) {
                ((event_launcher<R(Args...)>*)(ptr))->DisconnectReceiver(hash);
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

};