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


/*!\brief is the place to get traces from your 2D PS data stores.  */


mExpClass(Seis) PS2DProvider : public Provider2D
{
public:

			PS2DProvider();
			PS2DProvider(const DBKey&,uiRetVal&);

    bool		isPS() const override	{ return true; }

protected:

    friend class	PS2DFetcher;
    PS2DFetcher&	fetcher_;

    Fetcher&		gtFetcher() override;
    size_type		gtNrOffsets() const override;
    void		gtGather(SeisTrcBuf&,uiRetVal&) const override;

};


} // namespace Seis
