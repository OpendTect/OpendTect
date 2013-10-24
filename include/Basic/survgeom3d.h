#ifndef survgeom3d_h
#define survgeom3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		2013
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "basicmod.h"
#include "survgeom.h"
#include "cubesampling.h"
#include "posidxpair2coord.h"


namespace Survey
{


/*!\brief Scaled down survey geometry for an inl/crl geometry */

mExpClass(Basic) Geometry3D : public Survey::Geometry
{
public:
    
			Geometry3D(const char* nm,const ZDomain::Def& zd )
			    : name_( nm )
    			    , zdomain_( zd )	{}
    virtual bool	is2D() const		{ return false; }
    virtual const char*	getName() const		{ return name_.buf(); }
    			    
    float		zScale() const 		{ return zscale_; }

    StepInterval<int>	inlRange() const	{ return cs_.hrg.inlRange(); }
    StepInterval<int>	crlRange() const	{ return cs_.hrg.crlRange(); }
    StepInterval<float>	zRange() const		{ return cs_.zrg; }
    int			inlStep() const 	{ return cs_.hrg.step.inl(); }
    int			crlStep() const 	{ return cs_.hrg.step.crl(); }
    
    float		zStep() const 		{ return cs_.zrg.step; }

    virtual Coord	toCoord(int line,int tracenr) const;
    virtual TrcKey	nearestTrace(const Coord&,float* distance) const;
    virtual bool	includes(int line,int tracenr) const;

    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;
    const Pos::IdxPair2Coord& binID2Coord() const	{ return b2c_; }

    float		inlDistance() const;
    float		crlDistance() const;

    bool		isClockWise() const;
    			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */

    const CubeSampling&	sampling() const	{ return cs_; }
    const ZDomain::Def&	zDomain() const		{ return zdomain_; }

    Coord3		oneStepTranslation(const Coord3& planenormal) const;
    void		setGeomData(const Pos::IdxPair2Coord&,
	    				const CubeSampling&,float zscl);

protected:
    
    BufferString	name_;
    const ZDomain::Def	zdomain_;
    Pos::IdxPair2Coord	b2c_;
    CubeSampling	cs_; 
    float		zscale_;

};

} // namespace Survey


#endif
