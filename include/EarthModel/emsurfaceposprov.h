#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "posprovider.h"

#include "datapointset.h"
#include "embody.h"
#include "emposid.h"
#include "keystrs.h"
#include "multiid.h"
#include "sortedlist.h"
#include "trckeysampling.h"
#include <tuple>


namespace EM { class RowColIterator; class Region3D; class Surface; }
namespace Stats { class RandGen; }

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
			~EMSurfaceProvider();

    const char*		type() const override;	//!< sKey::Surface()

    bool		initialize(TaskRunner* tr=0) override;
    void		reset() override;

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual float	curZ() const;
    bool		hasZAdjustment() const override;
    float		adjustedZ(const Coord&,float) const override;

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    bool		getZRange(const TrcKey&,Interval<float>&) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const { return estnrpos_; }
    virtual int		estNrZPerPos() const;

    int			nrSurfaces() const;
    MultiID		surfaceID( int idx ) const
			{ return idx ? id2_ : id1_; }
    EM::Surface*	surface( int idx )
			{ return idx ? surf2_.ptr() : surf1_.ptr(); }
    const EM::Surface*	surface( int idx ) const
			{ return idx ? surf2_.ptr() : surf1_.ptr(); }
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
    RefMan<EM::Surface> surf1_;
    RefMan<EM::Surface> surf2_;
    float		zstep_;
    Interval<float>	extraz_;
    TrcKeySampling	hs_;
    Interval<float>	zrg1_;
    Interval<float>	zrg2_;
    od_int64		estnrpos_ = -1;
    Stats::RandGen&	gen_;

    EM::RowColIterator* iterator_ = nullptr;
    EM::PosID		curpos_;
    Interval<float>	curzrg_;
    float		curz_;
    int			nrsamples_;
    bool		dorandom_ = false;
    bool		enoughsamples_ = true;
    od_int64		maxidx_ = 0;
    SortedList<postuple> posindexlst_;
};


#define mEMSurfaceProviderDefFnsBase \
    bool	isProvider() const override { return true; } \
    float	estRatio( const Provider& p ) const override \
			{ return Provider::estRatio(p); } \
    bool	toNextPos() override \
			{ return EMSurfaceProvider::toNextPos(); } \
    bool	toNextZ() override \
			{ return EMSurfaceProvider::toNextZ(); } \
    float	curZ() const override \
			{ return EMSurfaceProvider::curZ(); } \
    int		estNrZPerPos() const override \
			{ return EMSurfaceProvider::estNrZPerPos(); } \
    od_int64	estNrPos() const override { return estnrpos_; } \

/*!
\brief EMSurfaceProvider for 3D positioning.
*/

mExpClass(EarthModel) EMSurfaceProvider3D : public Provider3D
					  , public EMSurfaceProvider
{ mODTextTranslationClass(EMSurfaceProvider3D);
public:
			EMSurfaceProvider3D();
			EMSurfaceProvider3D(const EMSurfaceProvider3D&);
			~EMSurfaceProvider3D();

    const char*		factoryKeyword() const override { return type(); }
    EMSurfaceProvider3D& operator =( const EMSurfaceProvider3D& p )
			{ copyFrom(p); return *this; }
    Provider*		clone() const override
			{ return new EMSurfaceProvider3D(*this); }

    BinID		curBinID() const override;
    bool		includes(const BinID&,float) const override;
    bool		includes( const Coord& c, float z ) const override
			{ return Provider3D::includes(c,z); }
    void		getExtent(BinID&,BinID&) const override;
    Coord		curCoord() const override
			{ return Provider3D::curCoord(); }
    void		getTrcKeyZSampling( TrcKeyZSampling& cs ) const override
			{ return Provider3D::getTrcKeyZSampling(cs); }
    void		getZRange(Interval<float>& rg ) const override
			{ return EMSurfaceProvider::getZRange(rg); }

    static void		initClass();
    static Provider3D*	create()	{ return new EMSurfaceProvider3D; }

    mEMSurfaceProviderDefFnsBase

};


/*!
\brief EMSurfaceProvider for 2D positioning.
*/

mExpClass(EarthModel) EMSurfaceProvider2D : public Provider2D
			  , public EMSurfaceProvider
{ mODTextTranslationClass(EMSurfaceProvider2D);
public:
			EMSurfaceProvider2D();
			EMSurfaceProvider2D(const EMSurfaceProvider2D&);
			~EMSurfaceProvider2D();

    EMSurfaceProvider2D& operator =( const EMSurfaceProvider2D& p )
			{ copyFrom(p); return *this; }
    const char*		factoryKeyword() const override { return type(); }
    Provider*		clone() const override
			{ return new EMSurfaceProvider2D(*this); }

    virtual const char*	curLine() const;
    Pos::GeomID	curGeomID() const override;
    int		curNr() const override;
    TrcKey	curTrcKey() const override;
    Coord	curCoord() const override;
    bool	includes(const Coord&,float) const override;
    bool	includes(int,float,int) const override;
    void	getExtent(Interval<int>&,int nr = -1) const override;
    void	getZRange(Interval<float>& rg, int lidx ) const override
			{ return EMSurfaceProvider::getZRange(rg); }
    void	getZRange(Interval<float>& rg ) const override
			{ return EMSurfaceProvider::getZRange(rg); }
    int			nrLines() const			{ return 1; }

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

    const char*		factoryKeyword() const override { return type(); }
    EMSurface2DProvider3D&	operator =( const EMSurface2DProvider3D& p );
				//{ copyFrom(p); return *this; }
    bool		initialize(TaskRunner* tr=0) override;
    Provider*		clone() const override
				{ return new EMSurface2DProvider3D(*this); }

    BinID		curBinID() const override;
    bool		includes(const BinID&,float) const override;
    bool		includes( const Coord& c, float z ) const override
				{ return Provider3D::includes(c,z); }
    void		getExtent(BinID&,BinID&) const override;
    void		getZRange(Interval<float>& rg ) const override
				{ return EMSurfaceProvider::getZRange(rg); }
    Coord		curCoord() const override
				{ return Provider3D::curCoord(); }

    mEMSurfaceProviderDefFnsBase

protected:
    RefMan<DataPointSet>	dpssurf1_;
    RefMan<DataPointSet>	dpssurf2_;

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
    EMImplicitBodyProvider*	clone() const override
				{ return new EMImplicitBodyProvider(*this); }

    EMImplicitBodyProvider&	operator =(const EMImplicitBodyProvider&);
    const char*		type() const override		{ return sKey::Body(); }
    const char*		factoryKeyword() const override { return type(); }

    bool		initialize(TaskRunner* tr=0) override;
    void		reset() override		{ initialize(); }

    BinID		curBinID() const override	{ return curbid_; }
    float		curZ() const override		{ return curz_; }

    bool		toNextPos() override;
    bool		toNextZ() override;
    int			estNrZPerPos() const override;
    od_int64		estNrPos() const override;
    void		getExtent(BinID&,BinID&) const override;
    void		getZRange(Interval<float>&) const override;
    bool		includes(const Coord& c,float z) const override;
    bool		includes(const BinID&,float) const override;

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getTrcKeyZSampling(TrcKeyZSampling& cs) const override;
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
    EMRegion3DProvider* clone() const override
				{ return new EMRegion3DProvider(*this); }

    EMRegion3DProvider&	operator =(const EMRegion3DProvider&);
    const char*		type() const override		{ return "Region3D"; }
    const char*		factoryKeyword() const override { return type(); }

    bool		initialize(TaskRunner* tr=0) override;
    void		reset() override		{ initialize(); }

    BinID		curBinID() const override	{ return curbid_; }
    float		curZ() const override		{ return curz_; }

    bool		toNextPos() override;
    bool		toNextZ() override;
    int			estNrZPerPos() const override;
    od_int64		estNrPos() const override;
    void		getExtent(BinID&,BinID&) const override;
    void		getZRange(Interval<float>&) const override;
    bool		includes(const Coord& c,float z) const override;
    bool		includes(const BinID&,float) const override;

    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getTrcKeyZSampling(TrcKeyZSampling& cs) const override;

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
