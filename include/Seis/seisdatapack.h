#ifndef seisdatapack_h
#define seisdatapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
 RCS:		$Id: seisdatapack.h 38554 2015-03-18 09:20:03Z mahant.mothey@dgbes.com $
________________________________________________________________________

-*/

#include "seismod.h"

#include "bindatadesc.h"
#include "datapackbase.h"
#include "trckeyzsampling.h"

template <class T> class Array3DImpl;

namespace ZDomain { class Info; }

/*!
\brief SeisDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularSeisDataPack : public SeisDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						    const BinDataDesc* bdd=0);

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }
    bool			is2D() const;

    bool			addComponent(const char* nm);

    int				nrTrcs() const
				{ return (int)sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(int globaltrcidx) const;
    int				getGlobalIdx(const TrcKey& tk) const;

    virtual void		dumpInfo(IOPar&) const;

    const StepInterval<float>&	getZRange() const
				{ return sampling_.zsamp_; }


protected:

    TrcKeyZSampling		sampling_;
};


/*!
\brief SeisDataPack for random lines.
*/

mExpClass(Seis) RandomSeisDataPack : public SeisDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* bdd=0);

    bool			is2D() const		{ return false; }
    int				nrTrcs() const		{ return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const;
    int				getGlobalIdx( const TrcKey& tk ) const
				{ return path_.indexOf( tk ); }

    const StepInterval<float>&	getZRange() const	{ return zsamp_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zsamp_ = zrg; }

    void			setPath( const TrcKeyPath& path )
				{ path_ = path; }
    const TrcKeyPath&		getPath() const		{ return path_; }

    bool			addComponent(const char* nm);

protected:

    TrcKeyPath			path_;
    StepInterval<float>		zsamp_;

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

    virtual bool		isVertical() const			= 0;
    virtual bool		is2D() const				= 0;

    virtual const TrcKeyPath&	getPath() const				= 0;
				//!< Will be empty if isVertical() is false.
				//!< Example: Timeslices.

    bool			isAltDim0InInt(const char* keystr) const;
    void			getAuxInfo(int i0,int i1,IOPar&) const;

    const ZDomain::Info&	zDomain() const
				{ return source_.zDomain(); }
    float			nrKBytes() const;

protected:

				SeisFlatDataPack(const SeisDataPack&,int comp);

    virtual void		setSourceData()				= 0;

    const SeisDataPack&		source_;
    int				comp_;
};


/*!
\brief FlatDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularFlatDataPack : public SeisFlatDataPack
{
public:
				RegularFlatDataPack(
					const RegularSeisDataPack&,int comp);

    bool			isVertical() const
				{ return dir_ != TrcKeyZSampling::Z; }
    bool			is2D() const;

    const TrcKeyPath&		getPath() const		{ return path_; }

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }
    Coord3			getCoord(int i0,int i1) const;

    void			getAltDim0Keys(BufferStringSet&) const;

protected:

    void			setSourceData();

    TrcKeyPath			path_;
    const TrcKeyZSampling&	sampling_;
    TrcKeyZSampling::Dir	dir_;
};


/*!
\brief FlatDataPack for random lines.
*/

mExpClass(Seis) RandomFlatDataPack : public SeisFlatDataPack
{
public:
				RandomFlatDataPack(
					const RandomSeisDataPack&,int comp);

    bool			isVertical() const	{ return true; }
    bool			is2D() const		{ return false; }

    const TrcKeyPath&		getPath() const		{ return path_; }

    const StepInterval<float>&	getZRange() const	{ return zsamp_; }
    Coord3			getCoord(int i0,int i1) const;

protected:

    void			setSourceData();

    const TrcKeyPath&		path_;
    const StepInterval<float>&	zsamp_;
};

#endif
