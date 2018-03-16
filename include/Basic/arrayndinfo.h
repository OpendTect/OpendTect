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

/*!\brief Contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) ArrayNDInfo
{
public:

    typedef short	NrDimsType;	// number of dimensions, rank
    typedef NrDimsType	DimIdxType;
    typedef int		SzType;		// size of a singe dimension
    typedef SzType	IdxType;
    typedef od_int64	OffsetType;	// total size of the entire array
    typedef OffsetType	TotalSzType;
    typedef const int*	NDSize;		// Array with sizes for each dimension
    typedef NDSize	NDPos;
    typedef TypeSet<IdxType> NDPosBuf;	// to put your own ND-indexes

    virtual ArrayNDInfo* clone() const			= 0;
    virtual		~ArrayNDInfo()			{}

    virtual bool	isOK() const;
    virtual NrDimsType	nrDims() const			= 0;
    virtual SzType	getSize(DimIdxType) const	= 0;
    virtual bool	setSize(DimIdxType,SzType)	{ return false; }
    virtual TotalSzType	totalSize() const;

    virtual bool	validPos(NDPos) const;
    bool		validDimPos(DimIdxType,IdxType) const;

    virtual bool	getArrayPos(OffsetType,IdxType*) const;
    virtual OffsetType	getOffset(NDPos) const;
			/*!<Returns offset in a 'flat' array.*/

    inline NrDimsType	rank() const			{ return nrDims(); }
    inline bool		validPos( const NDPosBuf& pos ) const
			{ return validPos( pos.arr() ); }
    inline bool		getArrayPos( OffsetType offs, NDPosBuf& pb ) const
			{ return getArrayPos( offs, pb.arr() ); }
    inline OffsetType	getOffset( const NDPosBuf& pos ) const
			{ return getOffset( pos.arr() ); }

protected:

			ArrayNDInfo()		{}

    TotalSzType		calcTotalSz() const;

public:

    mDeprecated inline NrDimsType getNDim() const	{ return nrDims(); }
    mDeprecated inline TotalSzType getTotalSz() const	{ return totalSize(); }

};


#define mTypeDefArrNDTypes \
    typedef ArrayNDInfo::DimIdxType	DimIdxType; \
    typedef ArrayNDInfo::NrDimsType	NrDimsType; \
    typedef ArrayNDInfo::IdxType	IdxType; \
    typedef ArrayNDInfo::SzType		SzType; \
    typedef ArrayNDInfo::OffsetType	OffsetType; \
    typedef ArrayNDInfo::TotalSzType	TotalSzType; \
    typedef ArrayNDInfo::NDSize		NDSize; \
    typedef ArrayNDInfo::NDPos		NDPos; \
    typedef ArrayNDInfo::NDPosBuf	NDPosBuf

#define mDefNDPosBuf(nm,nrdims) ArrayNDInfo::NDPosBuf nm( nrdims, 0 )
#define mNDPosFromPosBuf(bufnm) bufnm.arr()
#define mNDPosBufFromPos(pos,nrdims) ArrayNDInfo::NDPosBuf( pos, nrdims )


inline bool operator ==( const ArrayNDInfo& a1, const ArrayNDInfo& a2 )
{
    const ArrayNDInfo::NrDimsType nd = a1.nrDims();
    if ( nd != a2.nrDims() )
	return false;
    for ( ArrayNDInfo::DimIdxType idx=0; idx<nd; idx++ )
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

    virtual NrDimsType	nrDims() const			{ return 1; }

    virtual OffsetType	getOffset( IdxType pos ) const
			{ return pos; }
    virtual bool	validPos( IdxType pos ) const
			{ return ArrayNDInfo::validPos( &pos ); }

    virtual OffsetType	getOffset( NDPos pos ) const
			{ return getOffset( *pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Contains the information about the size of Array2D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array2DInfo : public ArrayNDInfo
{
public:

    virtual NrDimsType	nrDims() const			{ return 2; }

    virtual OffsetType	getOffset(IdxType,IdxType) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(IdxType,IdxType) const;

    virtual OffsetType	getOffset( NDPos pos ) const
			{ return ArrayNDInfo::getOffset( pos ); }
    virtual bool	validPos( NDPos pos ) const
			{ return ArrayNDInfo::validPos( pos ); }

};


/*!\brief Contains the information about the size of Array3D, and
in what order the data is stored (if accessable via a pointer). */

mExpClass(Basic) Array3DInfo : public ArrayNDInfo
{
public:

    virtual NrDimsType	nrDims() const			{ return 3; }

    virtual OffsetType	getOffset(IdxType,IdxType,IdxType) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(IdxType,IdxType,IdxType) const;

    virtual OffsetType	getOffset( NDPos pos ) const
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

			Array1DInfoImpl(SzType nsz=0);
			Array1DInfoImpl(const Array1DInfo&);

    virtual SzType	getSize(DimIdxType) const;
    virtual bool	setSize(DimIdxType,SzType);
    virtual bool	isOK() const			{ return dimsz_>=0; }
    virtual TotalSzType	totalSize() const		{ return dimsz_; }

protected:

    SzType		dimsz_;

};


/*!\brief Implementation of Array2DInfo. */

mExpClass(Basic) Array2DInfoImpl : public Array2DInfo
{
public:

    virtual Array2DInfo* clone() const { return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(SzType sz0=0,SzType sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    virtual SzType	getSize(DimIdxType) const;
    virtual bool	setSize(DimIdxType,SzType nsz);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual TotalSzType	totalSize() const	{ return cachedtotalsz_; }

protected:

    SzType		dimsz_[2];
    TotalSzType		cachedtotalsz_;

};


/*!\brief Implementation of Array3DInfo. */

mExpClass(Basic) Array3DInfoImpl : public Array3DInfo
{
public:

    virtual Array3DInfo* clone() const { return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(SzType sz0=0,SzType sz1=0,
					SzType sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    virtual SzType	getSize(DimIdxType) const;
    virtual bool	setSize(DimIdxType,SzType);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }
    virtual TotalSzType	totalSize() const	{ return cachedtotalsz_; }

protected:

    SzType		dimsz_[3];
    TotalSzType		cachedtotalsz_;

};


/*!\brief Implementation of ArrayNDInfo. */

mExpClass(Basic) ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const;
    static ArrayNDInfo*	create(NrDimsType);

			ArrayNDInfoImpl(NrDimsType);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			ArrayNDInfoImpl(const ArrayNDInfoImpl&);
			~ArrayNDInfoImpl();
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual TotalSzType	totalSize() const	{ return cachedtotalsz_; }
    virtual NrDimsType	nrDims() const;
    virtual SzType	getSize(DimIdxType) const;
    virtual bool	setSize(DimIdxType,SzType);

protected:

    NrDimsType		ndim_;
    IdxType*		dimsz_;

    TotalSzType		cachedtotalsz_;

};


inline ArrayNDInfo::SzType Array1DInfoImpl::getSize( DimIdxType dim ) const
{
    return dim ? 0 : dimsz_;
}


inline ArrayNDInfo::SzType Array2DInfoImpl::getSize( DimIdxType dim ) const
{
    return dim>1 || dim<0 ? 0 : dimsz_[dim];
}


inline ArrayNDInfo::SzType Array3DInfoImpl::getSize( DimIdxType dim ) const
{
    return dim>2 || dim<0 ? 0 : dimsz_[dim];
}
