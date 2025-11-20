//===============================================================//
//                        F D A R R A Y                          //
//                                                               //
//                   By Christopher Delay                        //
//                           V1.3                                //
//===============================================================//

#ifndef _included_fdarray_h
#define _included_fdarray_h

#include "darray.h"

//=================================================================
// Fast Dynamic array object
// Use : A dynamically sized list of data
// Which can be indexed into - an entry's index never changes
// Same as DArray, but has new advantages: Insert is MUCH faster

template <class T>
class FastDArray : public DArray<T>
{
  protected:
    int numused;
    int* freelist;
    int firstfree;

    void RebuildFreeList();									// SLOW
    void RebuildNumUsed();									// SLOW
    void Grow();									// SLOW

  public:
    FastDArray();
    FastDArray(int newstepsize);
    ~FastDArray();

    void SetSize(int newsize);					// SLOW

    int PutData(const T& newdata);				// FAST Returns index used
    void PutData(const T& newdata, int index);	// SLOW

    void MarkUsed(int index);						// SLOW
    void MarkNotUsed(int index);					// FAST

    T* GetPointer();                                 // FAST Returns next free element, sets to 'used'
    T* GetPointer(int index);                        // FAST Returns next free element, sets to 'used'
    int GetNextFree();								// FAST Sets the returned index to 'used'

    int NumUsed() const;								// FAST Returns the number of used entries

    void Empty();											// FAST Resets the array to empty
    void EmptyAndDelete();                                  // FAST
};

//  ===================================================================

template <class T>
FastDArray<T>::FastDArray()
  : DArray<T>()
{
  numused = 0;
  freelist = nullptr;
  firstfree = -1;
}

template <class T>
FastDArray<T>::FastDArray(int newstepsize)
  : DArray<T>(newstepsize)
{
  numused = 0;
  freelist = nullptr;
  firstfree = -1;
}

template <class T>
FastDArray<T>::~FastDArray()
{
  Empty();
  // user must call EmptyAndDelete() by hand when T is pointer,
  // otherwise memory leaks
}

template <class T>
void FastDArray<T>::RebuildFreeList()
{
  //  Reset free list

  delete freelist;
  freelist = NEW int[this->m_arraySize];
  firstfree = -1;
  int lastknownfree = -1;

  // Step through, rebuilding

  for (int i = 0; i < this->m_arraySize; ++i)
  {
    if (this->shadow[i] == 0)
    {
      if (firstfree == -1)
        firstfree = i;
      else
      {
        if (lastknownfree != -1)
          freelist[lastknownfree] = i;
      }

      freelist[i] = -1;
      lastknownfree = i;
    }
    else
      freelist[i] = -2;
  }
}

template <class T>
void FastDArray<T>::RebuildNumUsed()
{
  numused = 0;
  for (int i = 0; i < this->m_arraySize; ++i)
  {
    if (this->shadow[i] == 1)
      ++numused;
  }
}

template <class T>
void FastDArray<T>::SetSize(int newsize)
{
  if (newsize > this->m_arraySize)
  {
    int oldarraysize = this->m_arraySize;

    this->m_arraySize = newsize;
    T* temparray = NEW T[this->m_arraySize];
    char* tempshadow = NEW char[this->m_arraySize];
    int* tempfreelist = NEW int[this->m_arraySize];

    int a;

    for (a = 0; a < oldarraysize; ++a)
    {
      temparray[a] = this->array[a];
      tempshadow[a] = this->shadow[a];
      tempfreelist[a] = freelist[a];
    }

    for (a = oldarraysize; a < this->m_arraySize; ++a)
      tempshadow[a] = 0;

    int oldfirstfree = firstfree;
    firstfree = oldarraysize;
    for (a = oldarraysize; a < this->m_arraySize - 1; ++a)
      tempfreelist[a] = a + 1;
    tempfreelist[this->m_arraySize - 1] = oldfirstfree;

    delete[] this->array;
    delete[] this->shadow;
    delete[] freelist;

    this->array = temparray;
    this->shadow = tempshadow;
    freelist = tempfreelist;
  }
  else if (newsize < this->m_arraySize)
  {
    this->m_arraySize = newsize;
    T* temparray = NEW T[this->m_arraySize];
    char* tempshadow = NEW char[this->m_arraySize];
    int* tempfreelist = NEW int[this->m_arraySize];

    for (int a = 0; a < this->m_arraySize; ++a)
    {
      temparray[a] = this->array[a];
      tempshadow[a] = this->shadow[a];
      tempfreelist[a] = freelist[a];
    }

    delete[] this->array;
    delete[] this->shadow;
    delete[] freelist;

    this->array = temparray;
    this->shadow = tempshadow;
    freelist = tempfreelist;

    RebuildNumUsed();
    RebuildFreeList();
  }
  else if (newsize == this->m_arraySize)
  {
    // Do nothing
  }
}

template <class T>
void FastDArray<T>::Grow()
{
  if (this->m_stepSize == -1)
  {
    if (this->m_arraySize == 0)
      SetSize(1);
    else
    {
      // Double array size
      SetSize(this->m_arraySize * 2);
    }
  }
  else
  {
    // Increase array size by fixed amount
    SetSize(this->m_arraySize + this->m_stepSize);
  }
}

template <class T>
int FastDArray<T>::PutData(const T& newdata)
{
  if (firstfree == -1)
  {
    // Must resize the array
    Grow();
  }

  DEBUG_ASSERT(firstfree != -1);
  if (firstfree == -1)
  {
    // Must resize the array
    Grow();
  }

  int freeslot = firstfree;
  int nextfree = freelist[freeslot];

  this->array[freeslot] = newdata;
  if (this->shadow[freeslot] == 0)
    ++numused;
  this->shadow[freeslot] = 1;
  freelist[freeslot] = -2;
  firstfree = nextfree;

  return freeslot;
}

template <class T>
void FastDArray<T>::PutData(const T& newdata, int index)
{
  DEBUG_ASSERT(index < this->m_arraySize && index >= 0);

  this->array[index] = newdata;

  if (this->shadow[index] == 0)
  {
    this->shadow[index] = 1;
    ++numused;
    RebuildFreeList();
  }
}

template <class T>
void FastDArray<T>::EmptyAndDelete()
{
  delete[] freelist;
  freelist = nullptr;

  firstfree = -1;
  numused = 0;

  DArray<T>::EmptyAndDelete();
}

template <class T>
void FastDArray<T>::Empty()
{
  delete[] freelist;
  freelist = nullptr;

  firstfree = -1;
  numused = 0;

  DArray<T>::Empty();
}

template <class T>
void FastDArray<T>::MarkUsed(int index)
{
  DEBUG_ASSERT(index < this->m_arraySize && index >= 0);
  DEBUG_ASSERT(this->shadow[index] == 0);

  this->shadow[index] = 1;
  ++numused;
  RebuildFreeList();
}

template <class T>
void FastDArray<T>::MarkNotUsed(int index)
{
  DEBUG_ASSERT(index < this->m_arraySize && index >= 0);
  DEBUG_ASSERT(this->shadow[index] != 0);

  --numused;
  this->shadow[index] = 0;
  freelist[index] = firstfree;
  firstfree = index;
}

template <class T>
T* FastDArray<T>::GetPointer() { return GetPointer(GetNextFree()); }

template <class T>
T* FastDArray<T>::GetPointer(int index) { return DArray<T>::GetPointer(index); }

template <class T>
int FastDArray<T>::GetNextFree()
{
  if (firstfree == -1)
  {
    // Must resize the array
    Grow();
  }

  int freeslot = firstfree;
  int nextfree = freelist[freeslot];

  if (this->shadow[freeslot] == 0)
    ++numused;

  this->shadow[freeslot] = 1;
  freelist[freeslot] = -2;
  firstfree = nextfree;

  return freeslot;
}

template <class T>
int FastDArray<T>::NumUsed() const { return numused; }

#endif
