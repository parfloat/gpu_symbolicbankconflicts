#include <stdio.h>

class UVector
{
public:
   UVector();
   UVector(unsigned size);
   UVector(unsigned size, const unsigned & initial);
   UVector(const UVector & v);           
   ~UVector();
   unsigned capacity() const;         
   unsigned size() const;             
   bool empty() const;
   unsigned begin();                      
   unsigned end();                        
   unsigned & front();                           
   unsigned & back();                            
   void push_back(const unsigned & value);       
   void pop_back();                       
   void reserve(unsigned capacity);   
   void resize(unsigned size);        
   unsigned & operator[](unsigned index);    
   UVector & operator=(const UVector &);
   
   unsigned* buffer;
private:
   unsigned vector_size;
   unsigned vector_capacity;
   
};

void copy_UVector(UVector &target, unsigned start, unsigned end, UVector &destination, unsigned pos){
    unsigned cnt = 0; 
    for (int i = start; i < end; i++, cnt++)
        destination.buffer[pos + cnt] = target.buffer[i];
}

void rotate_UVector(UVector &target, unsigned start, unsigned middle, unsigned end){
  // ugly hack to avoid umul.with.overflow when "new unsigned [target.size()]".
  unsigned * new_buffer = (unsigned *) (new char[target.size()*4]); 
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

// template <class UnaryOperator>
// void transform_UVector(UVector &first, unsigned start1, unsigned end1, UVector &result, unsigned result_start, UnaryOperator operation){
//    while (start1 != end1) {
//     result[result_start] = operation(first[start1]);
//     ++start1; ++result_start;
//   }
// }

// template <class BinaryOperator>
// void transform_UVector(UVector &first, unsigned start1, unsigned end1, UVector &second, unsigned start2, UVector &result, unsigned result_start, BinaryOperator operation){
//    while (start1 != end1) {
//     result[result_start] = operation(first[start1], second[start2]);
//     ++start1; ++start2; ++result_start;
//   }
// }




UVector::UVector()
{
    vector_capacity = 0;
    vector_size = 0;
    buffer = 0;
}

UVector::UVector(const UVector & v)
{
    vector_size = v.vector_size;
    vector_capacity = v.vector_capacity;
    buffer = (unsigned*)(new char[vector_size*4]); 
    for (int i = 0; i < vector_size; i++)
        buffer[i] = v.buffer[i];  
}

UVector::UVector(unsigned size)
{
    vector_capacity = size;
    vector_size = size;
    buffer = (unsigned*) (new char[size*4]);
}

UVector::UVector(unsigned size, const unsigned & initial)
{
    vector_size = size;
    vector_capacity = size;
    buffer = (unsigned*)(new char[size*4]);
    for (int i = 0; i < size; i++)
        buffer[i] = initial;
}

UVector & UVector::operator = (const UVector & v)
{
    delete[ ] buffer;
    vector_size = v.vector_size;
    vector_capacity = v.vector_capacity;
    buffer = (unsigned*)(new char [vector_size*4]);
    for (int i = 0; i < vector_size; i++)
        buffer[i] = v.buffer[i];
    return *this;
}

unsigned UVector::begin()
{
    return 0;
}

unsigned UVector::end()
{
    return size();
}

unsigned& UVector::front()
{
    return buffer[begin()];
}

unsigned& UVector::back()
{
    return buffer[size() - 1];
}

void UVector::push_back(const unsigned & v)
{
    if (vector_size >= vector_capacity)
	reserve(vector_capacity +5);
    buffer [vector_size++] = v;
}

void UVector::pop_back()
{
    vector_size--;
}

void UVector::reserve(unsigned capacity)
{
    if(buffer == 0)
    {
        vector_size = 0;
        vector_capacity = 0;
    }    
    if (capacity <= vector_capacity)
	return;
    unsigned * new_buffer = (unsigned*) (new char[capacity*4]);
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

unsigned UVector::size()const
{
    return vector_size;
}

void UVector::resize(unsigned size)
{
    reserve(size);
    vector_size = size;
}

unsigned& UVector::operator[](unsigned index)
{
    return buffer[index];
}  

unsigned UVector::capacity()const
{
    return vector_capacity;
}

UVector::~UVector()
{
    delete[] buffer;
}
