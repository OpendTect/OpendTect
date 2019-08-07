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

/*!\brief is the place to get traces from your seismic lines.  */

mExpClass(Seis) LineProvider : public Provider2D
{
public:

			LineProvider();

    virtual GeomType	geomType() const	{ return Line; }

protected:

    friend class	LineFetcher;
    LineFetcher&	fetcher_;

    Fetcher&	gtFetcher() override;
    void	gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const override;

};


} // namespace Seis
