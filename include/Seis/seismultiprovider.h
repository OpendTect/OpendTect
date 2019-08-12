#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2017
________________________________________________________________________

*/

#include "seismod.h"
#include "seistype.h"
#include "survgeom2d.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "enums.h"

class SeisTrc;
class SeisTrcBuf;

namespace PosInfo { class CubeData; class LineData; }

namespace Seis
{

class Provider;
class SelData;

/*
\brief Base class for accessing multiple providers and obtaining results
according to a specified synchronisation policy.

You need to instantiate one of MultiProvider2D or MultiProvider3D. If all
you have is a list of IOPar, find out from IOPar by getting the DBKey from
the first IOPar and asking SeisIOObjInfo whether it's tied to 2D or 3D
seismics.
*/

mExpClass(Seis) MultiProvider
{ mODTextTranslationClass(Seis::MultiProvider);
public:

    mUseType( Pos,		GeomID );

    enum Policy			{ GetEveryWhere, RequireOnlyOne,
				  RequireAtLeastOne, RequireAll };
				mDeclareEnumUtils(Policy);
    enum ZPolicy		{ Minimum, Maximum };
				mDeclareEnumUtils(ZPolicy)

    virtual			~MultiProvider();
    virtual bool		is2D() const			= 0;

    int				size() const	{ return provs_.size(); }
    bool			isEmpty() const { return size() == 0; }
    void			setEmpty();

    uiRetVal			addInput(const DBKey&);
    void			setSelData(SelData*); //!< Becomes mine.
    void			selectComponent(int iprov,int icomp);
    void			selectComponents(const TypeSet<int>&);
				    //!< same comps for all providers
    void			forceFPData(bool yn=true);
    void			setReadMode(ReadMode);

    ZSampling			getZRange() const;
    uiRetVal			getComponentInfo(int iprov,BufferStringSet&,
						  DataType* dt=0) const;
    virtual od_int64		totalNr() const;

    void			fillPar(IOPar&) const;
    uiRetVal			usePar(const IOPar&);

    uiRetVal			getNext(SeisTrc&,bool dostack_else_first=false);
    uiRetVal			getNext(ObjectSet<SeisTrc>&);
    uiRetVal			getGather(SeisTrcBuf&,
					  bool dostack_else_first=false)
				    { return mTODONotImplPhrase(); }
    uiRetVal			getAt(const TrcKey&,ObjectSet<SeisTrc>&)const;
    uiRetVal			getGathersAt(const TrcKey&,
					   ObjectSet<SeisTrcBuf>&) const
				    { return mTODONotImplPhrase(); }
    uiRetVal			reset() const;
				    //!< done automatically when needed
    const SelData*		selData() const		{ return seldata_; }

protected:

				MultiProvider(Policy,ZPolicy,
					      float def_sample_value=0.0f);

    void			addInput(Seis::GeomType);
    bool			handleSetupChanges(uiRetVal&) const;
    void			handleTraces(ObjectSet<SeisTrc>&) const;
    void			ensureRightZSampling(
					ObjectSet<SeisTrc>&) const;

    virtual void		doReset(uiRetVal&) const		= 0;
    virtual void		doFillPar(IOPar&) const;
    virtual void		doUsePar(const IOPar&,uiRetVal&);
    virtual void		doGetNext(SeisTrc&,bool dostack,
					  uiRetVal&) const		= 0;
    virtual void		doGetNextTrcs(ObjectSet<SeisTrc>&,
					      uiRetVal&) const;
    virtual void		doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const			= 0;
    virtual bool		doMoveToNext() const			= 0;

    void			doGetStacked(SeisTrcBuf&,SeisTrc&) const;

    mutable Threads::Lock	lock_;
    mutable od_int64		totalnr_;
    mutable bool		setupchgd_;
    mutable ZSampling		zsampling_;
    float			specialvalue_;
    Policy			policy_;
    ZPolicy			zpolicy_;
    SelData*			seldata_;
    ObjectSet<Seis::Provider>	provs_;

    mutable TrcKeySamplingIterator	iter_;

public:

    const Provider&		provider( int idx ) const
				{ return *provs_[idx]; }
    Provider&			provider( int idx )
				{ return *provs_[idx]; }
};


/*
\brief MultiProvider for 3D seismic data.
*/

mExpClass(Seis) MultiProvider3D : public MultiProvider
{ mODTextTranslationClass(Seis::MultiProvider3D);
public:

				MultiProvider3D(Policy,ZPolicy);
				~MultiProvider3D()	{}

    bool			is2D() const		{ return false; }

    bool			getRanges(TrcKeyZSampling&) const;
    void			getGeometryInfo(PosInfo::CubeData&) const;

protected:

    void			doReset(uiRetVal&) const;
    void			doGetNext(SeisTrc&,bool dostack,
					  uiRetVal&) const;
    void			doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const;
    bool			doMoveToNext() const;

};


/*
\brief MultiProvider for 2D seismic data.
*/

mExpClass(Seis) MultiProvider2D : public MultiProvider
{ mODTextTranslationClass(Seis::MultiProvider2D);
public:

				MultiProvider2D(Policy,ZPolicy);
				~MultiProvider2D()	{}

    bool			is2D() const		{ return true; }

    int				nrLines() const
				{ return geomids_.size(); }
    GeomID			geomID( int iln ) const
				{ return geomids_[iln]; }
    BufferString		lineName( int iln ) const
				{ return geomID(iln).name(); }
    int				curLineIdx() const	{ return curlidx_; }

    bool			getRanges(int iln,StepInterval<int>& trcrg,
					  ZSampling&) const;
    void			getGeometryInfo(int iln,
					  PosInfo::LineData&) const;

protected:

    void			doReset(uiRetVal&) const;
    void			doGetNext(SeisTrc&,bool dostack,
					  uiRetVal&) const;
    void			doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const;
    bool			doMoveToNext() const;
    bool			doMoveToNextLine() const;

    void			doFillPar(IOPar&) const;
    void			doUsePar(const IOPar&,uiRetVal&);

    mutable int			curlidx_;
    mutable GeomIDSet		geomids_;

};

}
