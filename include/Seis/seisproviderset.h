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
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;

namespace PosInfo { class CubeData; class Line2DDataIterator; }

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

    enum Policy			{ GetEveryWhere, RequireOnlyOne,
				  RequireAtLeastOne, RequireAll };
    enum ZPolicy		{ Minimum, Maximum };
				//!< Trcs with that z-range

				virtual ~MultiProvider();

    virtual bool		is2D() const			= 0;

    int				size() const	{ return provs_.size(); }
    bool			isEmpty() const { return size() == 0; }
    void			setEmpty();

    uiRetVal			addInput(const DBKey&);
    void			setSelData(SelData*); //!< Becomes mine.
    void			selectComponent(int iprov,int icomp);
    void			selectComponents(const TypeSet<int>&);
				//!< List of component indices to be
				//!< selected. Same for all providers.
    void			forceFPData(bool yn=true);
    void			setReadMode(ReadMode);

    ZSampling			getZRange() const;
    uiRetVal			getComponentInfo(int iprov,BufferStringSet&,
				      TypeSet<Seis::DataType>* dts=0) const;
    virtual od_int64		totalNr() const		{ return -1; }

    uiRetVal			fillPar(ObjectSet<IOPar>&) const;
    uiRetVal			usePar(const ObjectSet<IOPar>&);

    uiRetVal			getNext(SeisTrc&,bool dostack=false);
				/*< \param dostack, if false, first
				available trace in the list of providers is
				returned.*/
    uiRetVal			getGather(SeisTrcBuf&,bool dostack=false)
				{ /* TODO */ return uiRetVal(); }
				/*< \param dostack, if false, first
				available trace in the list of providers is
				returned.*/
    uiRetVal			get(const TrcKey&,ObjectSet<SeisTrc>&)const;
				/*< Fills the traces with data from each
				 provider at the specified TrcKey.*/
    uiRetVal			getGathers(const TrcKey&,
					   ObjectSet<SeisTrcBuf>&) const
				{ /* TODO */ return uiRetVal(); }
				/*< Fills the SeisTrcBuf with gather from
				  each provider at the specified TrcKey.*/

protected:

				MultiProvider(Policy,ZPolicy);

    void			addInput(Seis::GeomType);

    virtual void		doFillPar(ObjectSet<IOPar>&,uiRetVal&)const;
    virtual void		doUsePar(const ObjectSet<IOPar>&,uiRetVal&)
									= 0;
    virtual void		doGetNext(SeisTrc&,bool dostack,uiRetVal&)
									= 0;
    virtual void		doGetNextTrcs(ObjectSet<SeisTrc>&,uiRetVal&)
									= 0;
    virtual void		doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const			= 0;

    void			doGetStacked(SeisTrcBuf&,SeisTrc&);

    Policy			policy_;
    ZPolicy			zpolicy_;
    SelData*			seldata_;
    ObjectSet<Seis::Provider>	provs_;

public:

    const Provider&		provider( int idx ) const
				{ return *provs_[idx]; }
    Provider&			provider( int idx )
				{ return *provs_[idx]; }
};


/*
\brief MultiProvider for Seis::Vol data.
*/

mExpClass(Seis) MultiProvider3D : public MultiProvider
{ mODTextTranslationClass(Seis::MultiProvider3D);
public:
				MultiProvider3D(Policy,ZPolicy);
				~MultiProvider3D()	{};

    bool			is2D() const		{ return false; }
    od_int64			totalNr() const
				{ return iter_.totalNr(); }

    bool			getRanges(TrcKeyZSampling&,bool incl) const;
				//!< incl=union, !incl=intersection
    void			getGeometryInfo(PosInfo::CubeData&,
						bool incl) const;
				//!< incl=union, !incl=intersection

protected:

    void			doUsePar(const ObjectSet<IOPar>&,uiRetVal&);

    void			doGetNext(SeisTrc&,bool dostack,uiRetVal&);
    void			doGetNextTrcs(ObjectSet<SeisTrc>&,uiRetVal&);
    void			doGet(const TrcKey&,ObjectSet<SeisTrc>&,
				      uiRetVal&) const;

    TrcKeySamplingIterator	iter_;
};

}
