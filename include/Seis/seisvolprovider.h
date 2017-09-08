#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seisprovider.h"


namespace Seis
{

class VolFetcher;

/*!\brief is the place to get traces from your seismic volumes.  */


mExpClass(Seis) VolProvider : public Provider3D
{ mODTextTranslationClass(Seis::VolProvider);
public:

			VolProvider();
			VolProvider(const DBKey&);
			~VolProvider();
    virtual GeomType	geomType() const	{ return Vol; }

    virtual bool	getRanges(TrcKeyZSampling&) const;
    virtual void	getGeometryInfo(PosInfo::CubeData&) const;

protected:

    friend class	VolFetcher;
    VolFetcher&		fetcher_;

    virtual void	doFillPar(IOPar&,uiRetVal&) const;
    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doReset(uiRetVal&) const;
    virtual TrcKey	doGetCurPosition() const;
    virtual bool	doGoTo(const TrcKey&);
    virtual uiRetVal	doGetComponentInfo(BufferStringSet&,DataType&) const;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGetNextData(TraceData&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;
    virtual void	doGetData(const TrcKey&,TraceData&,uiRetVal&) const;

};


} // namespace Seis
