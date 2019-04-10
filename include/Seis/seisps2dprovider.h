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

class PS2DFetcher;

/*!\brief is the place to get traces from your 2D PS data stores.  */


mExpClass(Seis) PS2DProvider : public Provider2D
{ mODTextTranslationClass(Seis::PS2DProvider);
public:

			PS2DProvider();
			PS2DProvider(const DBKey&);
			~PS2DProvider();
    virtual GeomType	geomType() const	{ return LinePS; }

protected:

    friend class	PS2DFetcher;
    PS2DFetcher&	fetcher_;

    Fetcher2D&		fetcher() const override;
    void		prepWork(uiRetVal&) const override;
    size_type		gtNrOffsets() const override;
    bool		doGoTo(GeomID,trcnr_type,uiRetVal*) const override;
    void		gtCurGather(SeisTrcBuf&,uiRetVal&) const override;
    void		gtGatherAt(GeomID,trcnr_type,SeisTrcBuf&,
				    uiRetVal&) const override;

};


} // namespace Seis
