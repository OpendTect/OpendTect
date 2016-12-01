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

class BinIDValueSet;


/*!\brief SeisDataPack for random lines. */

mExpClass(Seis) SeisVolumeDataPack : public VolumeDataPack
{
public:

    void			fillTrace(const TrcKey&,SeisTrc&) const;

protected:

				SeisVolumeDataPack( const char* cat,
						    const BinDataDesc* bdd )
				    : VolumeDataPack(cat,bdd)	{}

};


/*!\brief VolumeDataPack for 2D and 3D seismic data. */

mExpClass(Seis) RegularSeisDataPack : public SeisVolumeDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						    const BinDataDesc* bdd=0);
				~RegularSeisDataPack();
    RegularSeisDataPack*	getSimilar() const;

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }

    void			setTrcsSampling(const PosInfo::CubeData*);
				//!<Becomes mine
    const PosInfo::CubeData*	getTrcsSampling() const;
				//!<Only for 3D
    bool			is2D() const;

    bool			addComponent(const char* nm);

    int				nrTrcs() const
				{ return (int)sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(int globaltrcidx) const;
    int				getGlobalIdx(const TrcKey&) const;

    virtual void		dumpInfo(IOPar&) const;

    const StepInterval<float>&	getZRange() const
				{ return sampling_.zsamp_; }

    static DataPack::ID		createDataPackForZSlice(const BinIDValueSet*,
						const TrcKeyZSampling&,
						const ZDomain::Info&,
						const BufferStringSet& nms=0);
				/*!< Creates RegularSeisDataPack from
				BinIDValueSet for z-slices in z-axis transformed
				domain. nrComponents() in the created datapack
				will be one less than BinIDValueSet::nrVals(),
				as the	z-component is not used. \param nms is
				for passing component names. */
protected:

    TrcKeyZSampling		sampling_;
    PtrMan<const PosInfo::CubeData>	trcssampling_;
};


/*!\brief SeisVolumeDataPack for random lines. */

mExpClass(Seis) RandomSeisDataPack : public SeisVolumeDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* bdd=0);
    RandomSeisDataPack*		getSimilar() const;

    bool			is2D() const		{ return false; }
    int				nrTrcs() const		{ return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const;
    int				getGlobalIdx(const TrcKey&) const;

    const StepInterval<float>&	getZRange() const	{ return zsamp_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zsamp_ = zrg; }

    void			setPath( const TrcKeyPath& path )
				{ path_ = path; }
    const TrcKeyPath&		getPath() const		{ return path_; }
    TrcKeyPath&			getPath()		{ return path_; }
    void			setRandomLineID(int);
    int				getRandomLineID() const	{ return rdlid_; }

    bool			addComponent(const char* nm);

    static DataPack::ID		createDataPackFrom(const RegularSeisDataPack&,
						int rdmlineid,
						const Interval<float>& zrg,
						const BufferStringSet* nms=0);

protected:

    int				rdlid_;
    TrcKeyPath			path_;
    StepInterval<float>		zsamp_;

};


/*!\brief Base class for RegularFlatDataPack and RandomFlatDataPack. */

mExpClass(Seis) SeisFlatDataPack : public FlatDataPack
{
public:

				~SeisFlatDataPack();

    int				nrTrcs() const
				{ return source_.nrTrcs(); }
    TrcKey			getTrcKey( int trcidx ) const
				{ return source_.getTrcKey(trcidx); }
    const SeisVolumeDataPack&	getSourceDataPack() const
				{ return source_; }
    bool			is2D() const
				{ return source_.is2D(); }

    virtual const TrcKeyPath&	getPath() const				= 0;
				//!< Will be empty if isVertical() is false
				//!< Eg: Z-slices. Or if the data corresponds
				//!< to a single trace.
    int				getRandomLineID() const	{ return rdlid_; }
    const StepInterval<float>&	getZRange() const	{ return zsamp_; }

    bool			dimValuesInInt(const char* keystr) const;
    void			getAltDim0Keys(BufferStringSet&) const;
    double			getAltDim0Value(int ikey,int i0) const;
    void			getAuxInfo(int i0,int i1,IOPar&) const;

    const Scaler*		getScaler() const
				{ return source_.getScaler(); }
    const ZDomain::Info&	zDomain() const
				{ return source_.zDomain(); }
    float			nrKBytes() const;

protected:

				SeisFlatDataPack(const SeisVolumeDataPack&,
						 int icomp);

    virtual void		setSourceData()				= 0;
    virtual void		setTrcInfoFlds()			= 0;
    void			setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData. Assumes getPath() is
				 not empty. */

    const SeisVolumeDataPack&	source_;
    int				comp_;
    int				rdlid_;
    const StepInterval<float>&	zsamp_;

    TypeSet<SeisTrcInfo::Fld>	tiflds_;
};


/*!\brief FlatDataPack for 2D and 3D seismic data. */

mExpClass(Seis) RegularFlatDataPack : public SeisFlatDataPack
{
public:

				RegularFlatDataPack(const RegularSeisDataPack&,
						    int component);

    bool			isVertical() const
				{ return dir_ != TrcKeyZSampling::Z; }
    const TrcKeyPath&		getPath() const		{ return path_; }

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }
    Coord3			getCoord(int i0,int i1) const;

    const char*			dimName(bool dim0) const;

protected:

    void			setSourceDataFromMultiCubes();
    void			setSourceData();
    void			setTrcInfoFlds();

    TrcKeyPath			path_;
    const TrcKeyZSampling&	sampling_;
    TrcKeyZSampling::Dir	dir_;
    bool			usemulticomps_;
    bool			hassingletrace_;

};


/*!\brief FlatDataPack for random lines. */

mExpClass(Seis) RandomFlatDataPack : public SeisFlatDataPack
{
public:

				RandomFlatDataPack(const RandomSeisDataPack&,
						   int component);

    bool			isVertical() const	{ return true; }
    const TrcKeyPath&		getPath() const		{ return path_; }
    Coord3			getCoord(int i0,int i1) const;

    const char*			dimName( bool dim0 ) const
				{ return dim0 ? "Distance" : "Z"; }

protected:

    void			setSourceData();
    void			setTrcInfoFlds();
    const TrcKeyPath&		path_;

};
