// Test driver for mf_vector

#define FRYSTL_DEBUG
#include <vector>
#include <list>
#include "mf_vector.hpp"
#include "SelfCount.hpp"

using namespace frystl;

// Test fill insert.
// Assumes vec is a vector of type SelfCount
// such that vec[i]() == i for all vec[i].
// Does not modify vec.
template <class C>
static void TestFillInsert(C vec, unsigned iat, unsigned n)
{
    unsigned count0 = SelfCount::OwnerCount();
    unsigned size = vec.size();
    auto spot = vec.begin() + iat;
    vec.insert(spot,n,SelfCount(843));
    assert(vec.size() == size+n);
    assert(SelfCount::OwnerCount() == count0+n);
    auto pCell = vec.begin();
    for (unsigned i = 0; i < vec.size(); ++i, ++pCell)
    {
        if (i < iat) assert((*pCell)() == i);
        else if (iat+n <= i) assert((*pCell)() == i-n);
        else (*pCell)() == 843;
    }
}

static void DoMoveAssignment(mf_vector<SelfCount,50> &b, mf_vector<SelfCount,70> &c)
{
    c = std::move(b);
}

template<unsigned B, size_t N>
static bool AscendingInts(mf_vector<SelfCount,B,N> vec)
{
    for (unsigned i = 0; i < vec.size(); ++i){
        if (vec[i]() != i) 
            return false;
    }
    return true;
}

int main() {

    // Constructors.
    {
        // fill
        {
            mf_vector<int,8> i20(17);
            assert(i20.capacity() == 8*8);
            assert(i20.size() == 17);
            for (int k:i20) assert(k==0);
            mf_vector<int,8>::const_iterator x = i20.begin();

            mf_vector<int,16,3> i23(17, -6);
            assert(i23.capacity() == 16*3);
            assert(i23.size() == 17);
            for (int k:i23) assert(k==-6);
        }
        // range
        assert(SelfCount::OwnerCount() == 0);
        std::list<int> li;
        for (int i = 0; i < 30; ++i) li.push_back(i-13);
        mf_vector<SelfCount,30> sv(li.begin(),li.end());
        assert(SelfCount::OwnerCount() == 30);
        assert(sv.size() == 30);
        for (int i = 0; i < 30; ++i) assert(sv[i]() == i-13);
        for (int i = 0; i < 30; ++i) assert(*(sv.begin()+i) == i-13);
        for (int i = 0; i < 30; ++i) 
            assert(*(sv.end()-(30-i)) == i-13);
        {
            // copy
            assert(SelfCount::OwnerCount() == 30);
            mf_vector<SelfCount,80> i80 (sv);
            assert(i80.size() == 30);
            assert(SelfCount::OwnerCount() == 60);
            for (int i = 0; i < 30; ++i) assert(i80[i]() == i-13);

            mf_vector<SelfCount,80> j80 (i80);
            assert(j80.size() == 30);
            assert(SelfCount::OwnerCount() == 90);
            for (int i = 0; i < 30; ++i) assert(j80[i]() == i-13);
        }
        {
            // move
            assert(SelfCount::OwnerCount() == 30);
            mf_vector<SelfCount,73> i73 {std::move(sv)};
            assert(sv.size() == 0);
            assert(i73.size() == 30);
            assert(SelfCount::OwnerCount() == 30);
            for (int i = 0; i < 30; ++i) assert(i73[i]() == i-13);

            assert(SelfCount::OwnerCount() == 30);
            mf_vector<SelfCount,73> j73 (std::move(i73));
            assert(i73.size() == 0);
            assert(j73.size() == 30);
            assert(SelfCount::OwnerCount() == 30);
            for (int i = 0; i < 30; ++i) assert(j73[i]() == i-13);
        }
    }
    {
        // Default Constructor, empty()
        mf_vector<SelfCount,7> di7;
        assert(SelfCount::OwnerCount() == 0);
        assert(di7.size() == 0);
        assert(di7.empty());


        // emplace_back(), size()

        for (unsigned i = 0; i < 50; i+= 1){
            di7.emplace_back(i);
            assert(di7.size() == i+1);
            assert(SelfCount::OwnerCount() == di7.size());
        }

        const auto & cdi50{di7};

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di7.pop_back();
            assert(SelfCount::OwnerCount() == di7.size());
        }
        assert(di7.size() == 30);

        // at()
        assert(di7.at(9)() == 9);
        assert(cdi50.at(29)() == 29);
        try {
            int k = di7.at(30)();  // should throw std::out_of_bounds
            assert(false);
        } 
        catch (std::out_of_range) {}
        catch (...) {assert(false);}

        // operator[](), back(), front()
        assert(di7[7]() == 7);
        di7[7] = SelfCount(91);
        assert(di7[7]() == 91);
        di7[7] = SelfCount(7);
        assert(cdi50[23]() == 23);
        assert(di7.back()() == 29);
        di7.back() = SelfCount(92);
        assert(di7.back()() == 92);
        di7.back() = SelfCount(29);
        assert(di7.back()() == 29);
        assert(di7.front()() == 0);
        assert(cdi50.front()() == 0);

        // push_back()
        di7.push_back(SelfCount(30));
        assert(cdi50[30]() == 30);
        assert(SelfCount::OwnerCount() == 31);

        assert(di7.size() == 31);

        // begin(), end()
        assert((*(cdi50.begin()))() == 0);
        *(di7.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di7.begin()+8) = SelfCount(8);
        assert(di7.begin()+di7.size() == di7.end());
        assert(SelfCount::OwnerCount() == di7.size());

        // cbegin(), cend()
        assert(*(di7.cbegin()+6) == cdi50[6]);
        assert((*(di7.cbegin()))() == 0);
        *(di7.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di7.begin()+8) = SelfCount(8);
        assert(di7.cbegin()+di7.size() == cdi50.end());
        assert(SelfCount::OwnerCount() == di7.size());

        // rbegin(), rend(), crbegin(), crend()
        assert(&(*(di7.crbegin()+6)) == &(*(cdi50.cend()-7)));
        assert((*(di7.crbegin()))() == 30);
        *(di7.rbegin()+8) = SelfCount(71);
        assert(cdi50[22]() == 71);
        *(di7.rbegin()+8) = SelfCount(22);
        assert(di7.crbegin()+di7.size() == cdi50.rend());
        assert(SelfCount::OwnerCount() == di7.size());
        for (int i = 0; i < 31; i++) assert(cdi50[i]() == i);

        // erase()
        assert(di7.size() == 31);
        assert(di7.erase(di7.begin()+8) == di7.begin()+8);

        assert(SelfCount::OwnerCount() == di7.size());
        assert(di7.size() == 30);
        assert(di7[7]() == 7);
        assert(di7[8]() == 9);
        assert(di7[29]() == 30);

        // emplace()
        assert((*di7.emplace(di7.begin()+8,96))() == 96);
        assert(di7[9]() == 9);
        assert(di7.size() == 31);
        assert(di7[30] == 30);
        assert(SelfCount::OwnerCount() == di7.size());

        // range erase()
        auto spot = di7.erase(di7.begin()+8, di7.begin()+12);
        assert(spot == di7.begin()+8);
        assert(*spot == 12);
        assert(*(spot-1) == 7);
        assert(di7.size() == 27);
        assert(SelfCount::OwnerCount() == di7.size());

        spot = di7.erase(di7.end()-7, di7.end());
        assert(spot == di7.end());
        assert(di7.size() == 20);
        assert(di7.back() == 23);
        assert(SelfCount::OwnerCount() == di7.size());

        // clear()
        di7.clear();
        assert(di7.size() == 0);
        assert(SelfCount::OwnerCount() == di7.size());
    }
    assert(SelfCount::OwnerCount() == 0);
    {
        // end(), iterator arithmetic
        mf_vector<int,5> i5;
        assert(i5.begin() == i5.end());
        for (unsigned j = 0; j < 51; ++j) {
            i5.push_back(j);
            assert(i5.begin()+i5.size() == i5.end());
            assert(*(i5.end()-1) == j);
            assert(*(i5.end()-i5.size()) == 0);
        }
    }
    {
        // Operation on iterators
        using Vec = mf_vector<int,5>;
        using It  = Vec::const_iterator;
        Vec vec({0,1,2,3,4,5,6,7});
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
        mf_vector<int,2> dv;
        dv.assign(6,-29);
        assert(dv.size() == 6);
        for (auto i: dv) assert(i==-29);
    }{
        // range type
        mf_vector<int,10> dv;
        std::list<unsigned> lst;
        for (unsigned i = 9; i < 9+8; ++i){
            lst.push_back(i);
        }
        dv.assign(lst.begin(),lst.end());
        assert(dv.size() == 8);
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv[i-9]==i);}

        mf_vector<int,9> dv2;
        dv2.push_back(78);
        dv2.assign(dv.begin(),dv.end());
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv2[i-9]==i);}

        // initializer list type
        dv.assign({-3, 27, 12, -397});
        assert(dv.size() == 4);
        assert(dv[2] == 12);
    }{
        // assignment operators
        assert(SelfCount::OwnerCount() == 0);
        mf_vector<SelfCount, 50> a, b;
        for (unsigned i = 0; i<20; ++i)
            a.emplace_back(i);
        assert(SelfCount::OwnerCount() == 20);

        // copy operator=()
        b = a;
        assert(a==b);
        assert(b.size() == 20);
        assert(SelfCount::OwnerCount() == 40);

        a = a;
        assert(a.size() == 20);
        assert(a == b);
        assert(SelfCount::OwnerCount() == 40);

        // move operator=()
        b = std::move(a);
        assert(a.size() == 0);
        assert(b.size() == 20);
        assert(AscendingInts(b));
        assert(SelfCount::OwnerCount() == 20);
        assert(a != b);

        a = b;
        assert(SelfCount::OwnerCount() == 40);
        assert(b.size() == 20);
        assert(AscendingInts(b));
        assert(a.size() == 20);
        assert(AscendingInts(a));

        b = std::move(b);
        assert(SelfCount::OwnerCount() == 40);
        assert(b.size() == 20);
        assert(a == b);

        // initializer_list operator=()
        b = {14, -293, 1200, -2, 0};
        assert(SelfCount::OwnerCount() == 25);
        assert(b.size() == 5);
        assert(b[3]() == -2);
        assert(a.size() == 20);
        assert(AscendingInts(a));
    }{
        // assignment operators
        mf_vector<SelfCount, 50> a;
        mf_vector<SelfCount, 50> b;
        assert(SelfCount::OwnerCount() == 0);
        for (unsigned i = 0; i<20; ++i)
            a.emplace_back(i);
        assert(SelfCount::OwnerCount() == 20);

        // copy operator=()
        b = a;
        assert(a==b);
        assert(b.size() == 20);
        assert(SelfCount::OwnerCount() == 40);

        a = a;
        assert(a.size() == 20);
        assert(a == b);
        assert(SelfCount::OwnerCount() == 40);

        // move operator=()
        b = std::move(a);
        assert(b.size() == 20);
        assert(a.empty());
        assert(SelfCount::OwnerCount() == 20);
        assert(a != b);

        a = b;
        assert(SelfCount::OwnerCount() == 40);

        b = std::move(b);
        assert(SelfCount::OwnerCount() == 40);
        assert(b.size() == 20);
        assert(a == b);

        // move operator=() with different block sizes
        mf_vector<SelfCount,70> c;
        for (unsigned i = 12; i < 62; ++i)
        {
            c.emplace_back(i);
        }
        assert(SelfCount::OwnerCount() == 90);

        DoMoveAssignment(b,c);
        assert(SelfCount::OwnerCount() == 40);
        assert(c.size() == 20);
        assert(b.empty());
        assert(c[1] == 1);
    }{
        // The many flavors of insert()

        assert(SelfCount::OwnerCount() == 0);
        mf_vector<SelfCount,5> roop;
        for (unsigned i = 0; i < 47; ++i)
            roop.emplace_back(i);

        // Move insert(), copy insert()
        {
            auto spot = roop.begin()+9;
            SelfCount obj(71);
            assert(SelfCount::OwnerCount() == 48);
            // copy first
            roop.insert(spot,obj);
            assert(roop.size() == 48);
            assert(SelfCount::OwnerCount() == 49);
            assert(roop[8]() == 8);
            assert(roop[9]() == 71);
            assert(roop[10]() == 9);
            assert(roop[47]() == 46);
            roop.erase(spot);

            // then move
            assert(SelfCount::OwnerCount() == 48);
            roop.insert(spot,std::move(obj));
            assert(SelfCount::OwnerCount() == 48);
            assert(roop.size() == 48);
            assert(roop[8]() == 8);
            assert(roop[9]() == 71);
            assert(roop[10]() == 9);
            assert(roop[47]() == 46);
            roop.erase(spot);
        }

        // Fill insert()
        assert(roop.size() == 47);
        assert(SelfCount::OwnerCount() == 47);
        TestFillInsert(roop,20,3);
        TestFillInsert(roop,43,13);
        TestFillInsert(roop,roop.size(),13);
        TestFillInsert(roop,0,13);
        {
            // Range insert()
            std::list<int> intList;
            for (int i = 0; i < 9; ++i) {
                intList.push_back(i+173);
            }
            mf_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::OwnerCount() == 47*2);
            r2.insert(r2.begin()+31, intList.begin(), intList.end());
            assert(r2.size() == 47+9);
            assert(SelfCount::OwnerCount() == 2*47+9);
            assert(r2[30]() == 30);
            assert(r2[31+4]() == 4+173);
            assert(r2[31+9]() == 31);
        }
        assert(SelfCount::OwnerCount() == 47);
        {
            // Initializer list insert()
            mf_vector<SelfCount,19> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::OwnerCount() == 47*2);
            using Z = SelfCount;
            r2.insert(r2.begin()+31, {Z(-72),Z(0),Z(274),Z(-34245)});
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
        mf_vector<SelfCount, 99> v99;
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
        mf_vector<SelfCount, 9> va, vc;
        mf_vector<SelfCount, 9> vb, vd;
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
        mf_vector<SelfCount, 99> va, vb, vc, vd;
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
        std::swap(va,vb);
        assert(vb.size() == 57);
        assert(va.size() == 19);
        assert(SelfCount::OwnerCount() == 2*(19+57));
        assert(vd == va);
        assert(vc == vb);
    }{
        // comparison functions
        mf_vector<int,73> v0;
        mf_vector<int,70> v1;
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
    {
        /*
        // Grow it big (needs 1.5GB)
        const uint64_t sz = 3*64*1024*1024;
        mf_vector<int64_t,1024> big;
        big.reserve(sz);
        assert(big.capacity() >= sz);
        for (int_fast64_t j = 0; j < sz; ++j)
            big.push_back(j);
        for (int_fast64_t j = 0; j < sz; j += 93)
            assert(big[j] == j);
        */
    }
}