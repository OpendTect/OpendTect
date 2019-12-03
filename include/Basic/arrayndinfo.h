#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________


*/

#include "basicmod.h"
#include "typeset.h"

/*!\brief Contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) ArrayNDInfo
{
public:

    typedef od_int16	nr_dims_type;	// number of dimensions, rank
    typedef nr_dims_type dim_idx_type;
    typedef od_int32	size_type;	// size of a singe dimension
    typedef size_type	idx_type;
    typedef od_int64	offset_type;	// offset/total size in/of the array
    typedef offset_type	total_size_type;
    typedef const size_type* NDSize;	// arr with sizes for each dimension
    typedef const idx_type* NDPos;
    typedef TypeSet<idx_type> NDPosBuf;	// to put your own ND-indexes

    virtual ArrayNDInfo* clone() const			= 0;
    virtual		~ArrayNDInfo()			{}

    virtual bool	isOK() const;
    virtual bool	isEqual(const ArrayNDInfo&) const;
    virtual nr_dims_type nrDims() const			= 0;
    virtual size_type	getSize(dim_idx_type) const	= 0;
    virtual bool	setSize(dim_idx_type,size_type)	{ return false; }
    virtual total_size_type totalSize() const;

    virtual bool	validPos(NDPos) const;
    bool		validDimPos(dim_idx_type,idx_type) const;

    virtual bool	getArrayPos(offset_type,idx_type*) const;
    virtual offset_type	getOffset(NDPos) const;
			/*!<Returns offset in a 'flat' array.*/

    inline nr_dims_type	rank() const			{ return nrDims(); }
    inline bool		validPos( const NDPosBuf& pos ) const
			{ return validPos( pos.arr() ); }
    inline bool		getArrayPos( offset_type offs, NDPosBuf& pb ) const
			{ return getArrayPos( offs, pb.arr() ); }
    inline offset_type	getOffset( const NDPosBuf& pos ) const
			{ return getOffset( pos.arr() ); }

protected:

			ArrayNDInfo()		{}

    total_size_type	calcTotalSz() const;

public:

    mDeprecated inline nr_dims_type getNDim() const	{ return nrDims(); }
    mDeprecated inline total_size_type getTotalSz() const
							{ return totalSize(); }

};


#define mTypeDefArrNDTypes \
    mUseType( ArrayNDInfo, dim_idx_type ); \
    mUseType( ArrayNDInfo, nr_dims_type ); \
    mUseType( ArrayNDInfo, idx_type ); \
    mUseType( ArrayNDInfo, size_type ); \
    mUseType( ArrayNDInfo, offset_type ); \
    mUseType( ArrayNDInfo, total_size_type ); \
    mUseType( ArrayNDInfo, NDSize ); \
    mUseType( ArrayNDInfo, NDPos ); \
    mUseType( ArrayNDInfo, NDPosBuf ); \

#define mDefNDPosBuf(nm,nrdims) ArrayNDInfo::NDPosBuf nm( nrdims, 0 )
#define mNDPosFromPosBuf(bufnm) bufnm.arr()
#define mNDPosBufFromPos(pos,nrdims) ArrayNDInfo::NDPosBuf( pos, nrdims )


inline bool operator ==( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{
    const ArrayNDInfo::nr_dims_type nd = a1.nrDims();
    if ( nd != a2.nrDims() )
	return false;
    for ( ArrayNDInfo::dim_idx_type idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) )
	    return false;
    return true;
}

inline bool operator !=( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{
    return !(a1 == a2);
}


/*!\brief Contains the information about the size of Array1D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array1DInfo : public ArrayNDInfo
{
public:

    virtual nr_dims_type nrDims() const			{ return 1; }

    virtual offset_type	getOffset( idx_type pos ) const
			{ return pos; }
    virtual bool	validPos( idx_type pos ) const
			{ return ArrayNDInfo::validPos( &pos ); }

    virtual offset_type	getOffset( NDPos pos ) const
			{ return getOffset( *pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Contains the information about the size of Array2D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array2DInfo : public ArrayNDInfo
{
public:

    virtual nr_dims_type nrDims() const			{ return 2; }

    virtual offset_type	getOffset(idx_type,idx_type) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(idx_type,idx_type) const;

    virtual offset_type	getOffset( NDPos pos ) const
			{ return ArrayNDInfo::getOffset( pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Contains the information about the size of Array3D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array3DInfo : public ArrayNDInfo
{
public:

    virtual nr_dims_type nrDims() const			{ return 3; }

    virtual offset_type	getOffset(idx_type,idx_type,idx_type) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(idx_type,idx_type,idx_type) const;

    virtual offset_type	getOffset( NDPos pos ) const
			{ return ArrayNDInfo::getOffset( pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Contains the information about the size of Array4D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array4DInfo : public ArrayNDInfo
{
public:

    virtual nr_dims_type nrDims() const			{ return 4; }

    virtual offset_type	getOffset(idx_type,idx_type,idx_type,idx_type) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(idx_type,idx_type,idx_type,idx_type) const;

    virtual offset_type	getOffset( NDPos pos ) const
			{ return ArrayNDInfo::getOffset( pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Implementation of Array1DInfo. */

mExpClass(Basic) Array1DInfoImpl : public Array1DInfo
{
public:

    virtual Array1DInfo* clone() const
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(size_type nsz=0);
			Array1DInfoImpl(const Array1DInfo&);

    virtual size_type	getSize(dim_idx_type) const;
    virtual bool	setSize(dim_idx_type,size_type);
    virtual bool	isOK() const			{ return dimsz_>=0; }
    virtual total_size_type totalSize() const		{ return dimsz_; }

protected:

    size_type		dimsz_;

};


/*!\brief Implementation of Array2DInfo. */

mExpClass(Basic) Array2DInfoImpl : public Array2DInfo
{
public:

    virtual Array2DInfo* clone() const { return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(size_type sz0=0,size_type sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    virtual size_type	getSize(dim_idx_type) const;
    virtual bool	setSize(dim_idx_type,size_type nsz);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual total_size_type totalSize() const	{ return cachedtotalsz_; }

protected:

    size_type		dimsz_[2];
    total_size_type	cachedtotalsz_;

};


/*!\brief Implementation of Array3DInfo. */

mExpClass(Basic) Array3DInfoImpl : public Array3DInfo
{
public:

    virtual Array3DInfo* clone() const { return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(size_type sz0=0,size_type sz1=0,
					size_type sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    virtual size_type	getSize(dim_idx_type) const;
    virtual bool	setSize(dim_idx_type,size_type);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }
    virtual total_size_type totalSize() const	{ return cachedtotalsz_; }

protected:

    size_type		dimsz_[3];
    total_size_type	cachedtotalsz_;

};

/*!\brief Implementation of Array4DInfo. */

mExpClass(Basic) Array4DInfoImpl : public Array4DInfo
{
public:

    virtual Array4DInfo* clone() const { return new Array4DInfoImpl(*this); }

			Array4DInfoImpl(size_type sz0=0,size_type sz1=0,
					size_type sz2=0,size_type sz3=0);
			Array4DInfoImpl(const Array4DInfo&);

    virtual size_type	getSize(dim_idx_type) const;
    virtual bool	setSize(dim_idx_type,size_type);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }
    virtual total_size_type totalSize() const	{ return cachedtotalsz_; }

protected:

    size_type		dimsz_[4];
    total_size_type	cachedtotalsz_;

};


/*!\brief Implementation of ArrayNDInfo. */

mExpClass(Basic) ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const;
    static ArrayNDInfo*	create(nr_dims_type);
    template <class T> static ArrayNDInfo*	create(const T*,int sz);

			ArrayNDInfoImpl(nr_dims_type);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			ArrayNDInfoImpl(const ArrayNDInfoImpl&);
			~ArrayNDInfoImpl();
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual total_size_type totalSize() const	{ return cachedtotalsz_; }
    virtual nr_dims_type nrDims() const;
    virtual size_type	getSize(dim_idx_type) const;
    virtual bool	setSize(dim_idx_type,size_type);

protected:

    nr_dims_type	ndim_;
    idx_type*		dimsz_;

    total_size_type	cachedtotalsz_;

};


inline ArrayNDInfo::size_type Array1DInfoImpl::getSize( dim_idx_type dim ) const
{
    return dim ? 0 : dimsz_;
}


inline ArrayNDInfo::size_type Array2DInfoImpl::getSize( dim_idx_type dim ) const
{
    return dim>1 || dim<0 ? 0 : dimsz_[dim];
}


inline ArrayNDInfo::size_type Array3DInfoImpl::getSize( dim_idx_type dim ) const
{
    return dim>2 || dim<0 ? 0 : dimsz_[dim];
}


inline ArrayNDInfo::size_type Array4DInfoImpl::getSize( dim_idx_type dim ) const
{
    return dim>3 || dim<0 ? 0 : dimsz_[dim];
}

template <class T> inline
ArrayNDInfo* ArrayNDInfoImpl::create( const T* sizes, int sz )
{
    TypeSet<size_type> dimszs;
    for ( int idx=0; idx<sz; idx++ )
    {
	const size_type dimsz = mCast(size_type,mNINT32(sizes[idx]));
	if ( dimsz > 1 )
	    dimszs += dimsz;
    }
    ArrayNDInfo* ret = create( dimszs.size() );
    if ( !ret )
	return nullptr;

    for ( dim_idx_type idx=0; idx<ret->rank(); idx++ )
	ret->setSize( idx, dimszs[idx] );

    return ret;
}
