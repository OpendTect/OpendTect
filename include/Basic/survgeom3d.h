#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "survgeom.h"
#include "trckeyzsampling.h"
#include "posidxpair2coord.h"
#include "zdomain.h"


namespace Survey
{


/*!\brief Scaled down survey geometry for an inl/crl geometry */

mExpClass(Basic) Geometry3D : public Survey::Geometry
{
public:
			Geometry3D(const char* nm,const ZDomain::Def& zd );

    static Geometry3D&	current();

    bool		is2D() const override		{ return false; }
    const char*		getName() const override	{ return name_; }
    void		setName( const char* nm )	{ name_ = nm; }

    float		zScale() const			{ return zscale_; }

    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;
    StepInterval<float> zRange() const;
    int			inlStep() const;
    int			crlStep() const;

    float		zStep() const;

    Coord		toCoord(int line,int tracenr) const override;
    TrcKey		nearestTrace(const Coord&,
				     float* distance) const override;
    bool		includes(int line,int tracenr) const override;

    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;
    const Pos::IdxPair2Coord& binID2Coord() const	{ return b2c_; }

    float		inlDistance() const;
    float		crlDistance() const;

    bool		isRightHandSystem() const;
			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */
    bool		isClockWise() const { return isRightHandSystem(); }
			/*!< Legacy, will be removed. */

    const ZDomain::Def&	zDomain() const		{ return zdomain_; }
    void		setZDomain( const ZDomain::Def& def )
						{ zdomain_ = def; }

    Coord3		oneStepTranslation(const Coord3& planenormal) const;
    void		setGeomData(const Pos::IdxPair2Coord&,
					const TrcKeyZSampling&,float zscl);
    float		averageTrcDist() const override;
    RelationType	compare(const Geometry&,bool usezrg) const override;

    Geometry3D*		as3D() override		{ return this; }

    void		snap(BinID&,const BinID& dir=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward);
    void		snapStep(BinID&,const BinID& dir=BinID(0,0))const;
			//!< see snap() for direction
    void		snapZ(float&,int direction=0) const;
			//!< see snap() for direction
protected:

    BufferString	name_;
    ZDomain::Def	zdomain_;
    Pos::IdxPair2Coord	b2c_;

    float		zscale_ = 0.f;
};

} // namespace Survey
