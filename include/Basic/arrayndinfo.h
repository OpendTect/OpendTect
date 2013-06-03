#ifndef arrayndinfo_h
#define arrayndinfo_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id$
________________________________________________________________________


*/

#include "basicmod.h"
#include "gendefs.h"

/*!
\brief Contains the information about the size of ArrayND, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const					= 0;
    virtual		~ArrayNDInfo()					{} 

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

    virtual int		getNDim() const			{ return 1; }

    virtual od_uint64	getOffset( int pos ) const
			{ return pos; }
    virtual bool	validPos( int pos ) const
			{ return ArrayNDInfo::validPos( &pos ); }

    virtual od_uint64	getOffset( const int* iarr ) const
			{ return getOffset( *iarr ); }
    virtual bool	validPos( const int* iarr ) const
			{ return ArrayNDInfo::validPos( iarr ); }

};


/*!
\brief Contains the information about the size of Array2D, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) Array2DInfo : public ArrayNDInfo
{
public:

    virtual int		getNDim() const			{ return 2; }

    virtual od_uint64	getOffset(int,int) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int,int) const;

    virtual od_uint64	getOffset( const int* iarr ) const
			{ return ArrayNDInfo::getOffset( iarr ); }
    virtual bool	validPos( const int* iarr ) const
			{ return ArrayNDInfo::validPos( iarr ); }

};


/*!
\brief Contains the information about the size of Array3D, and
in what order the data is stored (if accessable via a pointer).
*/

mExpClass(Basic) Array3DInfo : public ArrayNDInfo
{
public:

    virtual int		getNDim() const			{ return 3; }

    virtual od_uint64	getOffset(int, int, int) const;
			/*!<Returns offset in a 'flat' array.*/
    virtual bool	validPos(int,int,int) const;

    virtual od_uint64	getOffset( const int* iarr ) const
			{ return ArrayNDInfo::getOffset( iarr ); }
    virtual bool	validPos( const int* iarr ) const
			{ return ArrayNDInfo::validPos( iarr ); }

};


/*!
\brief Implementation of Array1DInfo.
*/

mExpClass(Basic) Array1DInfoImpl : public Array1DInfo
{
public:

    virtual Array1DInfo* clone() const
			{ return new Array1DInfoImpl(*this); }

			Array1DInfoImpl(int nsz=0); 
			Array1DInfoImpl(const Array1DInfo&);

    virtual int		getSize(int dim) const; 
    virtual bool	setSize(int dim,int nsz);
    virtual bool	isOK() const			{ return dimsz_>=0; }
    virtual od_uint64	getTotalSz() const		{ return dimsz_; }

protected:

    int			dimsz_;

};


/*!
\brief Implementation of Array2DInfo.
*/

mExpClass(Basic) Array2DInfoImpl : public Array2DInfo
{
public:

    virtual Array2DInfo* clone() const { return new Array2DInfoImpl(*this); }

			Array2DInfoImpl(int sz0=0, int sz1=0);
			Array2DInfoImpl(const Array2DInfo&);

    virtual int		getSize(int dim) const;
    virtual bool	setSize(int dim,int nsz);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual od_uint64	getTotalSz() const	{ return cachedtotalsz_; }
    
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

    virtual Array3DInfo* clone() const { return new Array3DInfoImpl(*this); }

			Array3DInfoImpl(int sz0=0, int sz1=0, int sz2=0);
			Array3DInfoImpl(const Array3DInfo&);

    virtual int		getSize(int dim) const; 
    virtual bool	setSize(int dim,int nsz);
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }
    virtual od_uint64	getTotalSz() const	{ return cachedtotalsz_; }

protected:

    int                 dimsz_[3];
    od_uint64		cachedtotalsz_;

};  


/*!
\brief Implementation of ArrayNDInfo.
*/

mExpClass(Basic) ArrayNDInfoImpl : public ArrayNDInfo
{
public:

    virtual ArrayNDInfo* clone() const;
    static ArrayNDInfo*	create(int ndim);

			ArrayNDInfoImpl(int ndim);
			ArrayNDInfoImpl(const ArrayNDInfo&);
			~ArrayNDInfoImpl();
    virtual bool	isOK() const		{ return cachedtotalsz_ > 0; }

    virtual od_uint64	getTotalSz() const	{ return cachedtotalsz_; }
    virtual int		getNDim() const;
    virtual int		getSize(int dim) const;
    virtual bool	setSize(int dim,int nsz);

protected:

    int 		ndim_;
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


#endif

