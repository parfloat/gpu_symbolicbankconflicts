#include <stdio.h>

template <class T>
class Vector
{
public:
   Vector();
   Vector(unsigned size);
   Vector(unsigned size, const T & initial);
   Vector(const Vector<T> & v);           
   ~Vector();
   unsigned capacity() const;         
   unsigned size() const;             
   bool empty() const;
   unsigned begin();                      
   unsigned end();                        
   T & front();                           
   T & back();                            
   void push_back(const T & value);       
   void pop_back();                       
   void reserve(unsigned capacity);   
   void resize(unsigned size);        
   T & operator[](unsigned index);    
   Vector<T> & operator=(const Vector<T> &);
   
   
   T* buffer;
private:
   unsigned vector_size;
   unsigned vector_capacity;
   
};

template <class T>
void copy_Vector(Vector<T> &target, unsigned start, unsigned end, Vector<T> &destination, unsigned pos){
    unsigned cnt = 0; 
    for (int i = start; i < end; i++, cnt++)
        destination.buffer[pos + cnt] = target.buffer[i];
}

template <class T>
void rotate_Vector(Vector<T> &target, unsigned start, unsigned middle, unsigned end){
   T * new_buffer = new T[target.size()];  
   if (!new_buffer){
        printf("Eror allocating new buffer in rotate\n");
        exit(-1);
        }
    
   unsigned pos = 0;
    for (int i = middle; i < end; i++, pos++)
        new_buffer[pos] = target.buffer[i];
    
    for (int i = start; i < middle; i++, pos++)
        new_buffer[pos] = target.buffer[i];
    
    //delete[] target.buffer;
    pos = 0;
    for (int i = start; i < end; i++, pos++)
        target.buffer[i] = new_buffer[pos];
    delete[] new_buffer;
}

template <class T, class UnaryOperator>
void transform_Vector(Vector<T> &first, unsigned start1, unsigned end1, Vector<T> &result, unsigned result_start, UnaryOperator operation){
   while (start1 != end1) {
    result[result_start] = operation(first[start1]);
    ++start1; ++result_start;
  }
}

template <class T, class BinaryOperator>
void transform_Vector(Vector<T> &first, unsigned start1, unsigned end1, Vector<T> &second, unsigned start2, Vector<T> &result, unsigned result_start, BinaryOperator operation){
   while (start1 != end1) {
    result[result_start] = operation(first[start1], second[start2]);
    ++start1; ++start2; ++result_start;
  }
}




template<class T>
Vector<T>::Vector()
{
    vector_capacity = 0;
    vector_size = 0;
    buffer = 0;
}

template<class T>
Vector<T>::Vector(const Vector<T> & v)
{
    vector_size = v.vector_size;
    vector_capacity = v.vector_capacity;
    buffer = new T[vector_size]; 
    for (int i = 0; i < vector_size; i++)
        buffer[i] = v.buffer[i];  
}

template<class T>
Vector<T>::Vector(unsigned size)
{
    vector_capacity = size;
    vector_size = size;
    buffer = new T[size];
}

template<class T>
Vector<T>::Vector(unsigned size, const T & initial)
{
    vector_size = size;
    vector_capacity = size;
    buffer = new T [size];
    for (int i = 0; i < size; i++)
        buffer[i] = initial;
}

template<class T>
Vector<T> & Vector<T>::operator = (const Vector<T> & v)
{
    delete[ ] buffer;
    vector_size = v.vector_size;
    vector_capacity = v.vector_capacity;
    buffer = new T [vector_size];
    for (int i = 0; i < vector_size; i++)
        buffer[i] = v.buffer[i];
    return *this;
}

template<class T>
unsigned Vector<T>::begin()
{
    return 0;
}

template<class T>
unsigned Vector<T>::end()
{
    return size();
}

template<class T>
T& Vector<T>::front()
{
    return buffer[begin()];
}

template<class T>
T& Vector<T>::back()
{
    return buffer[size() - 1];
}

template<class T>
void Vector<T>::push_back(const T & v)
{
    if (vector_size >= vector_capacity)
	reserve(vector_capacity +5);
    buffer [vector_size++] = v;
}

template<class T>
void Vector<T>::pop_back()
{
    vector_size--;
}

template<class T>
void Vector<T>::reserve(unsigned capacity)
{
    if(buffer == 0)
    {
        vector_size = 0;
        vector_capacity = 0;
    }    
    if (capacity <= vector_capacity)
	return;
    T * new_buffer = new T [capacity];
    if (!new_buffer){
        printf("Eror allocating new buffer in reserve\n");
        exit(-1);
        }
    for (int i = 0; i < vector_size; i++)
        new_buffer[i] = buffer[i];
    vector_capacity = capacity;
    delete[] buffer;
    buffer = new_buffer;
    
}

template<class T>
unsigned Vector<T>::size()const
{
    return vector_size;
}

template<class T>
void Vector<T>::resize(unsigned size)
{
    reserve(size);
    vector_size = size;
}

template<class T>
T& Vector<T>::operator[](unsigned index)
{
    return buffer[index];
}  

template<class T>
unsigned Vector<T>::capacity()const
{
    return vector_capacity;
}

template<class T>
Vector<T>::~Vector()
{
    delete[] buffer;
}
