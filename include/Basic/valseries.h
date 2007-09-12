#ifndef valseries_h
#define valseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseries.h,v 1.12 2007-09-12 10:23:06 cvskris Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "errh.h"

/*\brief Interface to a series of values

  If the values are in contiguous memory, arr() should return non-null.
 
 */

template <class T>
class ValueSeries
{
public:

    virtual		~ValueSeries()		{}

    virtual bool	isOK() const			{ return true; }

    virtual T		value(int64) const		= 0;
    virtual bool	writable() const		{ return false; }
    virtual void	setValue(int64,T)		{}

    virtual bool	reSizeable() const		{ return false; }
    virtual bool	setSize(int64) 			{ return false; }

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    inline T		operator[]( int64 idx ) const	{ return value(idx); }

};


template <class T>
class OffsetValueSeries : public ValueSeries<T>
{
public:
    inline		OffsetValueSeries( ValueSeries<T>& src, int64 off );
    inline		OffsetValueSeries( const ValueSeries<T>& src,int64 off);

    inline T		value( int64 idx ) const;
    inline void		setValue( int64 idx, T v ) const;
    inline T*		arr();
    inline const T*	arr() const;
    inline bool		writable() const;

    inline int64	getOffset() const;
    inline void		setOffset(int64 no);

    const ValueSeries<T>&	source() const { return src_; }

protected:
    ValueSeries<T>&	src_;
    int64		off_;
    bool		writable_;
};


#define mImplArr \
{ return typeid(RT)==typeid(AT) ? (RT*) ptr_ : (RT*) 0;}

/*\brief series of values from a pointer to some kind of array. If a more
         advanced conversion between the return type and the array type is
	 wanted, use ConvValueSeries instead. */

template <class RT, class AT>
class ArrayValueSeries : public ValueSeries<RT>
{
public:

    		ArrayValueSeries( AT* ptr, bool memmine, int64 sz=-1 );
    		ArrayValueSeries( int64 sz );
    		~ArrayValueSeries()		{ if ( mine_ ) delete [] ptr_; }

    bool	isOK() const			{ return ptr_; }

    RT		value( int64 idx ) const	{ return ptr_[idx]; }
    bool	writable() const		{ return true; }
    void	setValue( int64 idx, RT v )	{ ptr_[idx] = (AT) v; }

    const RT*	arr() const			mImplArr;
    RT*		arr()				mImplArr;

    bool	reSizeable() const		{ return mine_; }
    inline bool	setSize(int64);
    int64	size() const			{ return cursize_; }

protected:

    AT*		ptr_;
    bool	mine_;
    int64	cursize_;
};

#undef mImplArr



template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( ValueSeries<T>& src, int64 off )
    : src_( src ), off_( off ), writable_(true) 
{}


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( const ValueSeries<T>& src, int64 off )
    : src_( const_cast<ValueSeries<T>& >(src) ), off_( off ), writable_(false) 
{}


template <class T> inline
T OffsetValueSeries<T>::value( int64 idx ) const
{ return src_.value(idx+off_); }

template <class T> inline
void OffsetValueSeries<T>::setValue( int64 idx, T v ) const
{
    if ( writable_ )
	src_.setValue(idx+off_,v);
    else
	pErrMsg("Attempting to write to write-protected array");
}


template <class T> inline
T* OffsetValueSeries<T>::arr()
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
const T* OffsetValueSeries<T>::arr() const
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
int64 OffsetValueSeries<T>::getOffset() const
{ return off_; }


template <class T> inline
void OffsetValueSeries<T>::setOffset(int64 no)
{ off_ = no; }


template <class T> inline
bool OffsetValueSeries<T>::writable() const
{ return writable_; }


template <class RT, class AT>
ArrayValueSeries<RT,AT>::ArrayValueSeries( AT* ptr, bool memmine, int64 cursz )
    : ptr_(ptr), mine_(memmine), cursize_( cursz )
{}


template <class RT, class AT>
ArrayValueSeries<RT,AT>::ArrayValueSeries( int64 sz )
    : mine_(true), cursize_( sz )
{
    mTryAlloc( ptr_, AT[sz] )
}


template <class RT,class AT>
bool ArrayValueSeries<RT,AT>::setSize( int64 sz )
{
    if ( cursize_!=-1 && cursize_==sz && ptr_ ) return true;
    if ( !mine_ ) return false;

    delete [] ptr_;
    mTryAlloc( ptr_, AT[sz] )
    cursize_ = sz;
    return ptr_;
}


#endif
