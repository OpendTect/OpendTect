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

    typedef pos_idx_type		idx_type;
    typedef StepInterval<pos_idx_type>	pos_idx_range_type;

			Geometry3D();
			Geometry3D(const char* nm,const ZDomain::Def& zd );

    virtual bool	is2D() const		{ return false; }
    virtual const name_type& name() const	{ return name_; }
    virtual void	setName( const char* nm ) { name_ = nm; }

    const ZDomain::Def&	zDomain() const		{ return zdomain_; }
    void		setZDomain( const ZDomain::Def& def )
						{ zdomain_ = def; }
    z_type		zScale() const		{ return zscale_; }

    virtual RelationType compare(const Geometry&,bool usezrg) const;

    pos_idx_type	inlStart() const;
    pos_idx_type	crlStart() const;
    pos_idx_type	inlStop() const;
    pos_idx_type	crlStop() const;
    pos_idx_type	inlStep() const;
    pos_idx_type	crlStep() const;
    pos_idx_range_type	inlRange() const;
    pos_idx_range_type	crlRange() const;

    z_type		zStart() const;
    z_type		zStop() const;
    z_type		zStep() const;
    StepInterval<z_type> zRange() const;

    inline idx_type	idx4Inl(pos_idx_type) const;
    inline idx_type	idx4Crl(pos_idx_type) const;
    inline idx_type	idx4Z(z_type) const;
    inline pos_idx_type	inl4Idx(idx_type) const;
    inline pos_idx_type	crl4Idx(idx_type) const;
    inline z_type	z4Idx(int) const;

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
					const TrcKeyZSampling&,z_type zscl);
    float		averageTrcDist() const;

    Geometry3D*		as3D()			{ return this; }

    void		snap(BinID&,const BinID& dir=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward);
    void		snapStep(BinID&,const BinID& dir=BinID(0,0))const;
			//!< see snap() for direction
    void		snapZ(z_type&,int direction=0) const;
			//!< see snap() for direction

			// MapInfo = B2C and CoordSys
    void		getMapInfo(const IOPar&);
    void		putMapInfo(IOPar&) const;

protected:

    BufferString	name_;
    Pos::IdxPair2Coord	b2c_;
    ZDomain::Def	zdomain_;
    z_type		zscale_;

};


inline Geometry3D::idx_type Geometry3D::idx4Inl( pos_idx_type inl ) const
{ return sampling_.hsamp_.lineIdx( inl ); }
inline Geometry3D::idx_type Geometry3D::idx4Crl( pos_idx_type crl ) const
{ return sampling_.hsamp_.trcIdx( crl ); }
inline Geometry3D::idx_type Geometry3D::idx4Z( z_type z ) const
{ return sampling_.zsamp_.nearestIndex( z ); }
inline Geometry3D::pos_idx_type Geometry3D::inl4Idx( idx_type idx ) const
{ return sampling_.hsamp_.lineID( idx ); }
inline Geometry3D::pos_idx_type Geometry3D::crl4Idx( idx_type idx ) const
{ return sampling_.hsamp_.traceID( idx ); }
inline Geometry3D::z_type Geometry3D::z4Idx( idx_type idx ) const
{ return sampling_.zsamp_.atIndex( idx ); }


} // namespace Survey
