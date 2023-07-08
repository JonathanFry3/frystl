// mf_vector.hpp - defines a memory-friendly vector-like template class
//
// mf_vector<T,B,N> is a std::vector-like class that stores elements of type T
// in blocks of size B.  It grows by adding additional blocks, so existing
// blocks are never reallocated. It stores pointers to its blocks in a std::vector.
// That std::vector is initially allocated at size N.  This storage scheme
// is similar to that used by std::deque.
//
// It's functions have the same semantics as those of std::vector with the
// following exceptions:
//
//  (1) shrink_to_fit(), data(), max_size(), and get_allocator(), 
//      are not implemented.
//  (2) capacity() returns the number of elements the mf_vector can store
//      without reallocating the std::vector of block pointers.
//  (3) If capacity() < reserve(n), reserve(n) reallocates the std::vector
//      of blocks to allow space for n elements with further reallocation.
//      It does not create additional blocks or modify existing blocks.
//  (3) the function block_size() returns the value of the B parameter.
//
// Performance: Generally similar to std::deque.
// Adding an element at the end has amortized constant complexity.
// Random access (operator[]) takes longer than for a std::vector or 
// array, as a lookup in the vector of storage block pointers is needed.
// This extra lookup is far faster if B is a power of 2, as an optimizing
// compiler will replace the division and remainder operations needed with
// shift and mask operations.
// Sequential access (using an iterator) is faster than random access, as 
// lookup is not needed except between storage blocks.
// Like std::vector, mf_vector has an efficient swap() member function
// and non-member override that exchange implementations but do not copy
// any member values. That function is used for move construction and
// move assignment between mf_vectors having the same T and B parameters.
//
// Data Races: If reallocation happens in the vector that tracks the
// storage blocks, all elements of that vector are modified, so no 
// other access is safe. 
// Erase() and insert() modify all the elements at and after 
// their target positions. At() and operator[] can be used to
// modify an existing element, but accessing or modifying other elements
// is safe.  Emplace_back() and push_back() add elements at the end, 
// but do not modify any other elements, so accessing or modifying
// them is safe unless the block pointer vector is reallocated, 
// but any operation that explicitly or implicitly
// uses end() is not safe.
//
// Pointer, reference, and iterator invalidation.  Any operation
// that inserts elements (emplace(), insert()) invalidates
// pointers, references, and iterators at and after the target.  Those
// operations, plus push_back() and emplace_back(), can cause the 
// block pointer vector to be reallocated, in which case all
// iterators are invalidated.  The erase() function invalidates
// pointers, references, and iterators pointing to its target
// and elements after its target.
//
// Contrast with std::vector:
// + The memory required by std::vector is three time size() during
//   reallocation. Mf_vector never requires more than B extra spaces.
// + The copying of all the data can make the growing of a large
//   std::vector relatively slower.
// - Random access (operator[]) is faster with std::vector.
//
// Contrast with std::deque:
// + Mf_vector's storage is much more customizable than std::deque's.  
// + For some mutithreaded algorithms, read operations 
//   do not need any synchronization if the mf_vector used never 
//   reallocates the vector of block pointers. (Those are algorithms where
//   the only write operations are appending elements to the back
//   and that the read operations do not use end(), so no searches.)
// - Std::deque is a deque, i.e. elements can efficiently be pushed to
//   and popped from the front.
/*
MIT License

Copyright (c) 2020-2023 by Jonathan Fry

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef FRYSTL_MF_VECTOR
#define FRYSTL_MF_VECTOR

#include <utility>   // max, pair
#include <stdexcept> // out_of_range
#include <iterator>  // reverse_iterator
#include <vector>
#include <type_traits> // conditional
#include <algorithm>   // rotate
#include <initializer_list>
#include "frystl-defines.hpp"

namespace frystl
{
/***********************************************************************************/    
/***********************************************************************************/    
/**********************  Iterator Implementation  **********************************/    
/***********************************************************************************/    
/***********************************************************************************/    
    template <class T, bool IsConst>
        struct MFV_Iterator: std::random_access_iterator_tag
        {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = typename std::conditional<IsConst,const T*,T*>::type;
        using reference         = typename std::conditional<IsConst,const T&,T&>::type;
        using this_type         = MFV_Iterator<T, IsConst>;

        using ValPtr            = T*;
        using BlkPtr            = ValPtr*;
        BlkPtr _block;
        ValPtr _first;
        ValPtr _last;
        ValPtr _current; 

        MFV_Iterator() = delete;

        MFV_Iterator(pointer* block, pointer first, pointer last, pointer current)
        : _block(block), _first(first), _last(last), _current(current)
        {}

        // Implicit conversion from iterator to const_iterator
        template <bool WasConst, class = std::enable_if_t<IsConst && !WasConst> >
        MFV_Iterator(const MFV_Iterator<T, WasConst>& x) noexcept
            : _block(x._block)
            , _first(x._first)
            , _last(x._last)
            , _current(x._current)
        {}

        MFV_Iterator(const MFV_Iterator& x) = default;

        MFV_Iterator& operator++() noexcept  // prefix increment, as in ++iter
        {
            Increment();
            return *this;
        }
        MFV_Iterator operator++(int i) noexcept  // postfix increment, as in iter++
        {
            MFV_Iterator result = *this;
            Increment();
            return result;
        }
        MFV_Iterator& operator--() noexcept  // prefix decrement, as in --iter;
        {
            Decrement();
            return *this;
        }
        MFV_Iterator operator--(int i) noexcept // postfix decrement, as in iter--;
        { 
            MFV_Iterator result = *this;
            Decrement();
            return result;
        }
        bool operator==(const MFV_Iterator& other) const noexcept
        {
            return _current == other._current;
        }
        bool operator<(const MFV_Iterator& other) const noexcept
        {
            return _block < other._block || 
                (_block == other._block && _current < other._current);
        }
        MFV_Iterator operator+=(difference_type a) noexcept
        {
            const difference_type blkSize = _last - _first;
            const difference_type offset = a + (_current - _first);
            if (0 <= offset && offset < blkSize)
                _current += a;
            else {
                const long nodeOffset = 
                    (offset > 0) ? offset / blkSize
                    : (1 + offset - blkSize) / blkSize;
                _block += nodeOffset;
                _first = *_block;
                _last = _first + blkSize;
                _current = _first + (offset - nodeOffset * blkSize);
            }
            return *this;
        }
        MFV_Iterator operator-=(difference_type a) noexcept
        {
            return operator+=(-a);
        }
        MFV_Iterator operator+(difference_type a) const noexcept
        {
            MFV_Iterator t{ *this };
            return t += a;
        }
        MFV_Iterator operator-(difference_type a) const noexcept
        {
            MFV_Iterator t{ *this };
            return t += (-a);
        }
        reference operator*() const noexcept 
        {
            return *_current;
        }
        pointer operator->() const noexcept 
        {
            return _current;
        }
        reference operator[](difference_type x) 
        {
            return *(*this + x);
        }


    private:
        void Increment()
        {
            _current++;
            if (_last == _current) {
                ++_block;
                difference_type blockSize = _last - _first;
                _current = _first = *_block;
                _last = _first + blockSize;
            }
        }
        void Decrement()
        {
            if (_first == _current) {
                difference_type blockSize = _last - _first;
                --_block;
                _first = *_block;
                _current = _last = _first + blockSize;
            }
            --_current;
        }
    };

    // Non-member overrides for iterator operands

    template <class T, bool C1, bool C2>
    bool operator!=(const MFV_Iterator<T, C1>& a,
                    const MFV_Iterator<T, C2>& b)
    {
        return !(a == b);
    }
    template <class T, bool C1, bool C2>
    bool operator> (const MFV_Iterator<T, C1>& a,
                    const MFV_Iterator<T, C2>& b)
    {
        return b < a;
    }
    template <class T, bool C1, bool C2>
    bool operator<=(const MFV_Iterator<T, C1>& a,
                    const MFV_Iterator<T, C2>& b)
    {
        return !(b < a);
    }
    template <class T, bool C1, bool C2>
    bool operator>=(const MFV_Iterator<T, C1>& a,
                    const MFV_Iterator<T, C2>& b)
    {
        return !(a < b);
    }
    template <class T, bool C>
    typename MFV_Iterator<T, C>::difference_type
    operator-(const MFV_Iterator<T, C>& a, const MFV_Iterator<T, C>& b)
    { 
        return (a._block - b._block - 1) * (a._last - a._first)
            + (a._current - a._first) + (b._last - b._current);
    }
    template <class T, bool C1, bool C2>
    typename MFV_Iterator<T, C1>::difference_type
    operator-(const MFV_Iterator<T, C1>& a, const MFV_Iterator<T, C2>& b)
    {
        return (a._block - b._block - 1) * (a._last - a._first)
            + (a._current - a._first) + (b._last - b._current);
    }

/***********************************************************************************/    
/***********************************************************************************/    
/**********************  mf_vector Implementation  *********************************/    
/***********************************************************************************/    
/***********************************************************************************/    


    // A dummy to give the final pointer in any mf_vector a valid
    // data address to use.
    //
    // This is an ugly kluge.  Before changing it, consider how to
    // increment an iterator to end() when the last block is full.
    static long MFVectorDummyEnd = 0;

    template <
        class T,           // Value type
        unsigned BlockSize // Number of T elements per block.
                           // Powers of 2 are faster.
        = std::max<unsigned>(4096 / sizeof(T), 16),
        size_t NBlocks = BlockSize>
    class mf_vector
    {
    public:
        using value_type        = T;
        using reference         = T&;
        using pointer           = T*;
        using const_pointer     = const T*;
        using const_reference   = const T&;
        using size_type         = size_t;
        using this_type         = mf_vector;

        using iterator                  = MFV_Iterator<T, false>;
        using const_iterator            = MFV_Iterator<T, true>;
        using reverse_iterator          = std::reverse_iterator<iterator>;
        using const_reverse_iterator    = std::reverse_iterator<const_iterator>;

        // Constructors
        mf_vector() // default c'tor
            : _size(0)
        {
            _blocks.reserve(NBlocks + 1);
            _blocks.push_back(reinterpret_cast<pointer>(&MFVectorDummyEnd));
        }
        // Fill constructors
        explicit mf_vector(size_type count, const_reference value)
            : mf_vector()
        {
            for (size_type i = 0; i < count; ++i)
                push_back(value);
        }
        explicit mf_vector(size_type count)
            : mf_vector(count, T())
        {
        }
        // Range constructors
        template <class InputIt,
                  typename = RequireInputIter<InputIt>> 
        mf_vector(InputIt first, InputIt last)
            : mf_vector()
        {
            for (InputIt i = first; i != last; ++i)
                push_back(*i);
        }
        // Copy Constructors
        mf_vector(const mf_vector& other)
            : mf_vector()
        {
            for (auto& value : other)
                push_back(value);
        }
        template <unsigned B1, size_t NB>
        explicit mf_vector(const mf_vector<T, B1, NB>& other)
            : mf_vector()
        {
            for (auto& value : other)
                push_back(value);
        }
        // Move constructors
        mf_vector(mf_vector&& other)
            : mf_vector()
        {
            swap(other);
        }
        template <unsigned B1, size_t NB>
        mf_vector(mf_vector<T, B1, NB>&& other)
            : mf_vector()
        {
            if (block_size() == other.block_size())
                swap(reinterpret_cast<mf_vector&>(other));
            else {
                for (auto&& value : other)
                    MovePushBack(std::move(value));
                other.clear();
            }
        }
        // initializer list constructor
        mf_vector(std::initializer_list<value_type> il)
            : mf_vector()
        {
            for (auto& value : il)
                push_back(value);
        }
        void clear() noexcept
        {
            for (auto& m : *this)
                m.~T(); // destruct all
            _size = 0;
            Shrink(); // free memory
        }
        ~mf_vector() noexcept
        {
            clear();
        }

        void reserve(size_type newCap)
        {
            size_type nBlocks = QuotientRoundedUp(newCap, BlockSize);
            _blocks.reserve(nBlocks+1);
        }
        size_type size() const noexcept
        {
            return _size;
        }
        size_type capacity() const noexcept
        {
            return block_size() * (_blocks.capacity() - 1);
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        template <class... Args>
        void emplace_back(Args... args)
        {
            Grow(_size + 1);
            iterator e = End();
            new (e.operator->()) value_type(args...);
            ++_size;
        }
        template <class... Args>
        iterator emplace(const_iterator position, Args... args)
        {
            iterator pos = MakeRoom(position,1);
            new (pos.operator->()) T(args...);
            return pos;
        }
        void push_back(const_reference t)
        {
            Grow(_size + 1);
            iterator e = End();
            new (e.operator->()) value_type(t);
            ++_size;
        }
        void pop_back() noexcept
        {
            FRYSTL_ASSERT2(_size,"mf_vector::pop_back() on empty vector");
            iterator e = end() - 1;
            back().~T(); //destruct
            _size -= 1;
            if (e._first == e.operator->())
                Shrink();
        }
        reference back() noexcept
        {
            FRYSTL_ASSERT2(_size,"mf_vector::back() on empty vector");
            return *(end() - 1);
        }
        const_reference back() const noexcept
        {
            FRYSTL_ASSERT2(_size,"mf_vector::back() on empty vector");
            return *(cend() - 1);
        }
        reference front() noexcept
        {
            FRYSTL_ASSERT2(_size,"mf_vector::front() on empty vector");
            return **_blocks.data();
        }
        const_reference front() const noexcept
        {
            FRYSTL_ASSERT2(_size,"mf_vector::front() on empty vector");
            return **_blocks.data();
        }

        reference at(size_type index)
        {
            Verify(index < _size);
            return operator[](index);
        }
        const_reference at(size_type index) const
        {
            Verify(index < _size);
            return operator[](index);
        }
        reference operator[](size_type index) noexcept
        {
            FRYSTL_ASSERT2(index < _size,"mf_vector::operator[] index error");
            return *(_blocks[index / BlockSize] + index % BlockSize);
        }
        const_reference operator[](size_type index) const noexcept
        {
            FRYSTL_ASSERT2(index < _size,"mf_vector::operator[] index error");
            return *(_blocks[index / BlockSize] + index % BlockSize);
        }
        iterator erase(const_iterator first, const_iterator last) noexcept
        {
            iterator f = MakeIterator(first);
            // Requires: begin()<=first && first<=last && last<=end()
            if (first < last)
            {
                iterator l = MakeIterator(last);
                for (iterator it = f; it < l; ++it)
                    it->~value_type();
                std::move(l, end(), f);
                _size -= last - first;
                Shrink();
            }
            return f;
        }
        iterator erase(const_iterator position) noexcept
        {
            // Requires begin() <= position < end()
            return erase(position, position + 1);
        }
        //
        //  Assignment functions
        void assign(size_type n, const_reference val)
        {
            clear();
            while (size() < n)
                push_back(val);
        }
        void assign(std::initializer_list<value_type> x)
        {
            clear();
            for (auto& a : x)
                push_back(a);
        }
        template <class InputIterator,
                  typename = RequireInputIter<InputIterator>> 
        void assign(InputIterator begin, InputIterator end)
        {
            clear();
            for (InputIterator k = begin; k != end; ++k)
                push_back(*k);
        }
        mf_vector& operator=(const mf_vector& other) noexcept
        {
            if (this != &other)
                assign(other.begin(), other.end());
            return *this;
        }
        mf_vector& operator=(mf_vector&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                swap(other);
            }
            return *this;
        }
        template <unsigned B, size_t N>
        mf_vector& operator=(mf_vector<T, B, N>&& other) noexcept
        {
            mf_vector* pOther = reinterpret_cast<mf_vector*>(&other);
            if (this != pOther)
            {
                clear();
                if (block_size() == pOther->block_size()) {
                    swap(*pOther);
                }
                else {
                    for (auto& val : other) {
                        emplace_back(std::move(val));
                    }
                    other.clear();
                }
            }
            return *this;
        }
        mf_vector& operator=(std::initializer_list<value_type> il)
        {
            assign(il);
            return *this;
        }

        // move insert()
        iterator insert(const_iterator position, value_type&& val)
        {
            return emplace(position, std::move(val));
        }       
        // copy insert()
        iterator insert(const_iterator position, value_type& val)
        {
            return emplace(position, val);
        }
        // fill insert()
        iterator insert(const_iterator position, size_type n, const value_type& val)
        {
            iterator p = MakeRoom(position,n);
            // copy val n times into newly available cells
            for (iterator i = p; i < p + n; ++i)
            {
                new (i.operator->()) value_type(val);
            }
            return p;
        }
        // Range insert()
        template <class InputIterator,
                  typename = RequireInputIter<InputIterator>> 
        iterator insert(const_iterator position, InputIterator first, InputIterator last)
        {
            size_type posIndex = position-begin();
            size_type oldSize = _size;   
            try {
                // append(first,last);
                while (first != last) {
                    push_back(*first++);
                }
                std::rotate(begin()+posIndex, begin()+oldSize, end());
            } catch (std::bad_alloc){
                _size = oldSize;
                Shrink();
            }
            return begin()+posIndex;
        }
        // initializer list insert()
        iterator insert(const_iterator position, std::initializer_list<value_type> il)
        {
            // Requires begin()<=position && position<=end()
            size_type n = il.size();
            iterator p = MakeRoom(position,n);
            // copy il into newly available cells
            auto j = il.begin();
            for (iterator i = p; i < p + n; ++i, ++j)
            {
                new (i.operator->()) value_type(*j);
            }
            return p;
        }
        void resize(size_type n, const value_type& val)
        {
            while (n < size())
                pop_back();
            while (size() < n)
                push_back(val);
        }
        void resize(size_type n)
        {
            while (n < size())
                pop_back();
            while (size() < n)
                emplace_back();
        }
        iterator begin() noexcept
        {
            return Begin();
        }
        iterator end() noexcept
        {
            return End();
        }
        const_iterator begin() const noexcept
        {
            return Begin();
        }
        const_iterator end() const noexcept
        {
            return End();
        }
        const_iterator cbegin() const noexcept
        {
            return Begin();
        }
        const_iterator cend() const noexcept
        {
            return End();
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }
        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }
        void swap(mf_vector& other)
        {
            std::swap(_blocks, other._blocks);
            std::swap(_size, other._size);
        }

        constexpr unsigned block_size() const noexcept
        {
            return _blockSize;
        }
    private:
        // The functions below manage storage for the container.  The vector _blocks
        // stores pointers to a sequence of storage blocks, each of size BlockSize,
        // plus one extra pointer at the end.  The extra pointer at the end points to
        // "dummyEnd" and is used to make the end() function work properly when 
        // size() is a multiple of BlockSize.
        using storage_type =
            std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
        static const unsigned _blockSize = BlockSize;
        std::vector<pointer> _blocks;
        size_type _size;

        // Grow capacity to newSize..
        // Invalidates iterators.
        void Grow(size_type newSize)
        {
            try {
                while ((_blocks.size() - 1) * _blockSize < newSize)
                {
                    pointer b = reinterpret_cast<pointer>(new storage_type[_blockSize]);
                    _blocks.push_back(_blocks.back());
                    *(_blocks.end() - 2) = b;
                }
            }
            catch (...) {
                Shrink();
                throw;
            }
        }
        // Release any no-longer-needed storage blocks.
        // Expects _size already reflects the value(s) just erased.
        void Shrink() noexcept
        {
            size_type cap = _blockSize * (_blocks.size() - 1);
            if (_size + _blockSize <= cap) {
                auto ender = _blocks.back();
                do {
                    delete[] reinterpret_cast<storage_type*>(*(_blocks.end() - 2));
                    _blocks.pop_back();
                    cap -= _blockSize;
                } while (_size + _blockSize <= cap);
                _blocks.back() = ender;
            }
            FRYSTL_ASSERT2(QuotientRoundedUp(_size,BlockSize) + 1 == _blocks.size(),
                "mf_vector: internal error in Shrink()");
        }
        // Make n spaces available starting at pos.  Shift
        // all elements at and after pos right by n spaces.
        // Updates _size.
        iterator MakeRoom(const_iterator pos, size_type n)
        {
            size_type index = pos - begin();
            size_type nu = std::min(size()-index, n);
            Grow(_size + n);    // invalidates iterators
            iterator result = begin()+index;
            // fill the uninitialized target cells by move construction
            iterator from = end() - nu;
            iterator to = from + n;
            for (size_type i = 0; i < nu; ++i)
                new ((to++).operator->()) value_type(std::move(*(from++)));
            // shift elements to previously occupied cells by move assignment
            std::move_backward(result, end() - nu, end());
            _size += n;
            return result;
        }
        void MovePushBack(T&& value)
        {
            Grow(_size + 1);
            iterator e = End();
            new (e.operator->()) T(std::move(value));
            ++_size;
        }
        iterator Begin() const noexcept
        {
            // MFV_Iterator(pointer* block, pointer first, pointer last, pointer current)
            pointer* b = const_cast<pointer*>(_blocks.data());
            return iterator(b, *b, *b + BlockSize, *b);
        }
        iterator End() const noexcept
        {
            pointer* e = const_cast<pointer*>(_blocks.data() + _size / BlockSize);
            return iterator(e, *e, *e + BlockSize, *e + _size % BlockSize);
        }
        static void Verify(bool cond)
        {
            if (!cond)
                throw std::out_of_range("mf_vector range error");
        }
        iterator MakeIterator(const const_iterator& ci) noexcept
        {
            return iterator(
                const_cast<pointer*>(ci._block), 
                const_cast<pointer>(ci._first),
                const_cast<pointer>(ci._last),
                const_cast<pointer>(ci._current));
        }
    }; // template class mf_vector
    //
    //*******  Non-member overloads
    //
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator==(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator!=(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        return !(rhs == lhs);
    }
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator<(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator<=(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        return !(rhs < lhs);
    }
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator>(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        return rhs < lhs;
    }
    template <class T, unsigned B1, unsigned B2, size_t N1, size_t N2>
    bool operator>=(const mf_vector<T, B1, N1>& lhs, const mf_vector<T, B2, N2>& rhs)
    {
        return !(lhs < rhs);
    }
    template <class T, unsigned BS, size_t NB0, size_t NB1>
    void swap(mf_vector<T, BS, NB0>& a, mf_vector<T, BS, NB1>& b)
    {
        a.swap(b);
    }
}; // namespace frystl
#endif      // ndef FRYSTL_MF_VECTOR