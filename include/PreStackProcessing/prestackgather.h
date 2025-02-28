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
				       Seis::OffsetType,OD::AngleType,
				       const ZDomain::Info&);
				mDeprecated("Provide OD::AngleType")
				Gather(const FlatPosData&,
				       Seis::OffsetType,
				       const ZDomain::Info&);

    Gather&			operator =(const Gather&)	= delete;

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
    Gather&			setBinID(const BinID&);
    const MultiID&		getStoredID() const	{ return storagemid_; }
    const ZSampling&		zRange() const		{ return zrg_; }
    Gather&			setZRange( const ZSampling& zrg )
				{ zrg_ = zrg; return *this; }

    int				getSeis2DTraceNr() const;
    const char*			getSeis2DName() const;

    bool			isLoaded() const	{ return arr2d_; }

    static int			offsetDim()		{ return 0; }
    static int			zDim()			{ return 1; }

    float			getOffset(int) const;
    float			getAzimuth(int) const;
    OffsetAzimuth		getOffsetAzimuth(int) const;

    bool			isCorrected() const;
    bool			isOffsetAngle() const;
    bool			isOffsetInMeters() const;
    bool			isOffsetInFeet() const;
    Seis::OffsetType		offsetType() const;
    OD::AngleType		azimuthAngleType() const;
    Gather&			setCorrected(bool yn);
    Gather&			setOffsetType(Seis::OffsetType);
    Gather&			setAzimuthAngleType(OD::AngleType);
    Gather&			setZDomain(const ZDomain::Info&) override;

    const MultiID&		getVelocityID() const	{ return velocitymid_; }
    const MultiID&		getStorageID() const    { return storagemid_; }
    const MultiID&		getStaticsID() const	{ return staticsmid_; }

    static bool			getVelocityID(const MultiID& stor,MultiID& vid);

    static const char*		sDataPackCategory();
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

    bool			iscorr_ = true;
    Seis::OffsetType		offsettype_;
    OD::AngleType		azimuthangletype_ = OD::AngleType::Degrees;
    TrcKey			tk_;
    Coord			coord_;
    TypeSet<float>		azimuths_;
    ZSampling			zrg_;

private:
    uiString			dimName(bool dim0) const override;
    uiString			dimUnitLbl(bool dim0,bool display,
					   bool abbreviated=true,
				      bool withparentheses=true) const override;
    const UnitOfMeasure*	dimUnit(bool dim0,bool display) const override;

    TrcKey			getTrcKey(int,int) const override;
    Coord3			getCoord(int itrc,int isamp) const override;
    double			getZ(int itrc,int isamp) const override;
    void			getAuxInfo(int itrc,int idim1,
					   IOPar&) const override;

public:
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool iscorrected,
					    Seis::OffsetType,OD::AngleType,
					    const ZDomain::Info&,
					    bool snapzrangetosi=false);
				mDeprecated("Provide OD::AngleType")
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					    bool iscorrected,
					    Seis::OffsetType,
					    const ZDomain::Info&,
					    bool snapzrangetosi=false);
    bool			setFromTrcBuf(SeisTrcBuf&,int comp,
					      const GatherSetDataPack&,
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

mExpClass(PreStackProcessing) GatherSetDataPack : public ZDataPack
{
public:
				GatherSetDataPack(const char* ctgery);
				GatherSetDataPack(const char* ctgery,
						  const ObjectSet<Gather>&);

    void			fill(Array2D<float>&,int offsetidx) const;
    void			fill(SeisTrcBuf&,int offsetidx) const;
    void			fill(SeisTrcBuf&,Interval<float> stackrg) const;
    SeisTrc*			getTrace(int gatheridx,int offsetidx);
    const SeisTrc*		getTrace(int gatheridx,int offsetidx) const;

    float			nrKBytes() const override	{ return 0.f; }
    int				nrGathers() const;
    Interval<float>		offsetRange() const;
    float			offsetRangeStep() const;

    TrcKey			getTrcKeyByIdx(int idx) const;
    DataPackID			getGatherIDByIdx(int idx) const;
    DataPackID			getGatherID(const TrcKey&) const;
    mDeprecated("Use TrcKey")
    DataPackID			getGatherID(const BinID&) const;
    ConstRefMan<PreStack::Gather> getGather(int gatheridx) const;
    ConstRefMan<PreStack::Gather> getGather(const TrcKey&) const;
    mDeprecated("Use TrcKey")
    ConstRefMan<PreStack::Gather> getGather(const BinID&) const;

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
    OD::AngleType		azimuthAngleType() const;
    GatherSetDataPack&		setCorrected(bool yn);
    GatherSetDataPack&		setOffsetType(Seis::OffsetType);
    GatherSetDataPack&		setAzimuthAngleType(OD::AngleType);
    GatherSetDataPack&		setZDomain(const ZDomain::Info&) override;

    static const char*		sDataPackCategory();

protected:

				~GatherSetDataPack();

private:
				mOD_DisableCopy(GatherSetDataPack);

    SeisTrc*			gtTrace(int gatheridx,int offsetidx) const;

    RefObjectSet<Gather>	gathers_;
    Array3D<float>&		arr3d_;

    bool			iscorr_ = true;
    Seis::OffsetType		offsettype_;
    OD::AngleType		azimuthangletype_ = OD::AngleType::Degrees;

public:

    void			obtainGathers();
				/*!< Make all gathers available in the
				  FlatDataPack Mgr */
};

} // namespace PreStack
