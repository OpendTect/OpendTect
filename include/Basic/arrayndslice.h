#ifndef arrayndslice_h
#define arrayndslice_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id$
________________________________________________________________________

@$*/

#include "basicmod.h"
#include "arraynd.h"
#include "varlenarray.h"

mClass(Basic) ArrayNDSliceBase
{
public:				
    virtual			~ArrayNDSliceBase();
    int				getDimSize(int dim) const;
    int				getPos(int dim) const;
    bool			setPos(int dim,int pos);
    bool			init();
    void			setDimMap(int localdim, int remotedim );
protected:
    				ArrayNDSliceBase( ArrayNDInfo*,
						  const ArrayNDInfo& );
    void			getSourcePos(const int* localpos,
	    				     int* sourcepos) const;
    ArrayNDInfo&		info_;
    const ArrayNDInfo&		sourceinfo_;

    TypeSet<int>		vardim_;
    TypeSet<int>		position_;
    od_int64			offset_;
};


/*! \brief access-tool to another ArrayND with higher dimensionality.

ArrayXDSlice is an ArrayND that is an access-tool to another ArrayND with
higher dimensionality. It can be used to get Array1D through a Array3D cube.
Use setPos(int,int) to set the fixed positions and leave out the positions that should vary. When all positions are set, call init(). 

To unset a position, set it to -1. If positions are unset, init has to be
called prior to dataaccesing functions.
*/
template <class T>
class Array1DSlice : public Array1D<T>, public ArrayNDSliceBase
{
public:
				Array1DSlice(ArrayND<T>&);
				Array1DSlice(const ArrayND<T>&);
				~Array1DSlice();

    ValueSeries<T>*		clone() const;

    T				get( int ) const;
    void			set( int, T );
    const Array1DInfo&		info() const;
    bool			isSettable() const;

protected:
    const ValueSeries<T>*	getStorage_() const;

    bool				writable_;
    ArrayND<T>&				source_;
    mutable OffsetValueSeries<T>*	storage_;
};


/*! \brief access-tool to another ArrayND with higher dimensionality.

ArrayXDSlice is an ArrayND that is an access-tool to another ArrayND with
higher dimensionality. It can be used to get Array1D through a Array3D cube.
Use setPos(int,int) to set the fixed positions and leave out the positions that should vary. When all positions are set, call init(). 

To unset a position, set it to -1. If positions are unset, init has to be
called prior to dataaccesing functions.
*/
template <class T>
class Array2DSlice : public Array2D<T>, public ArrayNDSliceBase
{
public:
				Array2DSlice(ArrayND<T>&);
				Array2DSlice(const ArrayND<T>&);
				~Array2DSlice();

    T				get(int,int) const;
    void			set(int,int, T );
    const Array2DInfo&		info() const;
    bool			isSettable() const;

protected:
    const ValueSeries<T>*	getStorage_() const;
    bool			writable_;

    ArrayND<T>&				source_;
    mutable OffsetValueSeries<T>*	storage_;
};


//Array1DSlice
template <class T> inline
Array1DSlice<T>::Array1DSlice( ArrayND<T>& source )
    : ArrayNDSliceBase( new Array1DInfoImpl, source.info() )
    , source_( source )
    , storage_( 0 )
    , writable_( true )
{}


template <class T> inline
Array1DSlice<T>::Array1DSlice( const ArrayND<T>& source )
    : ArrayNDSliceBase( new Array1DInfoImpl, source.info() )
    , source_( const_cast<ArrayND<T>& >(source) )
    , storage_( 0 )
    , writable_( false )
{}


template <class T> inline
Array1DSlice<T>::~Array1DSlice()
{ delete storage_; }


template <class T> inline
bool Array1DSlice<T>::isSettable() const
{ return writable_ && source_.isSettable(); }


template <class T> inline
void Array1DSlice<T>::set( int pos, T val )
{
    if ( !writable_ ) return;
    mAllocVarLenArr( int, srcpos, position_.size() );
    getSourcePos( &pos, srcpos );
    source_.setND( srcpos, val );
}


template <class T> inline
T Array1DSlice<T>::get( int pos ) const
{
    mAllocVarLenArr( int, srcpos, position_.size() );
    getSourcePos( &pos, srcpos );
    return source_.getND( srcpos );
}


template <class T> inline
ValueSeries<T>* Array1DSlice<T>::clone() const
{
    Array1DSlice<T>* res = new Array1DSlice<T>( source_ );
    res->info_ = info_;
    res->vardim_ = vardim_;
    res->position_ = position_;
    res->offset_ = offset_;

    return res;
}

template <class T> inline
const Array1DInfo& Array1DSlice<T>::info() const	
{ return (const Array1DInfo&) info_; }


template <class T> inline
const ValueSeries<T>* Array1DSlice<T>::getStorage_() const
{
    if ( offset_<0 )
	return 0;

    if ( !source_.getStorage() )
	return 0;

    if ( offset_==0 )
    {
	delete storage_;
	storage_ = 0;
	return source_.getStorage();
    }

    if ( !storage_ || &storage_->source() != source_.getStorage() )
    {
	delete storage_;
	storage_ =
	    new OffsetValueSeries<T>( *source_.getStorage(), offset_ );
    }
    else
	storage_->setOffset( offset_ );
    return storage_;
}


//Array2DSlice
template <class T> inline
Array2DSlice<T>::Array2DSlice( ArrayND<T>& source )
    : ArrayNDSliceBase( new Array2DInfoImpl, source.info() )
    , source_( source )
    , storage_( 0 )
    , writable_( true )
{}


template <class T> inline
Array2DSlice<T>::Array2DSlice( const ArrayND<T>& source )
    : ArrayNDSliceBase( new Array2DInfoImpl, source.info() )
    , source_( const_cast<ArrayND<T>&>(source) )
    , storage_( 0 )
    , writable_( false )
{}


template <class T> inline
Array2DSlice<T>::~Array2DSlice()
{ delete storage_; }


template <class T> inline
bool Array2DSlice<T>::isSettable() const
{ return writable_ && source_.isSettable(); }


template <class T> inline
void Array2DSlice<T>::set( int pos0, int pos1, T val )
{
    if ( !writable_ ) return;

    const int localpos[] = { pos0, pos1 };
    mAllocVarLenArr( int, srcpos, position_.size() );
    getSourcePos( localpos, srcpos );
    source_.setND( srcpos, val );
}


template <class T> inline
T Array2DSlice<T>::get( int pos0, int pos1 ) const
{
    const int localpos[] = { pos0, pos1 };
    mAllocVarLenArr( int, srcpos, position_.size() );
    getSourcePos( localpos, srcpos );
    return source_.getND( srcpos );
}


template <class T> inline
const Array2DInfo& Array2DSlice<T>::info() const	
{ return (const Array2DInfo&) info_; }


template <class T> inline
const ValueSeries<T>* Array2DSlice<T>::getStorage_() const
{
    if ( offset_<0 )
	return 0;

    if ( !source_.getStorage() )
	return 0;

    if ( offset_==0 )
    {
	delete storage_;
	storage_ = 0;
	return source_.getStorage();
    }

    if ( !storage_ || &storage_->source() != source_.getStorage() )
    {
	delete storage_;
	storage_ =
	    new OffsetValueSeries<T>( *source_.getStorage(), offset_ );
    }
    else
	storage_->setOffset( offset_ );

    return storage_;
}



#endif

