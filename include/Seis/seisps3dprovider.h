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


/*!\brief is the place to get traces from your 3D PS data stores.  */


mExpClass(Seis) PS3DProvider : public Provider3D
{ mODTextTranslationClass(Seis::PS3DProvider);
public:

			PS3DProvider();
			PS3DProvider(const DBKey&,uiRetVal&);

    bool		isPS() const override	{ return true; }

protected:

    friend class	PS3DFetcher;
    PS3DFetcher&	fetcher_;

    Fetcher&		gtFetcher() override;
    size_type		gtNrOffsets() const override;
    void		gtGather(SeisTrcBuf&,uiRetVal&) const override;

};


} // namespace Seis
