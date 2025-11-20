//===============================================================//
//                        BINARY TREE                            //
//                                                               //
//                   By Christopher Delay                        //
//                           V1.3                                //
//===============================================================//

#ifndef _included_binary_tree_h
#define _included_binary_tree_h

#include "darray.h"

//=================================================================
// Binary tree object
// Use : A sorted dynamic data structure
// Every data item has a string id which is used for ordering
// Allows very fast data lookups

template <class T>
class BTree
{
  protected:
    BTree* ltree;
    BTree* rtree;

    bool m_caseSensitive;

    void RecursiveConvertToDArray(DArray<T>* darray, BTree<T>* btree);
    void RecursiveConvertIndexToDArray(DArray<char*>* darray, BTree<T>* btree);

    void AppendRight(BTree<T>* tempright);                            // Used by Remove

  public:
    char* id;
    T data;

    BTree();
    BTree(const char* newid, const T& newdata);
    BTree(const BTree<T>& copy);

    ~BTree();
    void Copy(const BTree<T>& copy);

    void PutData(const char* newid, const T& newdata);
    void RemoveData(const char* newid);                     // Requires a solid copy constructor in T class
    T GetData(const char* searchid);

    BTree* LookupTree(const char* searchid);

    void Empty();

    int Size() const;							 // Returns the size in elements

    void Print();                              // Prints this tree to stdout

    BTree* Left() const;
    BTree* Right() const;

    DArray<T>* ConvertToDArray();
    DArray<char*>* ConvertIndexToDArray();
};

//  ===================================================================

template <class T>
BTree<T>::BTree()
{
  ltree = NULL;
  rtree = NULL;
  id = nullptr;
  data = NULL;
}

template <class T>
BTree<T>::BTree(const char* newid, const T& newdata)
{
  ltree = NULL;
  rtree = NULL;
  id = NEW char[strlen(newid) + 1];
  strcpy(id, newid);
  data = newdata;
}

template <class T>
BTree<T>::BTree(const BTree<T>& copy) { Copy(copy); }

template <class T>
void BTree<T>::Copy(const BTree<T>& copy)
{
  if (copy.ltree)
    ltree = NEW BTree(*copy.ltree);
  else
    ltree = NULL;

  if (copy.rtree)
    rtree = NEW BTree(*copy.rtree);
  else
    rtree = NULL;

  if (copy.id)
  {
    id = NEW char[strlen(copy.id) + 1];
    strcpy(id, copy.id);
  }
  else
    id = nullptr;

  data = copy.data;
}

template <class T>
BTree<T>::~BTree() { Empty(); }

template <class T>
void BTree<T>::Empty()
{
  delete ltree;
  delete rtree;
  delete[] id;

  ltree = rtree = NULL;
  id = nullptr;
}

template <class T>
void BTree<T>::PutData(const char* newid, const T& newdata)
{
  if (!id)
  {
    id = NEW char[strlen(newid) + 1];
    strcpy(id, newid);
    data = newdata;

    return;
  }

  if (_stricmp(newid, id) <= 0)
  {
    if (ltree)
      ltree->PutData(newid, newdata);
    else
      ltree = NEW BTree(newid, newdata);
  }
  else if (_stricmp(newid, id) > 0)
  {
    if (rtree)
      rtree->PutData(newid, newdata);
    else
      rtree = NEW BTree(newid, newdata);
  }
}

template <class T>
void BTree<T>::RemoveData(const char* newid)
{
  /*
    Deletes an element from the list
    By replacing the element with it's own left node, then appending
    its own right node onto the extreme right of itself.
    */

  DEBUG_ASSERT(newid);

  if (_stricmp(newid, id) == 0)
  {
    //var tempright : pointer to node := data->right
    BTree<T>* tempright = Right();

    //data := data->left
    if (Left())
    {
      id = NEW char[strlen(Left()->id) + 1];
      strcpy(id, Left()->id);
      data = Left()->data;                  // This bit requires a good copy constructor
      rtree = Left()->Right();
      ltree = Left()->Left();

      AppendRight(tempright);
    }
    else
    {
      //append_right( data, tempright )

      if (Right())
      {
        id = NEW char[strlen(Right()->id) + 1];
        strcpy(id, Right()->id);
        data = Right()->data;                  // This bit requires a good copy constructor
        ltree = Right()->Left();
        rtree = Right()->Right();
      }
      else
        id = nullptr;                              // Hopefully this is the root node
    }
  }                                                   //elsif Name < data->name then
  else if (_stricmp(newid, id) < 0)
  {
    if (Left())
    {
      if (_stricmp(Left()->id, newid) == 0 && !Left()->Left() && !Left()->Right())
        ltree = NULL;
      else
        Left()->RemoveData(newid);
    }
  }
  else                                                //elsif Name > data->name then
  {
    if (Right())
    {
      if (_stricmp(Right()->id, newid) == 0 && !Right()->Left() && !Right()->Right())
        rtree = NULL;
      else
        Right()->RemoveData(newid);
    }
  }
}

template <class T>
T BTree<T>::GetData(const char* searchid)
{
  BTree<T>* btree = LookupTree(searchid);

  if (btree)
    return btree->data;
  return 0;
}

template <class T>
void BTree<T>::AppendRight(BTree<T>* tempright)
{
  if (!Right())
    rtree = tempright;

  else
    Right()->AppendRight(tempright);
}

template <class T>
BTree<T>* BTree<T>::LookupTree(const char* searchid)
{
  if (!id)
    return nullptr;

  if (_stricmp(searchid, id) == 0)
    return this;
  if (ltree && _stricmp(searchid, id) < 0)
    return ltree->LookupTree(searchid);
  if (rtree && _stricmp(searchid, id) > 0)
    return rtree->LookupTree(searchid);
  return nullptr;
}

template <class T>
int BTree<T>::Size() const
{
  unsigned int subsize = (id) ? 1 : 0;

  if (ltree)
    subsize += ltree->Size();
  if (rtree)
    subsize += rtree->Size();

  return subsize;
}

template <class T>
BTree<T>* BTree<T>::Left() const { return static_cast<BTree<T>*>(ltree); }

template <class T>
BTree<T>* BTree<T>::Right() const { return static_cast<BTree<T>*>(rtree); }

template <class T>
DArray<T>* BTree<T>::ConvertToDArray()
{
  DArray<T>* darray = NEW DArray<T>;
  RecursiveConvertToDArray(darray, this);

  return darray;
}

template <class T>
DArray<char*>* BTree<T>::ConvertIndexToDArray()
{
  auto darray = NEW DArray<char*>;
  RecursiveConvertIndexToDArray(darray, this);

  return darray;
}

template <class T>
void BTree<T>::RecursiveConvertToDArray(DArray<T>* darray, BTree<T>* btree)
{
  DEBUG_ASSERT(darray);

  if (!btree)
    return;            // Base case

  if (btree->id)
    darray->PutData(btree->data);

  RecursiveConvertToDArray(darray, btree->Left());
  RecursiveConvertToDArray(darray, btree->Right());
}

template <class T>
void BTree<T>::RecursiveConvertIndexToDArray(DArray<char*>* darray, BTree<T>* btree)
{
  DEBUG_ASSERT(darray);

  if (!btree)
    return;            // Base case

  if (btree->id)
    darray->PutData(btree->id);

  RecursiveConvertIndexToDArray(darray, btree->Left());
  RecursiveConvertIndexToDArray(darray, btree->Right());
}

#endif
