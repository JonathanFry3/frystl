// Test driver for static_vector

#define FRYSTL_DEBUG
#include "static_vector.hpp"
#include "SelfCount.hpp"
#include <iostream>
#include <vector>
#include <list>

using namespace frystl;

// Test fill insert.
// Assumes vec is a static_vector of type SelfCount
// such that vec[i]() == i for all vec[i].
template <class C>
static void TestFillInsert(C vec, unsigned iat, unsigned n)
{
    unsigned count0 = SelfCount::Count();
    unsigned ownerCount0 = SelfCount::OwnerCount();
    unsigned size = vec.size();
    auto spot = vec.cbegin() + iat;
    vec.insert(spot,n,SelfCount(843));
    assert(vec.size() == size+n);
    assert(SelfCount::Count() == count0+n);
    assert(SelfCount::OwnerCount() == ownerCount0+n);
    assert(vec[iat-1]() == iat-1);
    assert(vec[iat]() == 843);
    assert(vec[iat+n-1]() == 843);
    if (iat < size) {
        assert(vec[iat+n]() == iat);
        assert(vec[size+n-1]()== size-1);
    }
}

int main() {

    // Constructors.
    {
        // fill
        {
            static_vector<int,20> i20(17);
            assert(i20.size() == 17);
            for (int k:i20) assert(k==0);

            static_vector<int,23> i23(17, -6);
            assert(i23.size() == 17);
            for (int k:i23) assert(k==-6);

            struct NoCopy {
                int _a;
                NoCopy() = default;
                NoCopy(const NoCopy& b) = delete;
            };
            static_vector<NoCopy, 10> nc(7);
            assert (nc.size() == 7);
        }

        // range
        assert(SelfCount::Count() == 0);
        assert(SelfCount::OwnerCount() == 0);
        std::list<int> li;
        for (int i = 0; i < 30; ++i) li.push_back(i-13);
        static_vector<SelfCount,95> sv(li.begin(),li.end());
        assert(SelfCount::Count() == 30);
        assert(SelfCount::OwnerCount() == 30);
        assert(sv.size() == 30);
        for (int i = 0; i < 30; ++i) assert(sv[i]() == i-13);

        {
            // copy
            assert(SelfCount::Count() == 30);
            assert(SelfCount::OwnerCount() == 30);
            static_vector<SelfCount,80> i80 (sv);
            assert(i80.size() == 30);
            assert(SelfCount::Count() == 60);
            assert(SelfCount::OwnerCount() == 60);
            for (int i = 0; i < 30; ++i) assert(i80[i]() == i-13);

            static_vector<SelfCount,80> j80 (i80);
            assert(j80.size() == 30);
            assert(SelfCount::Count() == 90);
            assert(SelfCount::OwnerCount() == 90);
            for (int i = 0; i < 30; ++i) assert(j80[i]() == i-13);        
        }
        {
            // move
            assert(SelfCount::Count() == 30);
            assert(SelfCount::OwnerCount() == 30);
            static_vector<SelfCount,73> i73 (std::move(sv));
            assert(sv.size() == 0);
            assert(i73.size() == 30);
            assert(SelfCount::Count() == 30);
            assert(SelfCount::OwnerCount() == 30);
            for (int i = 0; i < 30; ++i) assert(i73[i]() == i-13);
            sv = std::move(i73);

            assert(SelfCount::Count() == 30);
            assert(SelfCount::OwnerCount() == 30);
        }
        {
            // initializer list constructor
            unsigned oc = SelfCount::OwnerCount();
            unsigned c = SelfCount::Count();
            static_vector<SelfCount, 10> i10 {28, -373, 42, 10000000, -1};
            assert(SelfCount::Count() == c+5);
            assert(SelfCount::OwnerCount() == oc+5);
            assert(i10[2] == 42);
            assert(i10.size() == 5);
        }            
    }
    {
        // Default Constructor, empty()
        static_vector<SelfCount,50> di50;
        assert(SelfCount::Count() == 0);
        assert(SelfCount::OwnerCount() == 0);
        assert(di50.size() == 0);
        assert(di50.capacity() == 50);
        assert(di50.empty());


        // emplace_back(), size()
        for (unsigned i = 0; i < 50; i+= 1){
            assert(di50.emplace_back(i)() == i);
            assert(di50.size() == i+1);
            assert(SelfCount::OwnerCount() == di50.size());
            assert(SelfCount::Count() == di50.size());
        }

        const auto & cdi50{di50};

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di50.pop_back();
            assert(SelfCount::OwnerCount() == di50.size());
            assert(SelfCount::Count() == di50.size());
        }
        assert(di50.size() == 30);

        // at()
        assert(di50.at(9)() == 9);
        assert(cdi50.at(29)() == 29);
        try {
            int k = di50.at(30)();  // should throw std::out_of_range
            assert(false);
        } 
        catch (std::out_of_range&) {}
        catch (...) {assert(false);}

        // operator[](), back(), front()
        assert(di50[7]() == 7);
        di50[7] = SelfCount(91);
        assert(di50[7]() == 91);
        assert(SelfCount::Count() == di50.size());
        assert(SelfCount::OwnerCount() == di50.size());
        di50[7] = SelfCount(7);
        assert(cdi50[23]() == 23);
        assert(di50.back()() == 29);
        di50.back() = SelfCount(92);
        assert(di50.back()() == 92);
        di50.back() = SelfCount(29);
        assert(di50.back()() == 29);
        assert(di50.front()() == 0);
        assert(cdi50.front()() == 0);
        assert(SelfCount::Count() == di50.size());
        assert(SelfCount::OwnerCount() == di50.size());

        // push_back()
        di50.push_back(SelfCount(30));
        assert(cdi50[30]() == 30);
        assert(di50.size() == 31);
        assert(SelfCount::OwnerCount() == 31);
        assert(SelfCount::Count() == di50.size());


        // data()
        const SelfCount& s {*(cdi50.data()+8)};
        assert(s() == 8);

        // begin(), end()
        assert(&(*(di50.begin()+6)) == cdi50.data()+6);
        assert((*(cdi50.begin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.begin()+di50.size() == di50.end());
        assert(SelfCount::OwnerCount() == di50.size());

        // cbegin(), cend()
        assert(&(*(di50.cbegin()+6)) == cdi50.data()+6);
        assert((*(di50.cbegin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.cbegin()+di50.size() == di50.cend());
        assert(SelfCount::OwnerCount() == di50.size());

        // rbegin(), rend(), crbegin(), crend()
        assert(&(*(di50.crbegin()+6)) == cdi50.end()-7);
        assert((*(di50.crbegin()))() == 30);
        *(di50.rbegin()+8) = SelfCount(71);
        assert(cdi50[22]() == 71);
        *(di50.rbegin()+8) = SelfCount(22);
        assert(di50.crbegin()+di50.size() == di50.crend());
        assert(SelfCount::OwnerCount() == di50.size());
        for (int i = 0; i < 31; i++) assert(cdi50[i]() == i);

        // erase()
        assert(di50.size() == 31);
        assert(di50.erase(di50.begin()+8) == di50.begin()+8);

        assert(SelfCount::OwnerCount() == di50.size());
        assert(SelfCount::Count() == di50.size());
        assert(di50.size() == 30);
        assert(di50[7]() == 7);
        assert(di50[8]() == 9);
        assert(di50[29]() == 30);

        // emplace()
        assert((*di50.emplace(di50.begin()+8,96))() == 96);
        assert(di50[9]() == 9);
        assert(di50.size() == 31);
        assert(SelfCount::Count() == di50.size());
        assert(SelfCount::OwnerCount() == di50.size());

        // range erase()
        auto spot = di50.erase(di50.begin()+8, di50.begin()+12);
        assert(spot == di50.begin()+8);
        assert(*spot == 12);
        assert(*(spot-1) == 7);
        assert(di50.size() == 27);
        assert(SelfCount::OwnerCount() == di50.size());
        assert(SelfCount::Count() == di50.size());

        auto td = di50.erase(di50.end()-7, di50.end());
        assert(td == di50.end());
        assert(di50.size() == 20);
        assert(di50.back() == 23);
        assert(SelfCount::OwnerCount() == di50.size());
        assert(SelfCount::Count() == di50.size());

        // clear()
        di50.clear();
        assert(di50.size() == 0);
        assert(SelfCount::Count() == di50.size());
        assert(SelfCount::OwnerCount() == di50.size());
    }{
        // Operation on iterators
        static_vector<int, 10> vec({0,1,2,3,4,5,6,7});
        using It = static_vector<int,5>::const_iterator;
        It i1 = vec.cbegin()+3;
        assert(*i1 == 3);
        assert(*(i1-2) == 1);
        i1 += 1;
        assert(*i1 == 4);
        i1 -= 3;
        assert(*i1 == 1);
        assert(i1[3] == 4);

        It i2(vec.end());
        assert(i2-i1 == 7);
        assert(i2>i1);
    }{
        // assign()
        // fill type
        static_vector<int,6> dv;
        dv.assign(6,-29);
        assert(dv.size() == 6);
        for (auto i: dv) assert(i==-29);
    }{
        // range type
        static_vector<int,10> dv;
        std::list<unsigned> lst;
        for (unsigned i = 9; i < 9+8; ++i){
            lst.push_back(i);
        }
        dv.assign(lst.begin(),lst.end());
        assert(dv.size() == 8);
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv[i-9]==i);}

        static_vector<int,9> dv2;
        dv2.push_back(78);
        dv2.assign(dv.begin(),dv.end());
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv2[i-9]==i);}

        // initializer list type
        dv.assign({-3, 27, 12, -397});
        assert(dv.size() == 4);
        assert(dv[2] == 12);
    }{
        // assignment operators
        static_vector<SelfCount, 50> a, b;
        assert(SelfCount::OwnerCount() == 0);
        for (unsigned i = 0; i<20; ++i)
            a.emplace_back(i);
        assert(SelfCount::OwnerCount() == 20);
        assert(SelfCount::Count() == 20);

        // copy operator=()
        b = a;
        assert(a==b);
        assert(b.size() == 20);
        assert(SelfCount::OwnerCount() == 40);

        a = a;
        assert(a.size() == 20);
        assert(a == b);
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);

        // move operator=()
        b = std::move(a);
        assert(b.size() == 20);
        assert(SelfCount::OwnerCount() == 20);
        assert(SelfCount::Count() == 20);
        assert(a != b);

        a = b;
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);

        b = std::move(b);
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);
        assert(b.size() == 20);
        assert(a == b);

        // initializer_list operator=()
        b = {14, -293, 1200, -2, 0};
        assert(b.size() == 5);
        assert(b[3]() == -2);
        assert(SelfCount::Count() == 25);
    }{
        // assignment operators between vectors of different capacities
        static_vector<SelfCount, 50> a;
        static_vector<SelfCount, 70> b;
        assert(SelfCount::Count() == 0);
        assert(SelfCount::OwnerCount() == 0);
        for (unsigned i = 0; i<20; ++i)
            a.emplace_back(i);
        assert(SelfCount::Count() == 20);
        assert(SelfCount::OwnerCount() == 20);

        // copy operator=()
        b = a;
        assert(a==b);
        assert(b.size() == 20);
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);

        a = a;
        assert(a.size() == 20);
        assert(a == b);
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);

        // move operator=()
        b = std::move(a);
        assert(b.size() == 20);
        assert(SelfCount::Count() == 20);
        assert(SelfCount::OwnerCount() == 20);
        assert(a != b);

        a = b;
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);

        b = std::move(b);
        assert(SelfCount::Count() == 40);
        assert(SelfCount::OwnerCount() == 40);
        assert(b.size() == 20);
        assert(a == b);
    }{
        // The many flavors of insert()

        assert(SelfCount::OwnerCount() == 0);
        static_vector<SelfCount,99> roop;
        for (unsigned i = 0; i < 47; ++i)
            roop.emplace_back(i);
        {
            // Move insert()
            assert(SelfCount::OwnerCount() == 47);
            auto spot = roop.cbegin()+9;
            SelfCount ob{71};
            roop.insert(spot,std::move(ob));
            assert(roop.size() == 48);
            assert(!ob.Owns());
            assert(SelfCount::Count() == 49);
            assert(SelfCount::OwnerCount() == 48);
            assert(roop[8]() == 8);
            assert(roop[9]() == 71);
            assert(roop[10]() == 9);
            assert(roop[47]() == 46);
            roop.erase(spot);
        }
        {
            // Test insert at the ends
            assert(roop.back()() == roop.size()-1);
            auto left = roop.front();
            auto sz = roop.size();
            roop.erase(roop.cbegin());
            assert(roop[0]() == 1);
            roop.insert(roop.cbegin(), left);
            assert(roop[0]() == 0);
            assert(roop[1]() == 1);
            assert(roop.size() == sz);

            auto right = roop.back();
            roop.erase(roop.end()-1);
            assert(roop.size() == sz-1);
            assert(roop.back()() == roop.size()-1);
            roop.insert(roop.end(), right);
            assert(roop.size() == sz);
            assert(roop.back() == right);
            assert((*(roop.end()-2))() == sz-2);
        }

        // Fill insert()
        assert(roop.size() == 47);
        assert(SelfCount::OwnerCount() == 47);
        TestFillInsert(roop,19,13);
        TestFillInsert(roop,43,13);
        TestFillInsert(roop,roop.size(),13);

        {
            // Range insert() from a range of input iterators
            std::list<int> intList;
            for (int i = 0; i < 9; ++i) {
                intList.push_back(i+173);
            }
            static_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::OwnerCount() == 47*2);
            assert(SelfCount::Count() == 47*2);
            r2.insert(r2.cbegin()+31, intList.cbegin(), intList.cend());
            assert(r2.size() == 47+9);
            assert(SelfCount::Count() == 2*47+9);
            assert(SelfCount::OwnerCount() == 2*47+9);
            assert(r2[30]() == 30);
            assert(r2[31+4]() == 4+173);
            assert(r2[31+9]() == 31);
        }
        assert(SelfCount::OwnerCount() == 47);
        {
            // Range insert() from a range of random access iterators
            static_vector<int,71> intList;
            for (int i = 0; i < 9; ++i) {
                intList.push_back(i+173);
            }
            static_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::Count() == 47*2);
            assert(SelfCount::OwnerCount() == 47*2);
            r2.insert(r2.cbegin()+31, intList.cbegin(), intList.cend());
            assert(r2.size() == 47+9);
            assert(SelfCount::OwnerCount() == 2*47+9);
            assert(r2[30]() == 30);
            assert(r2[31+4]() == 4+173);
            assert(r2[31+9]() == 31);
        }
        assert(SelfCount::OwnerCount() == 47);
        {
            // Initializer list insert()
            static_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::OwnerCount() == 47*2);
            using Z = SelfCount;
            r2.insert(r2.cbegin()+31, {Z(-72),Z(0),Z(274),Z(-34245)});
            assert(r2.size() == 47+4);
            assert(SelfCount::OwnerCount() == 2*47+4);
            assert(r2[30]() == 30);
            assert(r2[30+3]() == 274);
            assert(r2[31+4]() == 31);
        }
        assert(SelfCount::OwnerCount() == 47);
    }

    {
        // resize()
        assert(SelfCount::OwnerCount() == 0);
        static_vector<SelfCount, 99> v99;
        for (int i = 0; i < 73; ++i)
            v99.emplace_back(i);
        assert(v99.size() == 73);
        assert(SelfCount::OwnerCount() == 73);
        v99.resize(78,SelfCount(-823));
        assert(v99.size() == 78);
        assert(SelfCount::OwnerCount() == 78);
        assert(v99[72]() == 72);
        assert(v99[73]() == -823);
        assert(v99[77]() == -823);
        v99.resize(49);
        assert(v99.size() == 49);
        assert(SelfCount::OwnerCount() == 49);
        assert(v99[48]() == 48);
        v99.resize(56);
        assert(v99.size() == 56);
        assert(SelfCount::OwnerCount() == 56);
        assert(v99[55]() == 0);
    }{
        // swap() member
        assert(SelfCount::OwnerCount() == 0);
        static_vector<SelfCount, 99> va, vc;
        static_vector<SelfCount, 99> vb, vd;
        for (int i = 0; i < 57; ++i){
            va.emplace_back(i);
            if (i < 19) vb.emplace_back(i+300);
        }
        vc = va;
        vd = vb;
        assert(va.size() == 57);
        assert(vb.size() == 19);
        assert(SelfCount::OwnerCount() == 2*(19+57));
        assert(vc == va);
        assert(vd == vb);
        va.swap(vb);
        assert(vb.size() == 57);
        assert(va.size() == 19);
        assert(SelfCount::OwnerCount() == 2*(19+57));
        assert(vd == va);
        assert(vc == vb);
    }{
        // swap() non-member overload
        assert(SelfCount::OwnerCount() == 0);
        static_vector<SelfCount, 99> va, vb, vc, vd;
        for (int i = 0; i < 57; ++i){
            va.emplace_back(i);
            if (i < 19) vb.emplace_back(i+300);
        }
        vc = va;
        vd = vb;
        assert(va.size() == 57);
        assert(vb.size() == 19);
        assert(SelfCount::OwnerCount() == 2*(19+57));
        assert(vc == va);
        assert(vd == vb);
        swap(va,vb);
        assert(vb.size() == 57);
        assert(va.size() == 19);
        assert(SelfCount::OwnerCount() == 2*(19+57));
        assert(vd == va);
        assert(vc == vb);
    }{
        // comparison functions
        static_vector<int,73> v0;
        static_vector<int,70> v1;
        for (unsigned i = 0; i < 40; ++i){
            v0.push_back(i);
            v1.push_back(i);
        }

        assert(v0 == v1);
        assert(v0 == v0);
        assert(v1 == v0);
        assert(!(v0 < v1));

        v1.pop_back();
        assert(v1 < v0);
        assert(v1 <= v0);
        assert(v0 > v1);
        assert(v0 >= v1);
        assert(v1 != v0);

        v1[16] = 235;
        assert(v0 < v1);
        assert(v0 != v1);
    }
    std::cout << "test-sv finished normally." << std::endl;
}