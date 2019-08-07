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

mExpClass(Seis) VolProvider : public Provider3D
{
public:

			VolProvider();

    virtual GeomType	geomType() const	{ return Vol; }

protected:

    friend class	VolFetcher;
    VolFetcher&		fetcher_;

    Fetcher&		gtFetcher() override;
    void		gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const override;

};


} // namespace Seis
