#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "survgeom.h"
#include "trckeyzsampling.h"
#include "posidxpair2coord.h"
#include "zdomain.h"


namespace Survey
{


/*!\brief Scaled down survey geometry for an inl/crl geometry

  Note that the concept of 'Structure' determines whether a geometry is
  compatible.

*/

mExpClass(Basic) Geometry3D : public Survey::Geometry
{
public:

			Geometry3D(const char* nm,const ZDomain::Def& zd );

    virtual bool	is2D() const		{ return false; }
    virtual const char*	getName() const		{ return name_.buf(); }

    const ZDomain::Def&	zDomain() const		{ return zdomain_; }
    float		zScale() const		{ return zscale_; }

    bool		isCompatibleWith(const Geometry3D&) const;

    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;
    StepInterval<float> zRange() const;
    int			inlStep() const;
    int			crlStep() const;

    inline int		idx4Inl(int) const;
    inline int		idx4Crl(int) const;
    inline int		idx4Z(float) const;
    inline int		inl4Idx(int) const;
    inline int		crl4Idx(int) const;
    inline float	z4Idx(int) const;

    float		zStep() const;

    virtual Coord	toCoord(int line,int tracenr) const;
    virtual TrcKey	nearestTrace(const Coord&,float* distance) const;
    virtual bool	includes(int line,int tracenr) const;

    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;
    const Pos::IdxPair2Coord& binID2Coord() const	{ return b2c_; }

    float		inlDistance() const;
    float		crlDistance() const;

    bool		isRightHandSystem() const;
			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */
    mDeprecated bool	isClockWise() const { return isRightHandSystem(); }

    Coord3		oneStepTranslation(const Coord3& planenormal) const;
    void		setGeomData(const Pos::IdxPair2Coord&,
					const TrcKeyZSampling&,float zscl);
    float		averageTrcDist() const;
    RelationType	compare(const Geometry&,bool usezrg) const;

    Geometry3D*		as3D()			{ return this; }

    void		snap(BinID&,const BinID& dir=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward);
    void		snapStep(BinID&,const BinID& dir=BinID(0,0))const;
			//!< see snap() for direction
    void		snapZ(float&,int direction=0) const;
			//!< see snap() for direction

    void		getStructure(const IOPar&);
    void		putStructure(IOPar&) const;
    int			bufSize4Structure() const; // 76
    void		getStructure(const void*);
    void		putStructure(void*) const;

protected:

    BufferString	name_;
    Pos::IdxPair2Coord	b2c_;
    ZDomain::Def	zdomain_;
    float		zscale_;
};


inline int Survey::Geometry3D::idx4Inl( int inl ) const
{ return sampling_.hsamp_.lineIdx( inl ); }
inline int Survey::Geometry3D::idx4Crl( int crl ) const
{ return sampling_.hsamp_.trcIdx( crl ); }
inline int Survey::Geometry3D::idx4Z( float z ) const
{ return sampling_.zsamp_.nearestIndex( z ); }
inline int Survey::Geometry3D::inl4Idx( int idx ) const
{ return sampling_.hsamp_.lineID( idx ); }
inline int Survey::Geometry3D::crl4Idx( int idx ) const
{ return sampling_.hsamp_.traceID( idx ); }
inline float Survey::Geometry3D::z4Idx( int idx ) const
{ return sampling_.zsamp_.atIndex( idx ); }


} // namespace Survey
