#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________


-*/

#include "embody.h"
#include "posprovider.h"
#include "trckeysampling.h"
#include "dbkey.h"
#include "keystrs.h"
#include "sortedlist.h"
#include <tuple>

class DataPointSet;

namespace EM { class RowColIterator; class Region3D; class Surface; }

namespace Pos
{

/*!
\brief Provider based on surface(s)

  For one surface, the provider iterates through the horizon. For two horizons,
  the points between the surfaces are visited with the specified Z step.
*/
typedef std::tuple<od_int64,int> postuple;

mExpClass(EarthModel) EMSurfaceProvider : public virtual Filter
{ mODTextTranslationClass(EMSurfaceProvider);
public:
			EMSurfaceProvider();
    virtual		~EMSurfaceProvider();
    const char*		type() const;	//!< sKey::Surface()

    virtual bool	initialize(const TaskRunnerProvider&);
    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual float	curZ() const;
    virtual bool	hasZAdjustment() const;
    virtual float	adjustedZ(const Coord&,float) const;

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(uiString&) const;

    bool		getZRange(const TrcKey&,Interval<float>&) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const { return estnrpos_; }
    virtual int		estNrZPerPos() const;

    int			nrSurfaces() const;
    DBKey		surfaceID( int idx ) const
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
    EMSurfaceProvider&	operator=(const EMSurfaceProvider&);

    DBKey		id1_;
    DBKey		id2_;
    EM::Surface*	surf1_;
    EM::Surface*	surf2_;
    float		zstep_;
    Interval<float>	extraz_;
    TrcKeySampling		hs_;
    Interval<float>	zrg1_;
    Interval<float>	zrg2_;
    od_int64		estnrpos_;

    EM::RowColIterator*	iterator_;
    EM::PosID		curpos_;
    Interval<float>	curzrg_;
    float		curz_;
    int			nrsamples_;
    bool		dorandom_;
    bool		enoughsamples_;
    od_int64		maxidx_;
    SortedList<postuple> posindexlst_;
    TypeSet<GeomPosID>	posids_;
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

/*!
\brief EMSurfaceProvider for 3D positioning.
*/

mExpClass(EarthModel) EMSurfaceProvider3D : public Provider3D
					  , public EMSurfaceProvider
{ mODTextTranslationClass(EMSurfaceProvider3D);
public:

			EMSurfaceProvider3D();
			EMSurfaceProvider3D(const EMSurfaceProvider3D&);
    const char*		factoryKeyword() const { return type(); }
    EMSurfaceProvider3D& operator =(const EMSurfaceProvider3D&);
    Provider*		clone() const
			{ return new EMSurfaceProvider3D(*this); }

    virtual BinID	curBinID() const;
    virtual bool	includes(const BinID&,float) const;
    virtual bool	includes( const Coord& c, float z ) const
			{ return Provider3D::includes(c,z); }
    virtual void	getExtent(BinID&,BinID&) const;
    virtual Coord	curCoord() const { return Provider3D::curCoord(); }
    virtual void	getTrcKeyZSampling( TrcKeyZSampling& cs ) const
			{ return Provider3D::getTrcKeyZSampling(cs); }
    virtual void	getZRange(Interval<float>& rg ) const
			{ return EMSurfaceProvider::getZRange(rg); }
    GeomID		curGeomID() const override { return GeomID::get3D(); }

    static void		initClass();
    static Provider3D*	create()	{ return new EMSurfaceProvider3D; }

    mEMSurfaceProviderDefFnsBase

};


/*!\brief EMSurfaceProvider for 2D positioning.  */

mExpClass(EarthModel) EMSurfaceProvider2D : public Provider2D
					  , public EMSurfaceProvider
{ mODTextTranslationClass(EMSurfaceProvider2D);
public:

			EMSurfaceProvider2D();
			EMSurfaceProvider2D(const EMSurfaceProvider2D&);
    EMSurfaceProvider2D& operator =(const EMSurfaceProvider2D&);
    const char*		factoryKeyword() const { return type(); }
    Provider*		clone() const
			{ return new EMSurfaceProvider2D(*this); }

    virtual const char*	curLine() const;
    virtual Bin2D	curBin2D() const;
    virtual TrcKey	curTrcKey() const;
    virtual Coord	curCoord() const;
    virtual bool	includes(const Coord&,float) const;
    virtual bool	includes(int,float,int) const;
    virtual void	getExtent(Interval<int>&,int nr = -1) const;
    virtual void	getZRange(Interval<float>& rg, int lidx ) const
			{ return EMSurfaceProvider::getZRange(rg); }
    virtual void	getZRange(Interval<float>& rg ) const
			{ return EMSurfaceProvider::getZRange(rg); }
    int			nrLines() const			{ return 1; }
    GeomID		curGeomID() const override
			{ return curTrcKey().geomID(); }

    static void		initClass();
    static Provider2D*	create()	{ return new EMSurfaceProvider2D; }

    mEMSurfaceProviderDefFnsBase

};


/*!
\brief EMSurfaceProvider for 3D positions with 2D Horizon.
*/

mExpClass(EarthModel) EMSurface2DProvider3D : public Provider3D
					    , public EMSurfaceProvider
{ mODTextTranslationClass(EMSurface2DProvider3D);
public:
				EMSurface2DProvider3D();
				EMSurface2DProvider3D(
					const EMSurface2DProvider3D&);
				~EMSurface2DProvider3D();
    const char*			factoryKeyword() const { return type(); }
    EMSurface2DProvider3D&	operator =(const EMSurface2DProvider3D&);
    virtual bool		initialize(const TaskRunnerProvider&);
    virtual void		reset();
    Provider*			clone() const
				{ return new EMSurface2DProvider3D(*this); }

    virtual BinID		curBinID() const;
    virtual bool		includes(const BinID&,float) const;
    virtual bool		includes( const Coord& c, float z ) const
				{ return Provider3D::includes(c,z); }
    virtual void		getExtent(BinID&,BinID&) const;
    virtual void		getZRange(Interval<float>& rg ) const
				{ return EMSurfaceProvider::getZRange(rg); }
    virtual Coord		curCoord() const
				{ return Provider3D::curCoord(); }
    GeomID			curGeomID() const override
				{ return GeomID::get3D(); }

    const DataPointSet&		dataPointSet( bool nr1 ) const
				{ return nr1 ? dpssurf1_ : dpssurf2_; }

    mEMSurfaceProviderDefFnsBase

protected:

    DataPointSet&		dpssurf1_;
    DataPointSet&		dpssurf2_;

    void			mkDPS(const EM::Surface&,DataPointSet&);

};


/*!
\brief EM implicit body provider for 3D positioning.
*/

mExpClass(EarthModel) EMImplicitBodyProvider : public Provider3D
{ mODTextTranslationClass(EMImplicitBodyProvider);
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

    virtual bool		initialize(const TaskRunnerProvider&);
    virtual void		reset();

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
    GeomID			curGeomID() const override
				{ return GeomID::get3D(); }

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		getSummary(uiString&) const;

    virtual void		getTrcKeyZSampling(TrcKeyZSampling& cs) const;
    const TrcKeyZSampling&	getImpBodyRange() const { return tkzs_; }
    Array3D<float>*		getImpBodyData() const	{ return imparr_; }
    float                       getThreshold() const	{ return threshold_; }

    static const char*		sKeyUseInside()	{ return "Use InsideBody"; }
    static const char*		sKeyBBInlrg()	{ return "BB-Inl range"; }
    static const char*		sKeyBBCrlrg()	{ return "BB-Crl range"; }
    static const char*		sKeyBBZrg()	{ return "BB-Z range"; }

protected:

    bool			isOK() const;

    TrcKeyZSampling		tkzs_;
    Array3D<float>*		imparr_;
    float			threshold_;
    bool			useinside_;
    TrcKeyZSampling		bbox_;

    EM::Body*			embody_;
    BinID			curbid_;
    float			curz_;
    bool			initializedbody_;
};


/*!
\brief EM Region provider for 3D positioning.
*/

mExpClass(EarthModel) EMRegion3DProvider : public Provider3D
{ mODTextTranslationClass(EMRegion3DProvider)
public:

				EMRegion3DProvider();
				EMRegion3DProvider(const EMRegion3DProvider&);
				~EMRegion3DProvider();

    static void			initClass();
    static Provider3D*		create() { return new EMRegion3DProvider; }
    EMRegion3DProvider*	clone() const
				{ return new EMRegion3DProvider(*this); }

    EMRegion3DProvider&	operator =(const EMRegion3DProvider&);
    const char*			type() const		{ return "Region3D"; }
    const char*			factoryKeyword() const	{ return type(); }

    virtual bool		initialize(const TaskRunnerProvider&);
    virtual void		reset();

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
    GeomID			curGeomID() const override
				{ return GeomID::get3D(); }

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		getSummary(uiString&) const;

    virtual void		getTrcKeyZSampling(TrcKeyZSampling& cs) const;

    EM::Region3D&		region()		{ return region_; }
    const EM::Region3D&		region() const		{ return region_; }

protected:


    TrcKeyZSampling		bbox_;

    EM::Region3D&		region_;
    BinID			curbid_;
    float			curz_;
    bool			useinside_;
};

} // namespace Pos
