#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
________________________________________________________________________

@$*/

#include "basicmod.h"
#include "arraynd.h"
#include "varlenarray.h"
#include "valseriesimpl.h"

/*!\brief Base class of Array1DSlice and Array2DSlice. Access-tool to another
ArrayND with higher dimensionality.

  ArrayXDSlice is an ArrayND that is an access-tool to another ArrayND with
  higher dimensionality. It can be used to get Array1D through a Array3D cube.
  Use setPos() to set the fixed positions and leave out the positions
  that should vary. When all positions are set, call init().

  To unset a position, set it to -1. If positions are unset, init has to be
  called prior to dataaccesing functions.
*/

mExpClass(Basic) ArrayNDSliceBase
{
public:
				mTypeDefArrNDTypes;

    virtual			~ArrayNDSliceBase();

    size_type			getDimSize(dim_idx_type) const;
    idx_type			getPos(dim_idx_type) const;
    bool			setPos(dim_idx_type,idx_type);
    bool			init();
    void			setDimMap(dim_idx_type localdim,
					  dim_idx_type remotedim);

protected:
				ArrayNDSliceBase(ArrayNDInfo*,
						 const ArrayNDInfo&);
    void			getSourcePos(NDPos localpos,
					     idx_type* sourcepos) const;
    ArrayNDInfo&		info_;
    const ArrayNDInfo&		sourceinfo_;

    TypeSet<dim_idx_type>		vardim_;
    TypeSet<idx_type>		position_;
    offset_type			offset_;
    bool			isinited_;
};


/*!
\brief Subclass of Array1D and ArrayNDSliceBase.
*/

template <class T>
mClass(Basic) Array1DSlice : public Array1D<T>, public ArrayNDSliceBase
{
public:
				mTypeDefArrNDTypes;

				Array1DSlice(ArrayND<T>&);
				Array1DSlice(const ArrayND<T>&);
				~Array1DSlice();

    ValueSeries<T>*		clone() const;

    T				get(idx_type) const;
    void			set(idx_type,T);
    const Array1DInfo&		info() const;
    bool			isSettable() const;

protected:

    const ValueSeries<T>*	getStorage_() const;

    bool			writable_;
    ArrayND<T>&			source_;
    mutable OffsetValueSeries<T>* storage_;

};


/*!
\brief Subclass of Array2D and ArrayNDSliceBase.
*/

template <class T>
mClass(Basic) Array2DSlice : public Array2D<T>, public ArrayNDSliceBase
{
public:
				mTypeDefArrNDTypes;

				Array2DSlice(ArrayND<T>&);
				Array2DSlice(const ArrayND<T>&);
				~Array2DSlice();

    T				get(idx_type,idx_type) const;
    void			set(idx_type,idx_type,T);
    const Array2DInfo&		info() const;
    bool			isSettable() const;

protected:

    const ValueSeries<T>*	getStorage_() const;
    bool			writable_;

    ArrayND<T>&			source_;
    mutable OffsetValueSeries<T>* storage_;

};


/*!
\brief Subclass of Array3D and ArrayNDSliceBase.
*/

template <class T>
mClass(Basic) Array3DSlice : public Array3D<T>, public ArrayNDSliceBase
{
public:
				mTypeDefArrNDTypes;

				Array3DSlice(ArrayND<T>&);
				Array3DSlice(const ArrayND<T>&);
				~Array3DSlice();

    T				get(idx_type,idx_type,idx_type) const;
    void			set(idx_type,idx_type,idx_type,T);
    const Array3DInfo&		info() const;
    bool			isSettable() const;

protected:

    const ValueSeries<T>*	getStorage_() const;
    bool			writable_;

    ArrayND<T>&			source_;
    mutable OffsetValueSeries<T>* storage_;

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
void Array1DSlice<T>::set( idx_type pos, T val )
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

    if ( !writable_ ) return;
    mAllocVarLenArr( idx_type, srcpos, position_.size() );
    getSourcePos( &pos, srcpos );
    source_.setND( srcpos, val );
}


template <class T> inline
T Array1DSlice<T>::get( idx_type pos ) const
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

    mAllocVarLenArr( idx_type, srcpos, position_.size() );
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
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

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
	storage_ = new OffsetValueSeries<T>( *source_.getStorage(),
					     offset_, ArrayND<T>::totalSize() );
    }
    else
    {
	storage_->setOffset( offset_ );
	storage_->setSize( ArrayND<T>::totalSize() );
    }

    return storage_;
}


//Array2DSlice
template <class T> inline
Array2DSlice<T>::Array2DSlice( ArrayND<T>& source )
    : ArrayNDSliceBase( new Array2DInfoImpl, source.info() )
    , source_( source )
    , storage_( 0 )
    , writable_( true )
{
}


template <class T> inline
Array2DSlice<T>::Array2DSlice( const ArrayND<T>& source )
    : ArrayNDSliceBase( new Array2DInfoImpl, source.info() )
    , source_( const_cast<ArrayND<T>&>(source) )
    , storage_( 0 )
    , writable_( false )
{
}


template <class T> inline
Array2DSlice<T>::~Array2DSlice()
{ delete storage_; }


template <class T> inline
bool Array2DSlice<T>::isSettable() const
{ return writable_ && source_.isSettable(); }


template <class T> inline
void Array2DSlice<T>::set( idx_type pos0, idx_type pos1, T val )
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

    if ( !writable_ ) return;

    const idx_type localpos[] = { pos0, pos1 };
    mAllocVarLenArr( idx_type, srcpos, position_.size() );
    getSourcePos( const_cast<NDPos>(localpos), srcpos );
    source_.setND( srcpos, val );
}


template <class T> inline
T Array2DSlice<T>::get( idx_type pos0, idx_type pos1 ) const
{
    if ( !isinited_ )
	{ pErrMsg("ArrayNDSlice not inited!"); }

    const idx_type localpos[] = { pos0, pos1 };
    mAllocVarLenArr( idx_type, srcpos, position_.size() );
    getSourcePos( const_cast<NDPos>(localpos), srcpos );
    return source_.getND( srcpos );
}


template <class T> inline
const Array2DInfo& Array2DSlice<T>::info() const
{
    return (const Array2DInfo&)info_;
}


template <class T> inline
const ValueSeries<T>* Array2DSlice<T>::getStorage_() const
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

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
	storage_ = new OffsetValueSeries<T>( *source_.getStorage(),
					     offset_, ArrayND<T>::totalSize() );
    }
    else
    {
	storage_->setOffset( offset_ );
	storage_->setSize( ArrayND<T>::totalSize() );
    }

    return storage_;
}


//Array3DSlice
template <class T> inline
Array3DSlice<T>::Array3DSlice( ArrayND<T>& source )
    : ArrayNDSliceBase( new Array3DInfoImpl, source.info() )
    , source_( source )
    , storage_( 0 )
    , writable_( true )
{
}


template <class T> inline
Array3DSlice<T>::Array3DSlice( const ArrayND<T>& source )
    : ArrayNDSliceBase( new Array3DInfoImpl, source.info() )
    , source_( const_cast<ArrayND<T>&>(source) )
    , storage_( 0 )
    , writable_( false )
{
}


template <class T> inline
Array3DSlice<T>::~Array3DSlice()
{ delete storage_; }


template <class T> inline
bool Array3DSlice<T>::isSettable() const
{ return writable_ && source_.isSettable(); }


template <class T> inline
void Array3DSlice<T>::set( idx_type pos0, idx_type pos1, idx_type pos2, T val )
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

    if ( !writable_ ) return;

    const idx_type localpos[] = { pos0, pos1, pos2 };
    mAllocVarLenArr( idx_type, srcpos, position_.size() );
    getSourcePos( const_cast<NDPos>(localpos), srcpos );
    source_.setND( srcpos, val );
}


template <class T> inline
T Array3DSlice<T>::get( idx_type pos0, idx_type pos1, idx_type pos2 ) const
{
    if ( !isinited_ )
	{ pErrMsg("ArrayNDSlice not inited!"); }

    const idx_type localpos[] = { pos0, pos1, pos2 };
    mAllocVarLenArr( idx_type, srcpos, position_.size() );
    getSourcePos( const_cast<NDPos>(localpos), srcpos );
    return source_.getND( srcpos );
}


template <class T> inline
const Array3DInfo& Array3DSlice<T>::info() const
{
    return (const Array3DInfo&)info_;
}


template <class T> inline
const ValueSeries<T>* Array3DSlice<T>::getStorage_() const
{
    if ( !isinited_ )
    { pErrMsg("ArrayNDSlice not inited!"); }

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
	storage_ = new OffsetValueSeries<T>( *source_.getStorage(),
					     offset_, ArrayND<T>::totalSize() );
    }
    else
    {
	storage_->setOffset( offset_ );
	storage_->setSize( ArrayND<T>::totalSize() );
    }

    return storage_;
}
