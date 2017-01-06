#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

*/

#include "seisprovider.h"
class SeisPS3DReader;


namespace Seis
{

class PS3DFetcher;

/*!\brief is the place to get traces from your 3D PS data stores.  */


mExpClass(Seis) PS3DProvider : public Provider3D
{ mODTextTranslationClass(Seis::PS3DProvider);
public:

			PS3DProvider();
			PS3DProvider(const DBKey&);
			~PS3DProvider();
    virtual GeomType	geomType() const	{ return VolPS; }

    virtual bool		getRanges(TrcKeyZSampling&) const;
    virtual void		getGeometryInfo(PosInfo::CubeData&) const;

protected:

    friend class	PS3DFetcher;
    PS3DFetcher&	fetcher_;

    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doReset(uiRetVal&) const;
    virtual uiRetVal	doGetComponentInfo(BufferStringSet&,
					    TypeSet<Seis::DataType>&) const;
    virtual void	doGetNextGather(SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetGather(const TrcKey&,SeisTrcBuf&,uiRetVal&) const;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;

    SeisPS3DReader*	mkReader() const;

};


} // namespace Seis
