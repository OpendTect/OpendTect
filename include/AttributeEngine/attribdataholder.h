#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.1 2005-01-28 16:30:41 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Attrib
{

class SingleDataHolder
{
public:
    virtual		~SingleDataHolder() {}
    virtual float	operator[](int) const			= 0;
};


class DataHolder : public ObjectSet<SingleDataHolder>
{
public:
    			DataHolder( int t1_, int nrsamples_ )
			    : t1( t1_ ), nrsamples( nrsamples )
			{ allowNull(true); }
			~DataHolder() { deepErase(*this); }
    int			t1;
    int			nrsamples;
};


template <class T>
class SingleDataHolderPtrImpl : public SingleDataHolder
{
public:
    		~SingleDataHolderPtrImpl() { delete [] ptr; }
    		SingleDataHolderPtrImpl( T* ptr_ ) : ptr( ptr_ ) {}
    float	operator[](int idx) const { return ptr[idx]; }
    T*		ptr;
};



}; //Namespace


#endif
