#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
 RCS:		$Id: seisdatapack.h 38554 2015-03-18 09:20:03Z mahant.mothey@dgbes.com $
________________________________________________________________________

-*/

#include "seismod.h"

#include "datapackbase.h"
#include "trckeyzsampling.h"
#include "seisinfo.h"

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
						    const BinDataDesc* bdd=0);
				~RegularSeisDataPack();

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

    void			dumpInfo(IOPar&) const override;

    static DataPack::ID		createDataPackForZSlice(const BinIDValueSet*,
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
						   const BinDataDesc* bdd=0);

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

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
						   const TrcKeyPath& path,
						   const Interval<float>& zrg);

protected:

    TrcKeyPath			path_;
    ZSampling			zsamp_;

public:
    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
						   int rdmlineid,
						   const Interval<float>& zrg);

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
					       int rdmlineid,
					       const Interval<float>& zrg,
					       const BufferStringSet* nms);

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
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
				~SeisFlatDataPack();

    int				nrTrcs() const
				{ return source_.nrTrcs(); }
    TrcKey			getTrcKey( int trcidx ) const
				{ return source_.getTrcKey(trcidx); }
    const SeisDataPack&		getSourceDataPack() const
				{ return source_; }
    bool			is2D() const
				{ return source_.is2D(); }

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

    const Scaler*		getScaler() const
				{ return source_.getScaler(); }
    const ZDomain::Info&	zDomain() const
				{ return source_.zDomain(); }
    float			nrKBytes() const override;
    int				getRandomLineID() const;

protected:

				SeisFlatDataPack(const SeisDataPack&,int comp);

    virtual void		setSourceData()				= 0;
    virtual void		setTrcInfoFlds()			= 0;
    void			setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData. Assumes getPath() is
				 not empty. */

    const SeisDataPack&		source_;
    int				comp_;
    const StepInterval<float>	zsamp_;

    TypeSet<SeisTrcInfo::Fld>	tiflds_;
    int				rdlid_;
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
    const TrcKeyPath&		getPath() const override   { return path_; }
    Coord3			getCoord(int i0,int i1) const override;
    float			getPosDistance(bool dim0,
					       float trcfidx) const override;

    const char*			dimName( bool dim0 ) const override
				{ return dim0 ? "Distance" : "Z"; }

protected:

    void			setSourceData() override;
    void			setRegularizedPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData after regularizing. */

    void			setTrcInfoFlds() override;
    const TrcKeyPath&		path_;
};

