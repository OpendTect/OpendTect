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

class LineFetcher;

/*!\brief is the place to get traces from your seismic lines.  */


mExpClass(Seis) LineProvider : public Provider2D
{ mODTextTranslationClass(Seis::LineProvider);
public:

			LineProvider();
			LineProvider(const DBKey&);
			~LineProvider();

    virtual GeomType	geomType() const	{ return Line; }

protected:

    friend class	LineFetcher;
    LineFetcher&	fetcher_;

    Fetcher2D&	fetcher() const override;
    void	prepWork(uiRetVal&) const override;
    bool	doGoTo(GeomID,trcnr_type,uiRetVal*) const override;
    void	gtCur(SeisTrc&,uiRetVal&) const override;
    void	gtAt(GeomID,trcnr_type,TraceData&,SeisTrcInfo&,
			uiRetVal&) const override;
    void	gtComponentInfo(BufferStringSet&,DataType&) const override;

public:

    virtual const SeisTrcTranslator* curTransl() const override;

};


} // namespace Seis
