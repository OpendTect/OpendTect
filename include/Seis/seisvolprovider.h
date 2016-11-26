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

/*!\brief is the place to get traces from your seismic volumes.  */


mExpClass(Seis) VolumeProvider : public Provider
{ mODTextTranslationClass(Seis::VolumeProvider);
public:

			VolumeProvider();
			VolumeProvider(const DBKey&);
			~VolumeProvider();

    virtual GeomType	geomType() const	{ return Vol; }
    virtual uiRetVal	setInput(const DBKey&);

    virtual BufferStringSet getComponentInfo() const;
    virtual ZSampling	getZSampling() const;
    TrcKeySampling	getHSampling() const;

protected:

    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;

};


} // namespace Seis
