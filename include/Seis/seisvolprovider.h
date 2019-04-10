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
			~VolProvider();

    virtual GeomType	geomType() const	{ return Vol; }

protected:

    friend class	VolFetcher;
    VolFetcher&		fetcher_;

    Fetcher3D&		fetcher() const override;
    void		getLocationData(uiRetVal&) const override;
    void		prepWork(uiRetVal&) const override;
    bool		doGoTo(const BinID&,uiRetVal*) const override;
    void		gtCur(SeisTrc&,uiRetVal&) const override;
    void		gtAt(const BinID&,TraceData&,SeisTrcInfo&,
				uiRetVal&) const override;
    void		gtComponentInfo(BufferStringSet&,DataType&) const override;

public:

    virtual const SeisTrcTranslator* curTransl() const override;

};


} // namespace Seis
