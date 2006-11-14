#ifndef convmemvalseries_h
#define convmemvalseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kris Tingdahl
 Date:          Oct 2006
 RCS:           $Id: convmemvalseries.h,v 1.2 2006-11-14 12:38:21 cvskris Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "datainterp.h"
#include "bindatadesc.h"


/*!ValueSeries that holds data in memory, but the memory may be of a different
format than T. I.e. a ValueSeries<float> can have it's values stored as 
chars. */


template <class T>
class ConvMemValueSeries : public ValueSeries<T>
{
public:

    inline	    	ConvMemValueSeries(int sz, const BinDataDesc& stortype);

    inline		~ConvMemValueSeries();

    inline int		size() const;
    inline bool		writable() const;
    inline T		value(int idx) const;
    inline void		setValue( int idx, T v );

    inline const T*	arr() const;
    inline T*		arr();

    inline char*	storArr();
    inline const char*	storArr() const;

    inline BinDataDesc	dataDesc() const;

protected:

    DataInterpreter<T>	interpreter_;
    BinDataDesc		rettype_;

    char*		ptr_;
    int			size_;
};


template <class T> inline
ConvMemValueSeries<T>::ConvMemValueSeries( int sz, const BinDataDesc& stortype)
    : ptr_( 0 )
    , size_( sz )
    , interpreter_( DataCharacteristics(stortype) )
{
    T dummy;
    rettype_ = BinDataDesc( dummy );
    ptr_ = new char[sz*interpreter_.nrBytes()];
}


template <class T> inline
ConvMemValueSeries<T>::~ConvMemValueSeries()
{ delete [] ptr_; }


template <class T> inline
T ConvMemValueSeries<T>::value(int idx) const
{ return interpreter_.get( ptr_, idx ); }


template <class T> inline
bool ConvMemValueSeries<T>::writable() const
{ return true; }


template <class T> inline
int ConvMemValueSeries<T>::size() const
{ return size_; }


template <class T> inline
void ConvMemValueSeries<T>::setValue( int idx, T v )
{ interpreter_.put( ptr_, idx, v ); }


template <class T> inline
const T* ConvMemValueSeries<T>::arr() const
{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }


template <class T> inline
T* ConvMemValueSeries<T>::arr()
{ return interpreter_.dataChar()==rettype_ ? (T*) ptr_ : 0; }


template <class T> inline
char* ConvMemValueSeries<T>::storArr()
{ return ptr_; }


template <class T> inline
const char* ConvMemValueSeries<T>::storArr() const
{ return ptr_; }


template <class T> inline
BinDataDesc ConvMemValueSeries<T>::dataDesc() const
{ return interpreter_.dataChar(); }


#endif
