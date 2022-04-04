#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
________________________________________________________________________


*/

#include "basicmod.h"

#include "gendefs.h"
#include "typeset.h"

/*!
\brief Contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) ArrayNDInfo
{
public:

    typedef od_int16	nr_dims_type;	// number of dimensions, rank
    typedef nr_dims_type dim_idx_type;
    typedef od_int32	size_type;	// size of a singe dimension
    typedef size_type	idx_type;
    typedef od_int64	offset_type;	// offset/total size in/of the array
    typedef offset_type total_size_type;
    typedef const size_type* NDSize;	// arr with sizes for each dimension
    typedef const idx_type* NDPos;
    typedef TypeSet<idx_type> NDPosBuf; // to put your own ND-indexes

    virtual ArrayNDInfo* clone() const					= 0;
    virtual		~ArrayNDInfo()					{}

    int			nrDims() const			{ return getNDim(); }
    virtual int		getNDim() const					= 0;
    virtual int		getSize(int dim) const				= 0;
    virtual bool	setSize(int dim,int sz);

    virtual bool	isOK() const;

    virtual od_uint64	getTotalSz() const;
    virtual od_uint64	getOffset(const int*) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(const int*) const;
			/*!<Checks if the position exists. */
    bool		validDimPos(int dim,int pos) const;
			/*!<Checks if the position exists on a certain dim. */
    virtual bool	getArrayPos(od_uint64, int*) const;
			/*!<Given an offset, what is the ND position. */

protected:

    od_uint64		calcTotalSz() const;

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
    int nd = a1.getNDim();
    if ( nd != a2.getNDim() ) return false;
    for ( int idx=0; idx<nd; idx++ )
	if ( a1.getSize(idx) != a2.getSize(idx) ) return false;
    return true;
}

inline bool operator !=( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{ return !(a1 == a2); }


/*!
\brief Contains the information about the size of Array1D, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) Array1DInfo : public ArrayNDInfo
{
public:

    int			getNDim() const override	{ return 1; }

    virtual od_uint64	getOffset( int pos ) const
			{ return pos; }
    virtual bool	validPos( int pos ) const
			{ return ArrayNDInfo::validPos( &pos ); }

    od_uint64		getOffset( const int* iarr ) const override
			{ return getOffset( *iarr ); }
    bool		validPos( const int* iarr ) const override
			{ return ArrayNDInfo::validPos( iarr ); }

};


/*!
\brief Contains the information about the size of Array2D, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) Array2DInfo : public ArrayNDInfo
{
public:

    int			getNDim() const override	{ return 2; }

    virtual od_uint64	getOffset(int,int) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int,int) const;

    od_uint64		getOffset( const int* iarr ) const override
			{ return ArrayNDInfo::getOffset( iarr ); }
    bool		validPos( const int* iarr ) const override
			{ return ArrayNDInfo::validPos( iarr ); }

};


/*!
\brief Contains the information about the size of Array3D, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) Array3DInfo : public ArrayNDInfo
{
public:
    int			getNDim() const override	{ return 3; }

    virtual od_uint64	getOffset(int, int, int) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int,int,int) const;

    od_uint64		getOffset( const int* iarr ) const override
			{ return ArrayNDInfo::getOffset( iarr ); }
    bool		validPos( const int* iarr ) const override
			{ return ArrayNDInfo::validPos( iarr ); }
};


/*!\brief Contains the information about the size of Array4D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array4DInfo : public ArrayNDInfo
{
public:
    int			getNDim() const override	{ return 4; }

    virtual od_uint64	getOffset(int,int,int,int) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int,int,int,int) const;

    od_uint64		getOffset( const int* pos ) const override
			{ return ArrayNDInfo::getOffset( pos ); }
    bool		validPos( const int* pos ) const override
			{ return ArrayNDInfo::validPos( pos ); }
};


/*!
\brief Implementation of Array1DInfo.
*/

mExpClass(Basic) Array1DInfoImpl : public Array1DInfo
{
public:

    Array1DInfo*	clone() const override
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(int nsz=0);
			Array1DInfoImpl(const Array1DInfo&);

    int			getSize(int dim) const override;
    bool		setSize(int dim,int nsz) override;
    bool		isOK() const override		{ return dimsz_>=0; }
    od_uint64		getTotalSz() const override	{ return dimsz_; }

protected:

    int			dimsz_;

};


/*!
\brief Implementation of Array2DInfo.
*/

mExpClass(Basic) Array2DInfoImpl : public Array2DInfo
{
public:

    Array2DInfo*	clone() const override
			{ return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(int sz0=0,int sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    int			getSize(int dim) const override;
    bool		setSize(int dim,int nsz) override;
    bool		isOK() const override	{ return cachedtotalsz_ > 0; }

    od_uint64		getTotalSz() const override { return cachedtotalsz_; }

protected:

    int                 dimsz_[2];
    od_uint64		cachedtotalsz_;

};


/*!
\brief Implementation of Array3DInfo.
*/

mExpClass(Basic) Array3DInfoImpl : public Array3DInfo
{
public:

    Array3DInfo*	clone() const override
			{ return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(int sz0=0,int sz1=0,int sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    int			getSize(int dim) const override;
    bool		setSize(int dim,int nsz) override;
    bool		isOK() const override	{ return cachedtotalsz_ > 0; }
    od_uint64		getTotalSz() const override { return cachedtotalsz_; }

protected:

    int			dimsz_[3];
    od_uint64		cachedtotalsz_;

};


/*!\brief Implementation of Array4DInfo. */

mExpClass(Basic) Array4DInfoImpl : public Array4DInfo
{
public:
    Array4DInfo*	clone() const override
			{ return new Array4DInfoImpl(*this); }

			Array4DInfoImpl(int sz0=0,int sz1=0,
					int sz2=0,int sz3=0);
			Array4DInfoImpl(const Array4DInfo&);

    int			getSize(int dim) const override;
    bool		setSize(int dim,int nsz) override;
    bool		isOK() const override	{ return cachedtotalsz_ > 0; }
    virtual od_uint64	totalSize() const	{ return cachedtotalsz_; }

protected:

    int			dimsz_[4];
    od_uint64		cachedtotalsz_;

};

/*!
\brief Implementation of ArrayNDInfo.
*/

mExpClass(Basic) ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    ArrayNDInfo*	clone() const override;
    static ArrayNDInfo*	create(int ndim);
    template <class T> static ArrayNDInfo*	create(const T*,int sz);

			ArrayNDInfoImpl(int ndim);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			ArrayNDInfoImpl(const ArrayNDInfoImpl&);
			~ArrayNDInfoImpl();
    bool		isOK() const override	{ return cachedtotalsz_ > 0; }

    od_uint64		getTotalSz() const override { return cachedtotalsz_; }
    int			getNDim() const override;
    int			getSize(int dim) const override;
    bool		setSize(int dim,int nsz) override;

protected:

    int			ndim_;
    int*		dimsz_;

    od_uint64		cachedtotalsz_;
};


inline int Array1DInfoImpl::getSize( int dim ) const
{
    return dim ? 0 : dimsz_;
}


inline int Array2DInfoImpl::getSize( int dim ) const
{
    return dim>1 || dim<0 ? 0 : dimsz_[dim];
}


inline int Array3DInfoImpl::getSize( int dim ) const
{
    return dim>2 || dim<0 ? 0 : dimsz_[dim];
}


inline int Array4DInfoImpl::getSize( int dim ) const
{
    return dim>3 || dim<0 ? 0 : dimsz_[dim];
}


template <class T> inline
ArrayNDInfo* ArrayNDInfoImpl::create( const T* sizes, int sz )
{
    TypeSet<int> dimszs;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int dimsz = mNINT32(sizes[idx]);
	if ( dimsz > 1 )
	    dimszs += dimsz;
    }
    ArrayNDInfo* ret = create( dimszs.size() );
    if ( !ret )
	return nullptr;

    for ( int idx=0; idx<ret->getNDim(); idx++ )
	ret->setSize( idx, dimszs[idx] );

    return ret;
}


