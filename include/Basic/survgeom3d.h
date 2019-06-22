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


namespace Survey
{


/*!\brief Scaled down survey geometry for an inl/crl geometry

  Note that the concept of 'Structure' determines whether a geometry is
  compatible.

*/

mExpClass(Basic) Geometry3D : public Survey::Geometry
{
public:

    mUseType( Pos,	IdxPair2Coord );

			Geometry3D(const char* nm=0);
			Geometry3D(const Geometry3D&);
    Geometry3D*		clone() const override	{ return new Geometry3D(*this);}

    GeomSystem		geomSystem() const override { return OD::VolBasedGeom; }
    const name_type&	name() const override	{ return name_; }

    pos_steprg_type&	inlRange()		{ return inlrg_; }
    const pos_steprg_type& inlRange() const	{ return inlrg_; }
    pos_steprg_type&	crlRange()		{ return trcNrRange(); }
    const pos_steprg_type& crlRange() const	{ return trcNrRange(); }

    inline idx_type	idx4Inl(pos_type) const;
    inline idx_type	idx4Crl(pos_type) const;
    inline pos_type	inl4Idx(idx_type) const;
    inline pos_type	crl4Idx(idx_type) const;

    bool		includes(const BinID&) const;
    Coord		getCoord(const BinID&) const;
    BinID		nearestTracePosition(const Coord&,dist_type* d=0) const;
    BinID		tracePosition(const Coord&,
				      dist_type maxdist=mUdf(dist_type)) const;

    const IdxPair2Coord& binID2Coord() const	{ return b2c_; }
    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;
    bool		isRightHandSystem() const;
    BinID		origin() const;
				//!< rotating the inline to the crossline axis

    void		snap(BinID&,OD::SnapDir =OD::SnapNearest) const;
    void		snapCrl(pos_type&,OD::SnapDir =OD::SnapNearest) const;
    void		snapStep(BinID&) const;

    dist_type		inlDistance() const; //!< NOT per step but per 1 inline
    dist_type		crlDistance() const; //!< NOT per step but per 1 xline
    dist_type		averageTrcDist() const;

			// MapInfo = B2C and CoordSys
    void		getMapInfo(const IOPar&);
    void		putMapInfo(IOPar&) const;

protected:

    BufferString	name_;
    IdxPair2Coord	b2c_;
    pos_steprg_type	inlrg_;

    Geometry2D*		gtAs2D() const	override		{ return 0; }
    Geometry3D*		gtAs3D() const	override
			{ return const_cast<Geometry3D*>(this); }

public:

    void		setName( const char* nm )		{ name_ = nm; }
    void		setTransform(const Pos::IdxPair2Coord&);
    void		setRanges(const pos_steprg_type&,const pos_steprg_type&,
				  const z_steprg_type&);

    RelationType	compare(const Geometry3D&,bool usezrg) const;

    Coord3		oneStepTranslation(const Coord3& planenormal) const;

};


inline Geometry3D::idx_type Geometry3D::idx4Inl( pos_type inl ) const
{ return inlRange().nearestIndex( inl ); }
inline Geometry3D::idx_type Geometry3D::idx4Crl( pos_type crl ) const
{ return crlRange().nearestIndex( crl ); }
inline Geometry3D::pos_type Geometry3D::inl4Idx( idx_type idx ) const
{ return inlRange().atIndex( idx ); }
inline Geometry3D::pos_type Geometry3D::crl4Idx( idx_type idx ) const
{ return crlRange().atIndex( idx ); }


} // namespace Survey
