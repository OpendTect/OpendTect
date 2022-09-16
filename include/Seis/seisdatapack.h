#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datapackbase.h"
#include "integerid.h"
#include "seisinfo.h"
#include "trckeyzsampling.h"

class BinIDValueSet;
class TraceData;
namespace PosInfo { class CubeData; }

/*!
\brief SeisDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularSeisDataPack : public SeisDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						   const BinDataDesc* =nullptr);

    RegularSeisDataPack*	clone() const;
    RegularSeisDataPack*	getSimilar() const;
    bool			copyFrom(const RegularSeisDataPack&);

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }
    ZSampling			zRange() const override
				{ return sampling_.zsamp_; }

    void			setTrcsSampling(PosInfo::CubeData*);
				//!<Becomes mine
    const PosInfo::CubeData*	getTrcsSampling() const;
				//!<Only for 3D
    bool			is2D() const override;

    bool			addComponent(const char* nm) override;
    bool			addComponentNoInit(const char* nm);

    int				nrTrcs() const override
				{ return (int)sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(int globaltrcidx) const override;
    int				getGlobalIdx(const TrcKey&) const override;

    void			dumpInfo(StringPairSet&) const override;

    static DataPackID		createDataPackForZSlice(const BinIDValueSet*,
						const TrcKeyZSampling&,
						const ZDomain::Info&,
					    const BufferStringSet* nms=nullptr);
				/*!< Creates RegularSeisDataPack from
				BinIDValueSet for z-slices in z-axis transformed
				domain. nrComponents() in the created datapack
				will be one less than BinIDValueSet::nrVals(),
				as the	z-component is not used. \param nms is
				for passing component names. */

    void			fillTrace(const TrcKey&,SeisTrc&) const;
    void			fillTraceInfo(const TrcKey&,SeisTrcInfo&) const;
    void			fillTraceData(const TrcKey&,TraceData&) const;

protected:
				~RegularSeisDataPack();

    TrcKeyZSampling		sampling_;
    PosInfo::CubeData*		rgldpckposinfo_;
};


/*!
\brief SeisDataPack for random lines.
*/

mExpClass(Seis) RandomSeisDataPack : public SeisDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* =nullptr);

    bool			is2D() const override	{ return false; }
    int				nrTrcs() const override { return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const override;
    int				getGlobalIdx(const TrcKey&) const override;

    ZSampling			zRange() const override { return zsamp_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zsamp_ = zrg; }

    void			setPath( const TrcKeyPath& path )
				{ path_ = path; }
    const TrcKeyPath&		getPath() const		{ return path_; }

    bool			addComponent(const char* nm) override;

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
						   const TrcKeyPath& path,
						   const Interval<float>& zrg);

protected:
				~RandomSeisDataPack();

    TrcKeyPath			path_;
    ZSampling			zsamp_;

public:
    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
						   RandomLineID rdmlineid,
						   const Interval<float>& zrg);

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
					       RandomLineID rdmlineid,
					       const Interval<float>& zrg,
					       const BufferStringSet* nms);

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
					       const TrcKeyPath& path,
					       const Interval<float>& zrg,
					       const BufferStringSet* nms);

    TrcKeyPath&			getPath()		{ return path_; }
};


/*!
\brief Base class for RegularFlatDataPack and RandomFlatDataPack.
*/

mExpClass(Seis) SeisFlatDataPack : public FlatDataPack
{
public:

    int				nrTrcs() const;
    TrcKey			getTrcKey(int trcidx) const;
    DataPackID		getSourceID() const;
    int				getSourceGlobalIdx(const TrcKey&) const;

    bool			is2D() const;

    virtual bool		isVertical() const			= 0;
    virtual const TrcKeyPath&	getPath() const				= 0;
				//!< Will be empty if isVertical() is false
				//!< Eg: Z-slices. Or if the data corresponds
				//!< to a single trace.
    ZSampling			zRange() const		{ return zsamp_; }

    bool			dimValuesInInt(
					const char* keystr) const override;
    void			getAltDim0Keys(BufferStringSet&) const override;
    double			getAltDim0Value(int ikey,int i0) const override;
    void			getAuxInfo(int i0,int i1,IOPar&) const override;

    const Scaler*		getScaler() const;
    const ZDomain::Info&	zDomain() const;
    float			nrKBytes() const override;
    RandomLineID		getRandomLineID() const;

protected:

				SeisFlatDataPack(const SeisDataPack&,int comp);
				~SeisFlatDataPack();

    virtual void		setSourceData()				= 0;
    virtual void		setTrcInfoFlds()			= 0;
    void			setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData. Assumes getPath() is
				 not empty. */

    ConstRefMan<SeisDataPack>	source_;
    int				comp_;
    const StepInterval<float>	zsamp_;

    TypeSet<SeisTrcInfo::Fld>	tiflds_;
    RandomLineID		rdlid_;
};


/*!
\brief FlatDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularFlatDataPack : public SeisFlatDataPack
{
public:
				RegularFlatDataPack(const RegularSeisDataPack&,
						    int component);

    bool			isVertical() const override
				{ return dir_ != TrcKeyZSampling::Z; }
    const TrcKeyPath&		getPath() const override { return path_; }
    float			getPosDistance(bool dim0,
					       float trcfidx) const override;

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }
    Coord3			getCoord(int i0,int i1) const override;

    const char*			dimName(bool dim0) const override;

protected:
				~RegularFlatDataPack();

    void			setSourceDataFromMultiCubes();
    void			setSourceData() override;
    void			setTrcInfoFlds() override;

    TrcKeyPath			path_;
    const TrcKeyZSampling&	sampling_;
    TrcKeyZSampling::Dir	dir_;
    bool			usemulticomps_;
    bool			hassingletrace_;
};


/*!
\brief FlatDataPack for random lines.
*/

mExpClass(Seis) RandomFlatDataPack : public SeisFlatDataPack
{
public:
				RandomFlatDataPack(const RandomSeisDataPack&,
						   int component);

    bool			isVertical() const override	{ return true; }
    int				getNearestGlobalIdx(const TrcKey&) const;
    const TrcKeyPath&		getPath() const override   { return path_; }
    Coord3			getCoord(int i0,int i1) const override;
    float			getPosDistance(bool dim0,
					       float trcfidx) const override;

    const char*			dimName( bool dim0 ) const override
				{ return dim0 ? "Distance" : "Z"; }

protected:
				~RandomFlatDataPack();

    void			setSourceData() override;
    void			setRegularizedPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData after regularizing. */

    void			setTrcInfoFlds() override;
    const TrcKeyPath&		path_;
};
