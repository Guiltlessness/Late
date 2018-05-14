#ifndef MY_VECTOR_H
#define MY_VECTOR_H
#include <memory>			//for a allocator
#include <iterator>			//for operations on iterators
#include <utility>			//for std::move & std::forward
#include <initializer_list>		//for operations on initializer lists
#include <type_traits>			//for cheking type
#include <stdexcept>			//for using std::out_of_range
#include <cstddef>          //ptrdiff_t
#include <functional>


namespace student {

template<class It>
void move_block(It* dst, size_t range, const It* source)
{
    for(size_t i = 0; i != range; ++i){
        new (dst + i) It(std::move(source[i]));
        source[i].~It();
    }
    //    size_t quantity_of_bits = sizeof(typename std::iterator_traits<It>::value_type);
    //    memmove(dst, source, range * quantity_of_bits);
}


template<class P>
void copy_block(P* dst, size_t range, const P* source)
{
    std::function<void(P*)> d = [range](P* p)
    {
        for(size_t i = 0; i != range; ++i)
            p[i].~P();
        operator delete[]((void*)p);
    };

    std::unique_ptr<P[], std::function<void(P*)>> buf(static_cast<P*>(operator new(range * sizeof(P))), d);
    for(size_t i = 0; i != range; ++i){
        new (&buf[i]) P(std::move(source[i]));
        source[i].~P();
    }

    for(size_t i = 0; i != range; ++i){
        new (dst + i) P(std::move(buf[i]));
    }

    //    size_t quantity_of_bits = sizeof(typename std::iterator_traits<It>::value_type);
    //    memcpy(dst, source, range * quantity_of_bits);
}

//the class is simle unique_ptr.
template<class T, class Alloc>
class my_ptr
{
    Alloc pincette{};
    T* allocated_memory{nullptr};
    size_t my_capacity{0};

public:

    size_t my_size{0};


    void swap(my_ptr &obj)
    {
        std::swap(allocated_memory, obj.allocated_memory);
        std::swap(my_size, obj.my_size);
        std::swap(my_capacity, obj.my_capacity);
    }
    my_ptr(){}
    my_ptr(size_t size) : my_capacity(size), my_size(0)
    {
        allocated_memory = pincette.allocate(my_capacity);
    }
    my_ptr(my_ptr<T, Alloc>&& obj)
        : my_ptr()
    {
        swap(obj);
    }
    ~my_ptr()
    {
        if(allocated_memory)
        {
            if(std::is_class<T>::value)
                for (size_t i = 0; i != my_size; ++i)
                    pincette.destroy(allocated_memory + i);
            pincette.deallocate(allocated_memory, my_capacity);
            allocated_memory = nullptr;
        }
    }
    void deallocate_without_destoy()
    {
        pincette.deallocate(allocated_memory, my_capacity);
        allocated_memory = nullptr;
        my_size = 0;
        my_capacity = 0;
    }

    void destroy_without_deallocate()
    {
        if(std::is_class<T>::value && my_size){
            do {
                --my_size;
                pincette.destroy(allocated_memory + my_size);
            } while (0 != my_size);

        }
    }
    explicit operator bool() const {
        return !!allocated_memory;
    }

    size_t const& capacity() const
    {
        return my_capacity;
    }
    size_t & size()
    {
        return my_size;
    }
    T* data()
    {
        return allocated_memory;
    }
    void destroy(size_t i)
    {
        if(std::is_class<T>::value)
            pincette.destroy(allocated_memory + i);
    }
    size_t const& max_size() const
    {
        return pincette.max_size();
    }
    T& operator [](size_t num)
    {
        return allocated_memory[num];
    }

    T const& operator [](size_t num) const
    {
        return allocated_memory[num];
    }
    my_ptr operator =(my_ptr&) = delete;
};


//class for exception
class out_of_range : std::exception
{
    std::string mes{"Bad access from \"at\""};
public:
    out_of_range(){}
    out_of_range(std::string const& str):mes(str){}
    virtual const char* what() const noexcept { return mes.data();}
};

//vector
template<typename T, typename Alloc = std::allocator<T>>
class vector {
    typedef my_ptr<T, Alloc> mem;
    mem ptr{};
    size_t const& my_capacity{ ptr.capacity() };
    size_t & my_size{ ptr.size() };
    T* allocated_memory(){return ptr.data();}


    template<typename InIt> using IfConvertableToInputIt =
    typename std::enable_if<
    std::is_convertible<
    typename std::iterator_traits<InIt>::iterator_category,
    std::input_iterator_tag>::value>::type;



    //some functions
    template<class ...ARG>
    void construct(size_t i, ARG && ... args)
    {
        new (allocated_memory() + i) T(std::forward<ARG>(args)...);
    }

    template<class It, typename = IfConvertableToInputIt<It>>
    void copy_by_iters(It first, It last)
    {

        size_t range = std::distance(first, last);
        if (allocated_memory())
            ptr.destroy_without_deallocate();

        if (my_capacity < range){
            mem temp(range);
            ptr.swap(temp);
        }

        for (It it = first; it != last; it = ++it, ++my_size)
            construct(my_size, *it);
    }

    void upsize()
    {
        mem temp(std::move(ptr));
        if (temp){
            mem temp2(temp.capacity() * 2);
            ptr.swap(temp2);
        }

        else{
            mem temp2(2);
            ptr.swap(temp2);
        }

        move_block(allocated_memory(), temp.size(), temp.data());
        temp.deallocate_without_destoy();
    }



public:

    //types
    typedef T                                                           value_type;
    typedef T*                                                          pointer;
    typedef const T*                                                    const_pointer;
    typedef size_t                                                      size_type;
    typedef T&                                                          reference;
    typedef const T&                                                    const_reference;
    typedef pointer                                                     iterator;
    typedef const_pointer                                               const_iterator;
    typedef std::reverse_iterator<iterator>                             reverse_iterator;
    typedef std::reverse_iterator<const_iterator>                       const_reverse_iterator;
    typedef ptrdiff_t                                                   difference_type;

    //constructor/destoy

    //default
    vector() noexcept {}

    //destroy
    ~vector() { }

    //fill
    explicit vector(size_type n) :ptr(n)
    {
        for(; n != my_size; ++my_size)
            construct(my_size);
    }

    //fill
    explicit vector(size_type n, const T &val)
        : ptr(n)
    {
        for( ; my_size != n; ++my_size)
            construct(my_size, val);
    }

    //range
    template<typename It, typename = IfConvertableToInputIt<It>>
    vector(It first, It last) : vector()
    {
        copy_by_iters(first, last);
    }

    //copy
    vector(const vector<T, Alloc> &v)
        : ptr(v.my_size)
    {
        for( ; my_size != v.my_size; ++my_size)
            construct(my_size, v[my_size]);
    }

    //move
    vector(vector<T, Alloc> &&v) noexcept : vector<T, Alloc>()
    {
        this->swap(v);
    }

    //initializer list
    vector(std::initializer_list<T> lt) : ptr(lt.size())
    {
        auto it = lt.begin();
        for( ; my_size != lt.size(); ++my_size, ++it)
            construct(my_size, *it);
    }

    //Assign content

    //copy
    vector<T, Alloc> & operator = (const vector<T, Alloc> &v)
    {
        ptr.destroy_without_deallocate();

        if (my_capacity < v.my_size){
            mem temp(v.my_size);
            ptr.swap(temp);
        }

        for ( ; my_size != v.my_size; ++my_size)
            construct(my_size, v[my_size]);
        return *this;
    }

    //move
    vector<T, Alloc> & operator = (vector<T, Alloc> &&v)
    {
        if (my_size)
            ptr.destroy_without_deallocate();

        this->swap(v);
        return *this;
    }

    //initializer list
    vector<T, Alloc> & operator = (std::initializer_list<T> lt) {
        this->swap(vector<T, Alloc>(lt));
        return *this;
    }

    //Assign vector content

    //fill
    void assign(size_type some_size, const T&  value)
    {
        if(my_capacity < some_size){
            mem temp(some_size);
            ptr.swap(temp);
        }
        else
            ptr.destroy_without_deallocate();
        for ( ; my_size != some_size; ++my_size)
            construct(my_size, value);
    }

    //range
    template<class It, typename = IfConvertableToInputIt<It>>
    void assign(It first, It last)
    {
        copy_by_iters(first, last);
    }

    //initializer list
    void assign(std::initializer_list<T> lt)
    {
        copy_by_iters(lt.begin(), lt.end());
    }


    // iterators:

    iterator begin() noexcept
    {
        return allocated_memory();
    }

    const_iterator cbegin() const noexcept
    {
        return allocated_memory();
    }

    iterator end() noexcept {
        return allocated_memory() + my_size;
    }

    const_iterator cend() const noexcept
    {
        return allocated_memory() + my_size;
    }

    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(allocated_memory() + my_size);
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return reverse_iterator(allocated_memory() + my_size);
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(allocated_memory());
    }

    const_reverse_iterator crend() const noexcept
    {
        return reverse_iterator(allocated_memory());
    }


    // capacity

    bool empty() const noexcept
    {
        return !my_size;
    }

    size_type size() const noexcept
    {
        return my_size;
    }

    size_type max_size() const noexcept
    {
        return ptr.max_size();
    }

    size_type capacity() const noexcept
    {
        return my_capacity;
    }

    void resize(size_type sz)
    {
        if (sz == my_size)
            return;
        if (sz > my_capacity)
        {
            mem temp(sz);
            ptr.swap(temp);
            move_block(allocated_memory(), (my_size = temp.size()), temp.data());
        }
        for (; my_size != sz; ++my_size)
            construct(my_size);
    }

    void resize(size_type sz, const T &val) {
        if (sz == my_size)
        {
            for (size_t i = 0; i != my_size; ++i)
            {
                ptr.destroy(i);
                construct(i, val);
            }
            return;
        }

        if (sz > my_capacity)
        {
            ptr.swap(mem(sz));
            for (; my_size != sz; ++my_size)
                construct(my_size, val);
        }
    }
    void reserve(size_type sz) {
        if (my_capacity >= sz)
            return;

        mem temp(sz);
        ptr.swap(temp);
        move_block(allocated_memory(), (my_size = temp.size()), temp.data());

        if (temp)
            temp.deallocate_without_destoy();
    }

    void shrink_to_fit()
    {
        if (my_capacity != my_size)
            resize(my_size);
    }

    // element access
    reference operator [](size_type num)
    {
        return ptr[num];
    }

    const_reference operator [](size_type num) const
    {
        return ptr[num];
    }

    reference at(size_type num)
    {
        if (num <= my_size)
            return *(allocated_memory() + num);
        else
            throw student::out_of_range();
    }

    const_reference at(size_type num) const
    {
        if (num <= my_size)
            return *(allocated_memory() + num);
        else
            throw student::out_of_range();
    }

    reference front()
    {
        return *allocated_memory();
    }

    const_reference front() const {
        return *allocated_memory();
    }

    reference back()
    {
        return *(allocated_memory() + my_size - 1);
    }

    const_reference back() const
    {
        return *(allocated_memory() + my_size - 1);
    }

    // data access
    T * data() noexcept {
        return allocated_memory();
    }

    const T * data() const noexcept {
        return allocated_memory();
    }


    //modifiers

    template <class ... Args> void emplace_back(Args && ... args)
    {
        if (my_size == my_capacity)
            upsize();
        construct(my_size, std::forward<Args>(args)...);
        ++my_size;
    }
    void push_back(const T &val)
    {
        if (my_size == my_capacity)
            upsize();
        construct(my_size, val);
        ++my_size;
    }

    void push_back(T &&val)
    {
        if (my_size == my_capacity)
            upsize();
        construct(my_size, std::move(val));
        ++my_size;
    }

    void pop_back()
    {
        --my_size;
        ptr.destroy(my_size);
    }

    template <class ... Args> iterator emplace(const_iterator it, Args && ... args)
    {
        const difference_type distance = it - allocated_memory();

        if (my_size == my_capacity) {

            size_t new_size;
            if (ptr)
                new_size = ptr.capacity() * 2;
            else
                new_size = 2;

            mem temp(new_size);
            ptr.swap(temp);
            std::swap(ptr.my_size, temp.my_size);

            move_block(allocated_memory(), distance, temp.data());
            move_block(allocated_memory() + distance + 1, my_size - distance, temp.data() + distance);
            temp.deallocate_without_destoy();
        }
        else {
            copy_block(allocated_memory() + distance + 1, my_size - distance, allocated_memory() + distance);
        }
        ++my_size;
        construct(distance, std::forward<Args>(args)...);
        return iterator(allocated_memory() + distance);

    }

    iterator insert(const_iterator it, const T &val)
    {
        return emplace(it, val);
    }

    iterator insert(const_iterator it, T && val)
    {
        return emplace(it, std::move(val));
    }


    iterator insert(const_iterator it, size_type sz, const T& val)
    {
        const difference_type distance = it - allocated_memory();

        if ((my_size + sz) > my_capacity)
        {

            size_t new_size;
            if ((my_capacity * 2) >= sz)
                new_size = my_capacity * 2;
            else
                new_size = my_capacity + sz + 2;

            mem temp(new_size);
            ptr.swap(temp);


            std::uninitialized_copy_n(allocated_memory(), distance, temp.data());

            for (size_type i = 0; i != sz; ++i)
                construct(distance + i, val);

            copy_block(allocated_memory() + distance + sz, my_size - distance, temp.data() + distance);

            my_size += sz;
            temp.deallocate_without_destoy();
        }
        else {
            copy_block(allocated_memory() + distance + sz, my_size - distance, allocated_memory() + distance);
            for (size_type i = 0; i != sz; ++i)
                construct(distance + i, val);
            my_size += sz;
        }
        return iterator(allocated_memory() + distance);
    }


    template <class InputIt, typename = IfConvertableToInputIt<InputIt>>
    iterator insert(const_iterator it, InputIt from, InputIt until)
    {
        const difference_type distance = it - allocated_memory();
        const difference_type size_of_range = std::distance(from, until);
        auto loc_from = from;

        if ((my_size + size_of_range) > my_capacity)
        {
            size_t new_size;

            if ((my_capacity * 2u) >= size_type(size_of_range))
                new_size = my_capacity * 2;
            else
                new_size = my_capacity + size_of_range + 2;

            mem temp(new_size);
            ptr.swap(temp);
            my_size = temp.size();

            move_block(allocated_memory(), distance, temp.data());

            for (auto i = 0; i != size_of_range; ++i, ++loc_from)
                construct(distance + i, *loc_from);

            move_block(allocated_memory() + distance + size_of_range, my_size - distance, temp.data() + distance);
            my_size += size_of_range;

            temp.deallocate_without_destoy();
        }
        else {
            copy_block(allocated_memory() + distance + size_of_range, my_size - distance, allocated_memory() + distance);
            for (auto i = 0; i != size_of_range; ++i, ++loc_from)
                construct(distance + i, *loc_from);
            my_size += size_of_range;
        }
        return iterator(allocated_memory() + distance);
    }

    iterator insert(const_iterator it, std::initializer_list<T> lt)
    {
        return insert(it, lt.begin(), lt.end());
    }


    iterator erase(const_iterator first, const_iterator second)
    {
        auto range = std::distance(first, second);
        auto dist = std::distance(const_iterator(allocated_memory()), first);

        for (auto it = dist; it != (dist + range); ++it)
            ptr.destroy(it);
        copy_block(allocated_memory() + dist, my_size - range - dist, allocated_memory() + dist + range);
        my_size -= range;
        return const_iterator(allocated_memory() + dist);
    }

    iterator erase(const_iterator it)
    {
        return erase(it, std::next(it));
    }

    void swap(vector<T, Alloc> &val) {
        ptr.swap(val.ptr);
    }

    friend void swap(vector<T, Alloc> &first, vector<T, Alloc> &second){
        first.swap(second);
    }

    void clear() noexcept
    {
        ptr.destroy_without_deallocate();
    }

    friend bool operator == (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        bool ans = true;
        if (first.my_size != second.my_size)
            return false;
        for (size_type i = 0; (i != first.my_size) && ans; ++i)
            ans = first[i] == second[i];
        return ans;
    }

    friend bool operator != (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        bool ans = true;
        if (first.my_size != second.my_size)
            return true;
        for (size_type i = 0; (i != first.my_size) && ans; ++i)
            ans = first[i] == second[i];
        return !ans;
    }

    friend bool operator < (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        if (first.my_size > second.my_size)
            return false;
        if (first.my_size < second.my_size)
            return true;
        size_type i = 0;
        for (; (i != first.my_size) && (first[i] == second[i]); ++i);
        return (first[i] < second[i]);
    }

    friend bool operator <= (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        return (first < second) || (first == second);
    }

    friend bool operator > (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        return !(first < second) && (first != second);
    }


    friend bool operator >= (const vector<T, Alloc> &first, const vector<T, Alloc> &second)
    {
        return !(first < second);
    }
};
}


#endif // MY_VECTOR_H
