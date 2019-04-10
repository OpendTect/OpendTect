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

class PS3DFetcher;

/*!\brief is the place to get traces from your 3D PS data stores.  */


mExpClass(Seis) PS3DProvider : public Provider3D
{ mODTextTranslationClass(Seis::PS3DProvider);
public:

			PS3DProvider();
			PS3DProvider(const DBKey&);
			~PS3DProvider();
    virtual GeomType	geomType() const	{ return VolPS; }

protected:

    friend class	PS3DFetcher;
    PS3DFetcher&	fetcher_;

    Fetcher3D&		fetcher() const override;
    void		getLocationData(uiRetVal&) const override;
    void		prepWork(uiRetVal&) const override;
    size_type		gtNrOffsets() const override;
    bool		doGoTo(const BinID&,uiRetVal*) const override;
    void		gtCurGather(SeisTrcBuf&,uiRetVal&) const override;
    void		gtGatherAt(const BinID&,SeisTrcBuf&,
				    uiRetVal&) const override;

};


} // namespace Seis
