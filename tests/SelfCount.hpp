#ifndef SELF_COUNT_HPP
#define SELF_COUNT_HPP

#include <cstdint>      // int32_t
#include <cassert>      // assert()

// A class which simulates owning a resource (or not) and
// counts both the instances that own the resource and
// the total number of undestructed instances.

struct SelfCount {
    SelfCount()
        : _member(0), _owns(true)
        {
            IncrOwnerCount(1);
            IncrCount(1);
        }
    SelfCount(int val)
        : _member(val), _owns(true)
        {
            IncrOwnerCount(1);
            IncrCount(1);
        }
    SelfCount(const SelfCount& val)
        : _member(val._member), _owns(true)
        {
            IncrOwnerCount(val._owns);
            IncrCount(1);
        }
    SelfCount(SelfCount&& val)
        : _member(val._member)
        , _owns(val._owns)
        {
            IncrOwnerCount(val._owns);
            IncrCount(1);
            val.Disown();
        }
    SelfCount & operator=(SelfCount&& right)
    {
        if (this != &right) {
            Disown();
            _member = right._member;
            _owns = right._owns;
            right._owns = false;
        }
        return *this;
    }
    SelfCount & operator=(const SelfCount& right) = delete;

    bool operator==(const SelfCount& right) const 
    {
        return _member == right._member && _owns == right._owns;
    }
    bool operator!=(const SelfCount& right) const 
    {
        return ! operator==(right);
    }
    uint32_t operator()() const noexcept {return _member;}
    bool Owns() const noexcept {return _owns;}
    
    ~SelfCount() {
        IncrCount(-1);
        Disown();
    }
    static int Count()
    {
        return count;
    }
    static int OwnerCount()
    {
        return ownerCount;
    }
private:
    int32_t _member;
    bool _owns;
    
    static int ownerCount;
    void IncrOwnerCount(int i) 
    {
        ownerCount += i;
    }
    
    static int count;
    void IncrCount(int i) 
    {
        count += i;
    }
    void Disown()
    {
        IncrOwnerCount(-short(_owns)); 
        _owns = false;
        assert(ownerCount >= 0);
    }
};

int SelfCount::count = 0;
int SelfCount::ownerCount = 0;
#endif // ndef SELF_COUNT_HPP