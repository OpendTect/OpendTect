/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Feb 2022
________________________________________________________________________

-*/

#include "welldatafilter.h"


#include "arrayndimpl.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"

using namespace Well;

// WellDataFilter
WellDataFilter::WellDataFilter( const ObjectSet<Well::Data>& wds )
    : allwds_(wds)
{}


WellDataFilter::~WellDataFilter()
{}


void WellDataFilter::getWellsFromLogs( const BufferStringSet& lognms,
				       BufferStringSet& wellnms ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const Well::LogSet& lis = wd->logs();
	BufferStringSet wdlognms; lis.getNames( wdlognms );
	bool addwell = true;
	for ( int lidx=0; lidx<lognms.size(); lidx++ )
	{
	    const bool wdhaslog = wdlognms.isPresent( lognms.get(lidx) );
	    if ( wdhaslog )
		continue;

	    addwell = false;
	    break;
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsWithNoLogs( BufferStringSet& wellnms ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( wd->logs().isEmpty() )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsFromMarkers( const BufferStringSet& markernms,
					  BufferStringSet& wellnms ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const Well::MarkerSet& ms = wd->markers();
	BufferStringSet wdmarkernms; ms.getNames( wdmarkernms );
	bool addwell = true;
	for ( int midx=0; midx<markernms.size(); midx++ )
	{
	    const BufferString& markernm = markernms.get( midx );
	    const bool wdhasmrkr =
			markernm==Well::ZRangeSelector::sKeyDataStart() ||
			markernm==Well::ZRangeSelector::sKeyDataEnd() ||
			wdmarkernms.isPresent( markernm );
	    if ( wdhasmrkr )
		continue;

	    addwell = false;
	    break;
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getMarkersLogsFromWells( const BufferStringSet& wellnms,
					      BufferStringSet& lognms,
					      BufferStringSet& markernms ) const
{
    bool first = true;
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	if ( first )
	{
	    wd->logs().getNames( lognms );
	    wd->markers().getNames( markernms );
	    first = false;
	}
	else
	{
	    BufferStringSet wdlognms, wdmarkernms, lognms2rm, markernms2rm;
	    wd->logs().getNames( wdlognms );
	    wd->markers().getNames( wdmarkernms );
	    for ( int lidx=0; lidx<lognms.size(); lidx++ )
	    {
		const BufferString& lognm = lognms.get( lidx );
		if ( wdlognms.isPresent(lognm) )
		    continue;

		lognms.removeSingle( lidx );
		lidx--;
	    }

	    for ( int midx=0; midx<markernms.size(); midx++ )
	    {
		const BufferString& markernm = markernms.get( midx );
		if ( wdmarkernms.isPresent(markernm) )
		    continue;

		markernms.removeSingle( midx );
		midx--;
	    }
	}
    }
}


void WellDataFilter::getLogPresence( const BufferStringSet& wellnms,
				     const char* topnm, const char* botnm,
				     const BufferStringSet& alllognms,
				     Array2D<int>& presence,
				     BufferStringSet& lognms,
				     Well::Info::DepthType depthtype ) const
{
    presence.setAll( -1 );

    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	Interval<float> markerrg = Interval<float>::udf();
	float kbelv = wd->track().getKbElev();
	if ( FixedString(topnm) == Well::ZRangeSelector::sKeyDataStart() )
	    markerrg.start = wd->track().dahRange().start;
	else
	{
	    const Well::Marker* marker = wd->markers().getByName( topnm );
	    if ( marker )
	    {
		float mrkrdahstart = marker->dah();
		if ( depthtype == Well::Info::MD)
		    markerrg.start = mrkrdahstart;
		else
		{
		    markerrg.start = wd->track().getPos(mrkrdahstart).z;
		    if ( depthtype == Well::Info::TVD )
			markerrg.start += kbelv;
		}
	    }
	}

	if ( FixedString(botnm) == Well::ZRangeSelector::sKeyDataEnd() )
	    markerrg.stop = wd->track().dahRange().stop;
	else
	{
	    const Well::Marker* marker = wd->markers().getByName( botnm );
	    if ( marker )
	    {
		float mrkrdahstop = marker->dah();
		if ( depthtype == Well::Info::MD )
		    markerrg.stop = mrkrdahstop;
		else
		{
		    markerrg.stop = wd->track().getPos(mrkrdahstop).z;
		    if ( depthtype == Well::Info::TVD )
			markerrg.stop += kbelv;
		}
	    }
	}

	if ( markerrg.isUdf() )
	{
	    for ( int lidx=0; lidx<alllognms.size(); lidx++ )
		presence.set( widx, lidx, 100 );
	    continue;
	}

	const float markerwidth = markerrg.width();
	for ( int lidx=0; lidx<alllognms.size(); lidx++ )
	{
	    const BufferString& lognm = alllognms.get( lidx );
	    const Well::Log* log = wd->logs().getLog( lognm );
	    if ( !log )
		continue;

	    const Interval<float>& logrg = log->dahRange();
	    int perc = 0;
	    if ( logrg.includes(markerrg,false) )
		perc = 100;
	    else if ( !logrg.overlaps(markerrg,false) )
		perc = 0;
	    else
	    {
		Interval<float> avlogrg = logrg;
		avlogrg.limitTo( markerrg );
		perc = mNINT32( 100.f * avlogrg.width()/markerwidth );
	    }

	    presence.set( widx, lidx, perc );
	}
    }
}


void WellDataFilter::getLogsForMnems(const MnemonicSelection& mns,
				     BufferStringSet& lognms) const
{
    for ( const auto* wd : allwds_ )
    {
	for ( const auto* mn : mns )
	{
	    TypeSet<int> idxs = wd->logs().getSuitable( *mn );
	    for ( const auto& idx : idxs )
		lognms.addIfNew( wd->logs().getLog(idx).name() );
	}
    }
}
