#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seisprovider.h"
class SeisPS2DReader;


namespace Seis
{

class PS2DFetcher;

/*!\brief is the place to get traces from your 2D PS data stores.  */


mExpClass(Seis) PS2DProvider : public Provider2D
{ mODTextTranslationClass(Seis::PS2DProvider);
public:

			PS2DProvider();
			PS2DProvider(const DBKey&);
			~PS2DProvider();
    virtual GeomType	geomType() const	{ return VolPS; }

    virtual BufferStringSet	getComponentInfo() const;
    virtual ZSampling		getZSampling() const;
    virtual TrcKeySampling	getHSampling() const;
    virtual void		getGeometryInfo(PosInfo::CubeData&) const;

protected:

    friend class	PS2DFetcher;
    PS2DFetcher&	fetcher_;

    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doReset(uiRetVal&) const;
    virtual void	doGetNextGather(SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetGather(const TrcKey&,SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;

    SeisPS2DReader*	mkReader() const;

};


} // namespace Seis
