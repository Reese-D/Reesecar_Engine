#pragma once
#include <stdint.h>
//example usage
//auto test = new position();
//auto compTest = Component(test);

//Note that component will clean up test on destruction
class Component
{
private:
    //We need this BS as a wrapper around our data, to be used in the Typed BS
    struct bullshit
    {
    public:
        virtual ~bullshit(){};
        bullshit() = delete;
        bullshit(void* data)
            :data_(data)
        {
                
        }
        void* data_;
    };

    //this allows us to clean up properly, since TBullshit actually knows the type, it can call its destructor
    template<class T> struct TBullshit : public bullshit
    {
        TBullshit<T>(T* data)
            :bullshit((void*) data)
        {
        }

        virtual ~TBullshit() override
        {
            //TADA, we can actually free it here
            ((T*)data_)->~T();
        }
    };
    
    //...But we don't reference TBullshit, we only reference bullshit, so Component doesn't have to be typed
    bullshit* data_;
public:
    
    //our constructor however is typed, so we can use the TBullshit constructor which knows how to clean up
    //properly
    template<class T> Component(T* value)
        :data_(new TBullshit<T>(value))
    {
    };

    Component(Component& cpy) = delete;
    
    Component(Component&& moving)
        :data_(nullptr)
    {
        data_ = moving.data_;
        moving.data_ = nullptr;
    };

    void* getData() const
    {
        return data_->data_;   
    }

    ~Component()
    {
        delete data_;
    };
};
