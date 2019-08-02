#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
 RCS:		$Id: seisdatapack.h 38554 2015-03-18 09:20:03Z mahant.mothey@dgbes.com $
________________________________________________________________________

-*/

#include "seisinfo.h"
#include "datapackbase.h"
#include "posinfo.h"
#include "trckeyzsampling.h"

class BinnedValueSet;
class TraceData;


/*!\brief Seis Volume DataPack base class. */

mExpClass(Seis) SeisVolumeDataPack : public VolumeDataPack
{
public:

    mDeclAbstractMonitorableAssignment(SeisVolumeDataPack);

    void		fillTrace(const TrcKey&,SeisTrc&) const;
    void		fillTraceInfo(const TrcKey&,SeisTrcInfo&) const;
    void		fillTraceData(const TrcKey&,TraceData&) const;

protected:

			SeisVolumeDataPack(const char* cat,const BinDataDesc*);
			~SeisVolumeDataPack();

};


/*!\brief VolumeDataPack for 2D and 3D seismic data. */

mExpClass(Seis) RegularSeisDataPack : public SeisVolumeDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						    const BinDataDesc* bdd=0);
				mDeclMonitorableAssignment(RegularSeisDataPack);

    bool			is2D() const;

    virtual RegularSeisDataPack* getSimilar() const;
    bool			copyFrom(const RegularSeisDataPack&);

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }

    void			setTrcsSampling(PosInfo::CubeData*);
				//!< Becomes mine
    const PosInfo::CubeData*	trcsSampling() const;
				//!< Only returns non-null if explictly set
    void			getTrcPositions(PosInfo::CubeData&) const;

    bool			addComponent(const char* nm,bool initvals);

    glob_size_type		nrTrcs() const { return (glob_size_type)
					sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(glob_idx_type) const;
    glob_idx_type		getGlobalIdx(const TrcKey&) const;

    const StepInterval<float>&	getZRange() const
				{ return sampling_.zsamp_; }

    static DataPack::ID		createDataPackForZSlice(const BinnedValueSet*,
				    const TrcKeyZSampling&,const ZDomain::Info&,
				    const BufferStringSet* nms=0);
				/*!< Creates RegularSeisDataPack from
				BinnedValueSet for z-slices in z-axis transformd
				domain. nrComponents() in the created datapack
				will be one less than BinnedValueSet::nrVals(),
				as the	z-component is not used. \param nms is
				for passing component names. */
protected:

				~RegularSeisDataPack();

    TrcKeyZSampling		sampling_;
    PtrMan<PosInfo::CubeData>	trcssampling_;

    virtual void		doDumpInfo(IOPar&) const;

};


/*!\brief SeisVolumeDataPack for random lines. */

mExpClass(Seis) RandomSeisDataPack : public SeisVolumeDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* bdd=0);
				mDeclMonitorableAssignment(RandomSeisDataPack);
    virtual RandomSeisDataPack*	getSimilar() const;

    bool			is2D() const		{ return false; }
    glob_size_type		nrTrcs() const		{ return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const;
    glob_idx_type		getGlobalIdx(const TrcKey&) const;

    const StepInterval<float>&	getZRange() const	{ return zsamp_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zsamp_ = zrg; }

    void			setPath( const TrcKeyPath& path )
				{ path_ = path; }
    const TrcKeyPath&		getPath() const		{ return path_; }
    TrcKeyPath&			getPath()		{ return path_; }
    void			setRandomLineID(int,
						const TypeSet<BinID>* subpth=0);
    int				getRandomLineID() const	{ return rdlid_; }

    bool			addComponent(const char* nm,bool initvals);

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
						int rdmlineid,
						const Interval<float>& zrg,
						const BufferStringSet* nms=0,
						const TypeSet<BinID>* subpth=0);

protected:

				~RandomSeisDataPack();

    int				rdlid_;
    TrcKeyPath			path_;
    StepInterval<float>		zsamp_;

};


/*!\brief Base class for RegularFlatDataPack and RandomFlatDataPack. */

mExpClass(Seis) SeisFlatDataPack : public FlatDataPack
{
public:

    mUseType( VolumeDataPack,	comp_idx_type );

    mDeclAbstractMonitorableAssignment(SeisFlatDataPack);

    size_type			nrTrcs() const
				{ return source_->nrTrcs(); }
    TrcKey			getTrcKey( idx_type trcidx ) const
				{ return source_->getTrcKey(trcidx); }
    const SeisVolumeDataPack&	getSourceDataPack() const
				{ return *source_; }
    bool			is2D() const
				{ return source_->is2D(); }

    virtual const TrcKeyPath&	getPath() const				= 0;
				//!< Will be empty if isVertical() is false
				//!< Eg: Z-slices. Or if the data corresponds
				//!< to a single trace.
    const StepInterval<float>&	getZRange() const	{ return zSamp(); }
    virtual int			getRandomLineID() const
				{ return source_->getRandomLineID(); }

    bool			dimValuesInInt(const char* keystr) const;
    void			getAltDim0Keys(BufferStringSet&) const;
    double			getAltDim0Value(int ikey,idx_type i0) const;
    void			getAuxInfo(idx_type,idx_type,IOPar&) const;

    const Scaler*		getScaler() const
				{ return source_->getScaler(); }
    const ZDomain::Info&	zDomain() const
				{ return source_->zDomain(); }

protected:

				SeisFlatDataPack(const SeisVolumeDataPack&,
						 comp_idx_type icomp);
				~SeisFlatDataPack();

    virtual void		setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData. Assumes getPath() is
				 not empty. */

    ConstRefMan<SeisVolumeDataPack> source_;
    comp_idx_type		comp_;
    TypeSet<SeisTrcInfo::Fld>	tiflds_;

    const StepInterval<float>&	zSamp() const	{ return source_->getZRange(); }
    virtual float		gtNrKBytes() const;

};


/*!\brief FlatDataPack for 2D and 3D seismic data. */

mExpClass(Seis) RegularFlatDataPack : public SeisFlatDataPack
{
public:
				mTypeDefArrNDTypes;

				RegularFlatDataPack(const RegularSeisDataPack&,
						    comp_idx_type);
				mDeclMonitorableAssignment(RegularFlatDataPack);

    bool			isVertical() const
				{ return dir() != OD::ZSlice; }
    const TrcKeyPath&		getPath() const		{ return path_; }
    float			getPosDistance(bool dim0,float trcfidx) const;

    const TrcKeyZSampling&	sampling() const
				{ return regSource().sampling(); }
    Pos::GeomID			getGeomID() const
				{ return sampling().hsamp_.getGeomID(); }
    OD::SliceType		dir() const
				{ return sampling().defaultDir(); }
    Coord3			getCoord(idx_type,idx_type) const;

    const char*			dimName(bool dim0) const;

protected:

				~RegularFlatDataPack();

    void			setSourceDataFromMultiCubes();
    void			setSourceData();
    void			setTrcInfoFlds();

    TrcKeyPath			path_;
    bool			usemulticomps_;
    bool			hassingletrace_;

    const RegularSeisDataPack&	regSource() const
				{ return (RegularSeisDataPack&)(*source_); }

};


/*!\brief FlatDataPack for random lines. */

mExpClass(Seis) RandomFlatDataPack : public SeisFlatDataPack
{
public:

				RandomFlatDataPack(const RandomSeisDataPack&,
						   comp_idx_type);
				mDeclMonitorableAssignment(RandomFlatDataPack);

    bool			isVertical() const	{ return true; }
    const TrcKeyPath&		getPath() const
				{ return rdlSource().getPath(); }
    Coord3			getCoord(idx_type,idx_type) const;
    float			getPosDistance(bool dim0,float trcfidx) const;

    const char*			dimName( bool dim0 ) const
				{ return dim0 ? "Distance" : "Z"; }

protected:

				~RandomFlatDataPack();

    void			setSourceData();
    void			setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData after regularizing. */

    void			setTrcInfoFlds();

    const RandomSeisDataPack&	rdlSource() const
				{ return (RandomSeisDataPack&)(*source_); }

};
