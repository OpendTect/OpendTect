#ifndef emsurfaceposprov_h
#define emsurfaceposprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "posprovider.h"

#include "emposid.h"
#include "horsampling.h"
#include "multiid.h"
#include "embody.h"
#include "keystrs.h"

class DataPointSet;

namespace EM { class RowColIterator; class Surface; }

namespace Pos
{

/*!\brief Provider based on surface(s)
 
  For one surface, the provider iterates trhough the horizon. For two
  horizons, the points between the surfaces are visited with the
  specified Z step.
 
 */

mClass(EarthModel) EMSurfaceProvider : public virtual Filter
{
public:
			EMSurfaceProvider();
			~EMSurfaceProvider();
    const char*		type() const;	//!< sKey::Surface()

    virtual bool	initialize(TaskRunner* tr=0);
    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual float	curZ() const;
    virtual bool	hasZAdjustment() const;
    virtual float	adjustedZ(const Coord&,float) const;

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const { return estnrpos_; }
    virtual int		estNrZPerPos() const;

    int			nrSurfaces() const;
    MultiID		surfaceID( int idx ) const
    			{ return idx ? id2_ : id1_; }
    EM::Surface*	surface( int idx )
    			{ return idx ? surf2_ : surf1_; }
    const EM::Surface*	surface( int idx ) const
    			{ return idx ? surf2_ : surf1_; }
    float		zStep() const		{ return zstep_; }
    void		setZStep( float s )	{ zstep_ = s; }
    Interval<float>	extraZ() const		{ return extraz_; }
    void		setExtraZ( Interval<float> i )	{ extraz_ = i; }

    static const char*	id1Key();
    static const char*	id2Key();
    static const char*	zstepKey();
    static const char*	extraZKey();

protected:

			EMSurfaceProvider(const EMSurfaceProvider&);
    void		copyFrom(const Pos::EMSurfaceProvider&);

    MultiID		id1_;
    MultiID		id2_;
    EM::Surface*	surf1_;
    EM::Surface*	surf2_;
    float		zstep_;
    Interval<float>	extraz_;
    HorSampling		hs_;
    Interval<float>	zrg1_;
    Interval<float>	zrg2_;
    od_int64		estnrpos_;

    EM::RowColIterator*	iterator_;
    EM::PosID		curpos_;
    Interval<float>	curzrg_;
    float		curz_;

};


#define mEMSurfaceProviderDefFnsBase \
    virtual bool	isProvider() const { return true; } \
    virtual float	estRatio( const Provider& p ) const \
			{ return Provider::estRatio(p); } \
    virtual bool	toNextPos() \
			{ return EMSurfaceProvider::toNextPos(); } \
    virtual bool	toNextZ() \
			{ return EMSurfaceProvider::toNextZ(); } \
    virtual float	curZ() const \
			{ return EMSurfaceProvider::curZ(); } \
    virtual int		estNrZPerPos() const \
			{ return EMSurfaceProvider::estNrZPerPos(); } \
    virtual od_int64	estNrPos() const { return estnrpos_; } \

/*!\brief EMSurfaceProvider for 3D positioning */

mClass(EarthModel) EMSurfaceProvider3D : public Provider3D
			  , public EMSurfaceProvider
{
public:

			EMSurfaceProvider3D()
			{}
			EMSurfaceProvider3D( const EMSurfaceProvider3D& p )
			{ *this = p; }
    const char*		factoryKeyword() const { return type(); }
    EMSurfaceProvider3D& operator =( const EMSurfaceProvider3D& p )
			{ copyFrom(p); return *this; }
    Provider*		clone() const
    			{ return new EMSurfaceProvider3D(*this); }

    virtual BinID	curBinID() const;
    virtual bool	includes(const BinID&,float) const;
    virtual bool	includes( const Coord& c, float z ) const
			{ return Provider3D::includes(c,z); }
    virtual void	getExtent(BinID&,BinID&) const;
    virtual Coord	curCoord() const { return Provider3D::curCoord(); }
    virtual void	getCubeSampling( CubeSampling& cs ) const 
			{ return Provider3D::getCubeSampling(cs); } 
    virtual void	getZRange(Interval<float>& rg ) const 
			{ return EMSurfaceProvider::getZRange(rg); } 

    static void		initClass();
    static Provider3D*	create()	{ return new EMSurfaceProvider3D; }

    mEMSurfaceProviderDefFnsBase

};


/*!\brief EMSurfaceProvider for 2D positioning */

mClass(EarthModel) EMSurfaceProvider2D : public Provider2D
			  , public EMSurfaceProvider
{
public:

			EMSurfaceProvider2D()
			{}
			EMSurfaceProvider2D( const EMSurfaceProvider2D& p )
			{ *this = p; }
    EMSurfaceProvider2D& operator =( const EMSurfaceProvider2D& p )
			{ copyFrom(p); return *this; }
    const char*		factoryKeyword() const { return type(); }
    Provider*		clone() const
    			{ return new EMSurfaceProvider2D(*this); }

    virtual const char*	curLine() const;
    virtual int		curNr() const;
    virtual Coord	curCoord() const;
    virtual bool	includes(const Coord&,float) const;
    virtual bool	includes(int,float,int) const;
    virtual void	getExtent(Interval<int>&,int nr = -1) const;
    virtual void	getZRange(Interval<float>& rg, int lidx ) const 
			{ return EMSurfaceProvider::getZRange(rg); }
    virtual void	getZRange(Interval<float>& rg ) const 
			{ return EMSurfaceProvider::getZRange(rg); }
    int			nrLines() const			{ return 1; }

    static void		initClass();
    static Provider2D*	create()	{ return new EMSurfaceProvider2D; }

    mEMSurfaceProviderDefFnsBase

};


/* !\brief EMSurfaceProvider for 3D positions with 2D Horizon */

mClass(EarthModel) EMSurface2DProvider3D : public Provider3D
			    , public EMSurfaceProvider
{
public:
    				EMSurface2DProvider3D();
				EMSurface2DProvider3D(
					const EMSurface2DProvider3D& p );
				//{ *this = p; }
				~EMSurface2DProvider3D();
    const char*			factoryKeyword() const { return type(); }
    EMSurface2DProvider3D&	operator =( const EMSurface2DProvider3D& p );
				//{ copyFrom(p); return *this; }
    virtual bool		initialize(TaskRunner* tr=0);
    Provider*			clone() const
				{ return new EMSurface2DProvider3D(*this); }

    virtual BinID       	curBinID() const;
    virtual bool		includes(const BinID&,float) const;
    virtual bool		includes( const Coord& c, float z ) const
				{ return Provider3D::includes(c,z); }
    virtual void		getExtent(BinID&,BinID&) const;
    virtual void		getZRange(Interval<float>& rg ) const 
				{ return EMSurfaceProvider::getZRange(rg); } 
    virtual Coord		curCoord() const 
    				{ return Provider3D::curCoord(); }

    const DataPointSet&		dataPointSet( bool nr1 ) const
				{ return nr1 ? dpssurf1_ : dpssurf2_; }

    mEMSurfaceProviderDefFnsBase

protected:

    DataPointSet&		dpssurf1_;
    DataPointSet&		dpssurf2_;

    void			mkDPS(const EM::Surface&,DataPointSet&);

};


mClass(EarthModel) EMImplicitBodyProvider : public Provider3D
{
public:

				EMImplicitBodyProvider();
				EMImplicitBodyProvider(
					const EMImplicitBodyProvider&);
				~EMImplicitBodyProvider();
    
    static void			initClass();
    static Provider3D*		create() { return new EMImplicitBodyProvider; }
    EMImplicitBodyProvider*	clone() const
				{ return new EMImplicitBodyProvider(*this); }

    EMImplicitBodyProvider&	operator =(const EMImplicitBodyProvider&);
    const char*			type() const		{ return sKey::Body(); }
    const char*			factoryKeyword() const	{ return type(); }

    virtual bool		initialize(TaskRunner* tr=0);
    virtual void		reset()			{ initialize(); }

    virtual BinID		curBinID() const	{ return curbid_; }
    virtual float		curZ() const		{ return curz_; }

    virtual bool		toNextPos();
    virtual bool		toNextZ();
    virtual int			estNrZPerPos() const; 
    virtual od_int64		estNrPos() const;
    virtual void		getExtent(BinID&,BinID&) const; 
    virtual void		getZRange(Interval<float>&) const;
    virtual bool		includes(const Coord& c,float z) const;
    virtual bool		includes(const BinID&,float) const;

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		getSummary(BufferString&) const;

    virtual void		getCubeSampling(CubeSampling& cs) const;
    const CubeSampling&		getImpBodyRange() const	{ return cs_; }
    Array3D<float>*		getImpBodyData() const	{ return imparr_; }
    float                       getThreshold() const	{ return threshold_; }

    static const char*		sKeyUseInside()	{ return "Use InsideBody"; }
    static const char*		sKeyBBInlrg()	{ return "BB-Inl range"; }
    static const char*		sKeyBBCrlrg()	{ return "BB-Crl range"; }
    static const char*		sKeyBBZrg()	{ return "BB-Z range"; }

protected:

    CubeSampling		cs_;
    Array3D<float>*		imparr_;
    float			threshold_;
    bool			useinside_;
    CubeSampling		bbox_;

    EM::Body*			embody_;
    BinID			curbid_;
    float			curz_;
};


} // namespace

#endif

