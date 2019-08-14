#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016 / Aug 2019
________________________________________________________________________

*/

#include "seisprovider.h"


namespace Seis
{

class LineFetcher;
class VolFetcher;
class PS2DFetcher;
class PS3DFetcher;


/*!\brief is the place to get traces from your seismic lines.  */

mExpClass(Seis) LineProvider : public Provider2D
{
public:

			LineProvider();
			LineProvider(const DBKey&,uiRetVal&);
			~LineProvider();

protected:

    friend class	LineFetcher;
    LineFetcher&	fetcher_;

    Fetcher&	gtFetcher() override;
    void	gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const override;

};


/*!\brief is the place to get traces from your seismic volumes.  */

mExpClass(Seis) VolProvider : public Provider3D
{
public:

			VolProvider();
			VolProvider(const DBKey&,uiRetVal&);
			~VolProvider();

protected:

    friend class	VolFetcher;
    VolFetcher&		fetcher_;

    Fetcher&		gtFetcher() override;
    void		gtTrc(TraceData&,SeisTrcInfo&,uiRetVal&) const override;

};


/*!\brief is the place to get traces from your 2D PS data stores.  */

mExpClass(Seis) PS2DProvider : public Provider2D
{
public:

			PS2DProvider();
			PS2DProvider(const DBKey&,uiRetVal&);
			~PS2DProvider();

    bool		isPS() const override	{ return true; }

protected:

    friend class	PS2DFetcher;
    PS2DFetcher&	fetcher_;

    Fetcher&		gtFetcher() override;
    size_type		gtNrOffsets() const override;
    void		gtGather(SeisTrcBuf&,uiRetVal&) const override;

};


/*!\brief is the place to get traces from your 3D PS data stores.  */


mExpClass(Seis) PS3DProvider : public Provider3D
{ mODTextTranslationClass(Seis::PS3DProvider);
public:

			PS3DProvider();
			PS3DProvider(const DBKey&,uiRetVal&);
			~PS3DProvider();

    bool		isPS() const override	{ return true; }

protected:

    friend class	PS3DFetcher;
    PS3DFetcher&	fetcher_;

    Fetcher&		gtFetcher() override;
    size_type		gtNrOffsets() const override;
    void		gtGather(SeisTrcBuf&,uiRetVal&) const override;

};


} // namespace Seis
