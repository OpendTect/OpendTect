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

    virtual int		nrLines() const;
    virtual int		curLineIdx() const;
    virtual BufferString lineName(int) const;
    virtual Pos::GeomID	geomID(int) const;
    virtual int		lineNr(Pos::GeomID) const;
    virtual void	getGeometryInfo(int,PosInfo::Line2DData&) const;
    virtual bool	getRanges(int,StepInterval<int>&,
					  ZSampling&) const;

protected:

    friend class	LineFetcher;
    LineFetcher&	fetcher_;

    virtual void	doUsePar(const IOPar&,uiRetVal&);
    virtual void	doReset(uiRetVal&) const;
    virtual uiRetVal	doGetComponentInfo(BufferStringSet&,
					    TypeSet<Seis::DataType>&) const;
    virtual void	doGetNext(SeisTrc&,uiRetVal&) const;
    virtual void	doGet(const TrcKey&,SeisTrc&,uiRetVal&) const;

};


} // namespace Seis
