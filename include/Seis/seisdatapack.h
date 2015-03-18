#ifndef seisdatapack_h
#define seisdatapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"

#include "bindatadesc.h"
#include "datapackbase.h"
#include "trckeyzsampling.h"
#include "valseries.h"

template <class T> class Array3DImpl;

namespace ZDomain { class Info; }


mExpClass(Seis) SeisDataPack : public DataPack
{
public:
				~SeisDataPack();
    virtual int			nrTrcs() const				= 0;
    virtual TrcKey		getTrcKey(int globaltrcidx) const	= 0;
    virtual int			getGlobalIdx(const TrcKey&) const	= 0;

    virtual bool		addComponent(const char* nm)		= 0;

    virtual const StepInterval<float>&	getZRange() const		= 0;

    const OffsetValueSeries<float> getTrcStorage(
					int comp,int globaltrcidx) const;
    const float*		getTrcData(int comp,int globaltrcidx)const;

    int				nrComponents() const
				{ return arrays_.size(); }
    bool			isEmpty() const
				{ return arrays_.isEmpty(); }
    bool			validComp( int comp ) const
				{ return arrays_.validIdx( comp ); }
    const char*			getComponentName(int comp=0) const;

    const Array3DImpl<float>&	data(int component=0) const;
    Array3DImpl<float>&		data(int component=0);

    void			setZDomain(const ZDomain::Info&);
    const ZDomain::Info&	zDomain() const
				{ return *zdomaininfo_; }

    void			setScale(int comp,const SamplingData<float>&);
    const SamplingData<float>&	getScale(int comp) const;
    
    const BinDataDesc&		getDataDesc() const	{ return desc_; }

    float			nrKBytes() const;

protected:
				SeisDataPack(const char*,const BinDataDesc*);

    bool			addArray(int sz0,int sz1,int sz2);

    BufferStringSet			componentnames_;
    ObjectSet<Array3DImpl<float> >	arrays_;
    TypeSet<SamplingData<float> >	scales_;
    ZDomain::Info*			zdomaininfo_;
    BinDataDesc				desc_;
};


/*!
\brief SeisDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularSeisDataPack : public SeisDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						    const BinDataDesc* bdd=0);

    int				nrTrcs() const
				{ return (int)sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(int globaltrcidx) const;
    int				getGlobalIdx( const TrcKey& tk ) const
				{ return (int)sampling_.hsamp_.globalIdx(tk); }

    const StepInterval<float>&	getZRange() const
				{ return sampling_.zsamp_; }

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }
    bool			is2D() const;

    bool			addComponent(const char* nm);

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
\brief FlatDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularFlatDataPack : public FlatDataPack
{
public:
				RegularFlatDataPack(
					const RegularSeisDataPack&,int comp);
				~RegularFlatDataPack();

    int				nrTrcs() const
				{ return source_.nrTrcs(); }
    TrcKey			getTrcKey( int trcidx ) const
				{ return source_.getTrcKey(trcidx); }

    bool			isVertical() const
				{ return dir_ != TrcKeyZSampling::Z; }
    bool			is2D() const
				{ return source_.is2D(); }
    const RegularSeisDataPack&	getSourceDataPack() const
				{ return source_; }

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }
    Coord3			getCoord(int i0,int i1) const;

    const ZDomain::Info&	zDomain() const
				{ return source_.zDomain(); }

    float			nrKBytes() const;

private:

    void			setSourceData();

    const RegularSeisDataPack&	source_;
    const TrcKeyZSampling&	sampling_;
    TrcKeyZSampling::Dir	dir_;
    int				comp_;
};


/*!
\brief FlatDataPack for random lines.
*/

mExpClass(Seis) RandomFlatDataPack : public FlatDataPack
{
public:
				RandomFlatDataPack(
					const RandomSeisDataPack&,int comp);
				~RandomFlatDataPack();

    int				nrTrcs() const		{ return path_.size(); }
    const TrcKey&		getTrcKey( int trcidx ) const
				{ return path_[trcidx]; }

    const RandomSeisDataPack&	getSourceDataPack() const
				{ return source_; }

    const StepInterval<float>&	getZRange() const	{ return zsamp_; }
    const TrcKeyPath&		getPath() const		{ return path_; }
    Coord3			getCoord(int i0,int i1) const;

    const ZDomain::Info&	zDomain() const
				{ return source_.zDomain(); }

    float			nrKBytes() const;

private:

    void			setSourceData();

    const RandomSeisDataPack&	source_;
    const TrcKeyPath&		path_;
    const StepInterval<float>&	zsamp_;
    int				comp_;
};

#endif
