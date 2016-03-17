#ifndef survgeom3d_h
#define survgeom3d_h

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


/*!\brief Scaled down survey geometry for an inl/crl geometry */

mExpClass(Basic) Geometry3D : public Survey::Geometry
{
public:
    
			Geometry3D(const char* nm,const ZDomain::Def& zd );

    virtual bool	is2D() const		{ return false; }
    virtual const char*	getName() const		{ return name_.buf(); }
    			    
    float		zScale() const 		{ return zscale_; }

    StepInterval<int>	inlRange() const;
    StepInterval<int>	crlRange() const;
    StepInterval<float> zRange() const;
    int			inlStep() const;
    int			crlStep() const;
    
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
    bool		isClockWise() const { return isRightHandSystem(); }
			/*!< Legacy, will be removed. */

    const ZDomain::Def&	zDomain() const		{ return zdomain_; }

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
protected:
    
    BufferString	name_;
    const ZDomain::Def	zdomain_;
    Pos::IdxPair2Coord	b2c_;

    float		zscale_;
};

} // namespace Survey


#endif
