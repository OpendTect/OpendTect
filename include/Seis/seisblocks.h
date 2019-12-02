#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seiscommon.h"
#include "ranges.h"
#include "iopar.h"

namespace Survey { class Geometry3D; }
template <class T> class DataInterpreter;


namespace Seis
{

/*!\brief Storage and access of data in survey-geometry driven blocks.

A 3D survey Geometry defines a full 'Lattice' that in turns defines
a unique set of indices for inlines, crosslines and Z. We can group the
positions into blocks of a size that makes it reasonable for efficient storage.

With these predefined dimensions, we can set up indexes for each block in each
dimension (the GlobIdx). Within the blocks, you then have local, relative
indices 0 - N-1 (the LocIdx).

  */

namespace Blocks
{
    typedef od_uint16		version_type;
    typedef od_uint16		size_type;
    typedef od_int16		idx_type;
    typedef SurvGeom3D		HGeom;
    typedef StepInterval<float>	ZGeom;
    typedef DataInterpreter<float> DataInterp;

#define mDefSeisBlocksPairClass(clss,typ) \
mExpClass(Seis) clss : public Twins<typ> \
{ \
public: \
\
    inline	clss()	: Twins<typ>(0,0)		{} \
    inline	clss( typ iidx, typ xidx ) \
			: Twins<typ>(iidx,xidx)		{} \
    inline bool	operator ==( const clss& oth ) const \
		{ return first() == oth.first() && second() == oth.second(); } \
    inline bool	operator <( const clss& oth ) const \
		{ return pair() < oth.pair(); } \
\
    inline typ	inl() const	{ return first(); } \
    inline typ&	inl()		{ return first(); } \
    inline typ	crl() const	{ return second(); } \
    inline typ&	crl()		{ return second(); } \
}

mDefSeisBlocksPairClass(HGlobIdx,idx_type);
mDefSeisBlocksPairClass(HLocIdx,idx_type);
mDefSeisBlocksPairClass(HDimensions,size_type);

#define mDefSeisBlocksTripletClass(clss,typ) \
mExpClass(Seis) clss : public Triplets<typ> \
{ \
public: \
 \
    inline		clss()			{} \
    inline		clss( typ iidx, typ xidx, typ zidx ) \
			    : Triplets<typ>(iidx,xidx,zidx)	{} \
    inline bool		operator ==( const clss& oth ) const \
			{ return Triplets<typ>::operator==(oth); } \
    inline clss&	set( const H##clss& oth ) \
			{ first()=oth.first(); second()=oth.second(); \
			    return *this; } \
    inline clss&	set( const clss& oth ) \
			{ *this = oth; return *this; } \
 \
    inline typ		inl() const	{ return first(); } \
    inline typ&		inl()		{ return first(); } \
    inline typ		crl() const	{ return second(); } \
    inline typ&		crl()		{ return second(); } \
    inline typ		z() const	{ return third(); } \
    inline typ&		z()		{ return third(); } \
 \
}

mDefSeisBlocksTripletClass(GlobIdx,idx_type);
mDefSeisBlocksTripletClass(LocIdx,idx_type);
mDefSeisBlocksTripletClass(Dimensions,size_type);


/*!\brief Base class for single block. */

mExpClass(Seis) Block
{
public:

			~Block()		{}

    const GlobIdx&	globIdx() const		{ return globidx_; }
    const HLocIdx&	start() const		{ return start_; }
    const Dimensions&	dims() const		{ return dims_; }

    static idx_type	globIdx4Inl(const HGeom&,int inl,size_type inlsz);
    static idx_type	globIdx4Crl(const HGeom&,int crl,size_type crlsz);
    static idx_type	globIdx4Z(const ZGeom&,float z,size_type zsz);
    static idx_type	locIdx4Inl(const HGeom&,int inl,size_type inlsz);
    static idx_type	locIdx4Crl(const HGeom&,int crl,size_type crlsz);
    static idx_type	locIdx4Z(const ZGeom&,float z,size_type zdim);
    static int		startInl4GlobIdx(const HGeom&,idx_type,size_type inlsz);
    static int		startCrl4GlobIdx(const HGeom&,idx_type,size_type crlsz);
    static float	startZ4GlobIdx(const ZGeom&,idx_type,size_type zsz);
    static int		inl4Idxs(const HGeom&,size_type inlsz,idx_type globidx,
				idx_type sampidx);
    static int		crl4Idxs(const HGeom&,size_type crlsz,idx_type globidx,
				idx_type sampidx);
    static float	z4Idxs(const ZGeom&,size_type zsz,idx_type globidx,
				idx_type loczidx);

    static Dimensions	defDims();

protected:

			Block( const GlobIdx& gidx, const HLocIdx& s,
			       const Dimensions& d )
			    : globidx_(gidx), start_(s), dims_(d)   {}

    const GlobIdx	globidx_;
    const HLocIdx	start_;
    const Dimensions	dims_;

};


/*!\brief Base class for column of blocks. */

mExpClass(Seis) Column
{
public:

    virtual		~Column()					{}

    const HGlobIdx&	globIdx() const		{ return globidx_; }
    const Dimensions&	dims() const		{ return dims_; }
    int			nrComponents() const	{ return nrcomps_; }

protected:

			Column( const HGlobIdx& gidx, const Dimensions& d,
				int nc )
			    : globidx_(gidx), dims_(d), nrcomps_(nc)	{}

    const HGlobIdx	globidx_;
    const Dimensions	dims_;
    const int		nrcomps_;

};


} // namespace Blocks

} // namespace Seis
