#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "arrayndimpl.h"
#include "datapackbase.h"
#include "multiid.h"
#include "odcommonenums.h"
#include "offsetazimuth.h"
#include "position.h"
#include "samplingdata.h"

class IOObj;
class SeisPSReader;
class SeisTrc;
class SeisTrcBuf;
namespace ZDomain { class Info; }

namespace PreStack
{

class GatherSetDataPack;

/*!
\brief PreStack gather.
*/

mExpClass(PreStackProcessing) Gather : public FlatDataPack
{ mODTextTranslationClass(Gather)
public:
				Gather();
				Gather(const Gather&);
				Gather(const FlatPosData&,
				       Seis::OffsetType,
				       const ZDomain::Info&);
		mDeprecated("Provide Seis::OffsetType and ZDomain::Info")
				Gather(const FlatPosData&);

    bool			is3D() const { return tk_.is3D(); }
    bool			is2D() const { return tk_.is2D(); }
    bool			isSynthetic() const { return tk_.isSynthetic();}

    bool			readFrom(const MultiID&,const TrcKey&,
					 int component=0,
					 uiString* errmsg=nullptr);
    bool			readFrom(const IOObj&,const TrcKey&,
					 int component=0,
					 uiString* errmsg=nullptr);
    bool			readFrom(const IOObj&,SeisPSReader& rdr,
					 const TrcKey&,int component=0,
					 uiString* errmsg=nullptr);
    mDeprecated("Use TrcKey")
    bool			readFrom(const IOObj&,SeisPSReader& rdr,
					 const BinID&,int component=0,
					 uiString* errmsg=nullptr);
				//!< Will use the reader geomID

    const Coord&		getCoord() const	{ return coord_; }
    Coord3			getCoord(int,int) const override
				{ return Coord3(coord_.x,coord_.y,0); }

    void                        detectOuterMutes(int* depths,
						 int taperlen=0) const;
				/*!<For each trace, find the depth where the
				    last outer-mute-affecte value is*/
    void                        detectInnerMutes(int* depths,
						 int taperlen=0) const;
	                        /*<!For each trace, try to detect the first
				   inner-mute affected value. */

    const TrcKey&		getTrcKey() const	{ return tk_; }
    Gather&			setTrcKey( const TrcKey& tk )
				{ tk_ = tk; return *this; }

				//for 3d only
    const BinID&		getBinID() const;
    void			setBinID(const BinID&);
    const MultiID&		getStoredID() const	{ return storagemid_; }
    const ZSampling&		zRange() const		{ return zrg_; }
    Gather&			setZRange( const ZSampling& zrg )
				{ zrg_ = zrg; return *this; }

    int				getSeis2DTraceNr() const;
    const char*			getSeis2DName() const;

    bool			isLoaded() const	{ return arr2d_; }

    const char*			dimName(bool dim0) const override;
    void			getAuxInfo(int,int,IOPar&) const override;

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    bool			isCorrected() const	{ return iscorr_; }
    bool			isOffsetAngle() const	{return offsetisangle_;}
    bool			isOffsetInMeters() const;
    bool			isOffsetInFeet() const;
    Seis::OffsetType		offsetType() const;
    const ZDomain::Info&	zDomain() const;
    bool			zIsTime() const		{ return zit_; }
    bool			zInMeter() const;
    bool			zInFeet() const;
    void			setCorrected( bool yn ) { iscorr_ = yn; }
    Gather&			setOffsetType(Seis::OffsetType);
    Gather&			setZDomain(const ZDomain::Info&);

    mDeprecated("Use setOffsetType")
    void			setOffsetIsAngle(bool yn);

    const MultiID&		getVelocityID() const	{ return velocitymid_; }
    const MultiID&		getStorageID() const    { return storagemid_; }
    const MultiID&		getStaticsID() const	{ return staticsmid_; }

    static bool			getVelocityID(const MultiID& stor,MultiID& vid);

    static const char*		sDataPackCategory();
    static const char*		sKeyIsAngleGather();
    static const char*		sKeyIsCorr();
    static const char*		sKeyZisTime();

    static const char*		sKeyPostStackDataID();
    static const char*		sKeyStaticsID();

    void			getAzimuths( TypeSet<float>& azimuths ) const
				{ azimuths = azimuths_; }
    void			setAzimuths( const TypeSet<float>& azimuths )
				{ azimuths_ = azimuths; }

protected:
				~Gather();

    MultiID			velocitymid_;
    MultiID			storagemid_;
    MultiID			staticsmid_;
    bool			offsetisangle_; //deprecated
    bool			iscorr_;

    bool			zit_; //deprecated
    TrcKey			tk_;
    Coord			coord_;
    TypeSet<float>		azimuths_;
    ZSampling			zrg_;

public:
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool iscorrected,
					    Seis::OffsetType,
					    const ZDomain::Info&,
					    bool snapzrangetosi=false);
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					      const GatherSetDataPack&,
					      bool snapzrangetosi=false);

    mDeprecated("Provide GatherSetDataPack")
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool snapzrangetosi=false);

    mDeprecated("Use TrcKey")
    bool			readFrom(const MultiID&,const BinID&,
					 int component=0,
					 uiString* errmsg=nullptr);
    mDeprecated("Use TrcKey")
    bool			readFrom(const IOObj&,const BinID&,
					 int component=0,
					 uiString* errmsg=nullptr);

    mDeprecated("Use TrcKey")
    bool			readFrom(const MultiID&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=nullptr);

    mDeprecated("Use TrcKey")
    bool			readFrom(const IOObj&, const int tracenr,
					 const char* linename,int comp,
					 uiString* errmsg=nullptr);
};


/*!
\brief A DataPack containing a set of gathers.
*/

mExpClass(PreStackProcessing) GatherSetDataPack : public DataPack
{
public:
				GatherSetDataPack(const char* ctgery);
				GatherSetDataPack(const char* ctgery,
						  const ObjectSet<Gather>&);

    void			fill(Array2D<float>&,int offsetidx) const;
    void			fill(SeisTrcBuf&,int offsetidx) const;
    void			fill(SeisTrcBuf&,Interval<float> stackrg) const;
    ConstRefMan<PreStack::Gather> getGather(int gatheridx) const;
    SeisTrc*			getTrace(int gatheridx,int offsetidx);
    const SeisTrc*		getTrace(int gatheridx,int offsetidx) const;

    float			nrKBytes() const override	{ return 0.f; }
    int				nrGathers() const;
    Interval<float>		offsetRange() const;
    float			offsetRangeStep() const;

    TrcKey			getTrcKeyByIdx(int idx) const;
    DataPackID			getGatherIDByIdx(int idx) const;
    DataPackID			getGatherID(const BinID&) const;

    const Array3D<float>&	data() const		{ return arr3d_; }

    void			addGather(PreStack::Gather&);
				//!< Gather becomes mine
    void			finalize();
				//!< Once when all gathers have been added
    void			setName(const char*) override;

    ZSampling			zRange() const;

    bool			isCorrected() const;
    bool			isOffsetAngle() const;
    bool			isOffsetInMeters() const;
    bool			isOffsetInFeet() const;
    Seis::OffsetType		offsetType() const;
    const ZDomain::Info&	zDomain() const;
    bool			zIsTime() const;
    bool			zInMeter() const;
    bool			zInFeet() const;
    GatherSetDataPack&		setCorrected(bool yn);
    GatherSetDataPack&		setOffsetType(Seis::OffsetType);
    GatherSetDataPack&		setZDomain(const ZDomain::Info&);

    static const char*		sDataPackCategory();

protected:

				~GatherSetDataPack();

private:
				mOD_DisableCopy(GatherSetDataPack);

    SeisTrc*			gtTrace(int gatheridx,int offsetidx) const;

    RefObjectSet<Gather>	gathers_;
    Array3D<float>&		arr3d_;

public:

    void			obtainGathers();
				/*!< Make all gathers available in the
				  FlatDataPack Mgr */
};

} // namespace PreStack
