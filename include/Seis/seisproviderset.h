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
#include "trckeyzsampling.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;

namespace PosInfo { class CubeData; }

namespace Seis
{

class Provider;

mExpClass(Seis) ProviderSet
{ mODTextTranslationClass(Seis::ProviderSet);
public:

    enum Policy			{ GetEveryWhere, RequireOnlyOne,
				  RequireAtLeastOne, RequireAll };

				~ProviderSet();

    virtual bool		is2D() const				= 0;

    bool			isEmpty() const { return size() == 0; }
    int				size() const	{ return provs_.size(); }

    void			addInput(Seis::GeomType);
    uiRetVal			addInput(const DBKey&);

    const Provider&		provider(int idx=0) const
				{ return *provs_[idx]; }
    Provider&			provider(int idx=0)
				{ return *provs_[idx]; }

    ZSampling			getZRange() const;
    virtual od_int64		totalNr() const		{ return -1; }

    uiRetVal			fillPar(ObjectSet<IOPar>&) const;
    uiRetVal			usePar(const ObjectSet<IOPar>&);

    uiRetVal			getNext(SeisTrc&);
    uiRetVal			get(const TrcKey&,ObjectSet<SeisTrc>&) const;

    void			deepErase();

    uiString			errMsg() const		{ return errmsg_; }

protected:

				ProviderSet(Policy=RequireAtLeastOne);

    virtual void		doFillPar(ObjectSet<IOPar>&,uiRetVal&) const;
    virtual void		doUsePar(const ObjectSet<IOPar>&,uiRetVal&)
				{}

    virtual void		doGetNext(SeisTrc&,uiRetVal&)		{}
    virtual void		doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const			{}

    mutable uiString		errmsg_;
    Policy			policy_;
    ObjectSet<Seis::Provider>	provs_;
};


mExpClass(Seis) ProviderSet3D : public ProviderSet
{ mODTextTranslationClass(Seis::ProviderSet3D);
public:
				ProviderSet3D(Policy pl=RequireAtLeastOne);
				~ProviderSet3D()	{};

    bool			is2D() const		{ return false; }
    od_int64			totalNr() const
				{ return iter_.totalNr(); }

    bool			getRanges(TrcKeyZSampling&) const;
    void			getGeometryInfo(PosInfo::CubeData&) const;

protected:

    void			doUsePar(const ObjectSet<IOPar>&,uiRetVal&);

    void			doGetNext(SeisTrc&,uiRetVal&);
    void			doGetNextTrcs(ObjectSet<SeisTrc>&,uiRetVal&);
    void			doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const;

    void			getStacked(SeisTrcBuf&,SeisTrc&);

    TrcKeySamplingIterator	iter_;
};


mExpClass(Seis) ProviderSet2D : public ProviderSet
{ mODTextTranslationClass(Seis::ProviderSet2D);
public:
				ProviderSet2D(Policy pl=RequireAtLeastOne)
				    : ProviderSet(pl)
				{};
				~ProviderSet2D()	{};

    bool			is2D() const		{ return true; }

protected:
};

}
