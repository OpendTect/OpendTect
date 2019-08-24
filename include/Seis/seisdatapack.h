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
class DataCharacteristics;
class TraceData;
class TrcKeyPath;


/*!\brief Seis Volume DataPack base class. */

mExpClass(Seis) SeisVolumeDataPack : public VolumeDataPack
{
public:

    mDeclAbstractMonitorableAssignment(SeisVolumeDataPack);

    bool		isFullyCompat(const z_steprg_type&,
				      const DataCharacteristics&) const;

			// following will just fill with all available data
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

    mUseType( PosInfo,		LineCollData );

				RegularSeisDataPack(const char* cat,
						    const BinDataDesc* bdd=0);
				mDeclMonitorableAssignment(RegularSeisDataPack);

    bool			is2D() const override;

    RegularSeisDataPack*	getSimilar() const override;
    bool			copyFrom(const RegularSeisDataPack&);

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }
    const TrcKeySampling&	hSamp() const
				{ return sampling_.hsamp_; }
    z_steprg_type		zRange() const override
				{ return sampling_.zsamp_; }

    void			setTrcsSampling(LineCollData*);
				//!< Becomes mine
    const LineCollData*		trcsSampling() const;
				//!< Only returns non-null if explictly set
    void			getTrcPositions(PosInfo::LineCollData&) const;

    bool			addComponent(const char* nm,bool initvals);

    glob_size_type		nrPositions() const override
				{ return (glob_size_type)hSamp().totalNr(); }
    void			getTrcKey(glob_idx_type,TrcKey&) const override;
    glob_idx_type		globalIdx(const TrcKey&) const override;

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
    PtrMan<LineCollData>	lcd_;

    void			doDumpInfo(IOPar&) const override;

};


/*!\brief SeisVolumeDataPack for random lines. */

mExpClass(Seis) RandomSeisDataPack : public SeisVolumeDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* bdd=0);
				mDeclMonitorableAssignment(RandomSeisDataPack);
    RandomSeisDataPack*		getSimilar() const override;

    bool			is2D() const override;
    glob_size_type		nrPositions() const	{ return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const;
    glob_idx_type		globalIdx(const TrcKey&) const override;

    z_steprg_type		zRange() const override	{ return zsamp_; }
    void			setZRange( const z_steprg_type& zrg )
				{ zsamp_ = zrg; }

    void			setPath(const TrcKeyPath&);
    const TrcKeyPath&		path() const		{ return path_; }
    void			getPath(TrcKeyPath&) const override;
    void			setRandomLineID(int,
						const TypeSet<BinID>* subpth=0);
    rdl_id			randomLineID() const	{ return rdlid_; }

    bool			addComponent(const char* nm,bool initvals);

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
						int rdmlineid,
						const z_rg_type& zrg,
						const BufferStringSet* nms=0,
						const TypeSet<BinID>* subpth=0);

protected:

				~RandomSeisDataPack();

    int				rdlid_;
    TrcKeyPath&			path_;
    z_steprg_type		zsamp_;

};


/*!\brief Base class for RegularFlatDataPack and RandomFlatDataPack. */

mExpClass(Seis) SeisFlatDataPack : public FlatDataPack
{
public:

    mUseType( VolumeDataPack,	comp_idx_type );
    mUseType( VolumeDataPack,	z_steprg_type );

    mDeclAbstractMonitorableAssignment(SeisFlatDataPack);

    glob_size_type		nrPositions() const override
				{ return source_->nrPositions(); }
    void			getTrcKey( idx_type trcidx,
					   TrcKey& tk ) const override
				{ return source_->getTrcKey(trcidx,tk); }
    const SeisVolumeDataPack&	getSourceDataPack() const
				{ return *source_; }
    bool			is2D() const override
				{ return source_->is2D(); }

    void			getPath(TrcKeyPath&) const override;
    const TrcKeyPath&		path() const override;
				//!< Will be empty if isVertical() is false
				//!< Eg: Z-slices. Or if the data corresponds
				//!< to a single trace.
    z_steprg_type		zRange() const override
				{ return source_->getZRange(); }
    int				randomLineID() const override
				{ return source_->randomLineID(); }

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
    const TrcKeyPath&		path() const		{ return path_; }
    float			getPosDistance(bool dim0,float trcfidx) const;

    const TrcKeyZSampling&	sampling() const
				{ return regSource().sampling(); }
    Pos::GeomID			geomID() const
				{ return hSamp().getGeomID(); }
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
    const TrcKeyPath&		path() const
				{ return rdlSource().path(); }
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
