#pragma once

//example usage
//auto test = new position();
//auto compTest = Component(test);

//Note that component will clean up test on destruction
class Component
{
private:
    const std::type_info& type;
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
            std::cout << "deleting TBullshit" << std::endl;
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
        :type(typeid(T))
        ,data_(new TBullshit<T>(value))
    {
    };

    ~Component()
    {
        std::cout << "deleting Component" << std::endl;
        delete data_;
    };
};
