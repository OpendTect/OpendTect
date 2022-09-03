/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

namespace Well
{

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


void WellDataFilter::getWellsFromMnems( const MnemonicSelection& mns,
				       BufferStringSet& wellnms ) const
{
    for ( const auto* wd : allwds_ )
    {
	const Well::LogSet& lis = wd->logs();
	MnemonicSelection wdmns; lis.getAllAvailMnems( wdmns );
	bool addwell = true;
	for ( const auto* mn : mns )
	{
	    const bool wdhasmn = wdmns.isPresent( mn );
	    if ( wdhasmn )
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


void WellDataFilter::getMarkersLogsMnemsFromWells(
					const BufferStringSet& wellnms,
					BufferStringSet& lognms,
					MnemonicSelection& mns,
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
	    wd->logs().getAllAvailMnems( mns );
	    wd->markers().getNames( markernms );
	    first = false;
	}
	else
	{
	    BufferStringSet wdlognms, wdmarkernms, lognms2rm, markernms2rm;
	    MnemonicSelection wdmns, wdmns2rm;
	    wd->logs().getNames( wdlognms );
	    wd->logs().getAllAvailMnems( wdmns );
	    wd->markers().getNames( wdmarkernms );
	    for ( int lidx=0; lidx<lognms.size(); lidx++ )
	    {
		const BufferString& lognm = lognms.get( lidx );
		if ( wdlognms.isPresent(lognm) )
		    continue;

		lognms.removeSingle( lidx );
		lidx--;
	    }

	    for ( int mnidx=mns.size()-1; mnidx>=0; mnidx-- )
	    {
		const Mnemonic* mn = mns.get( mnidx );
		if ( wdmns.isPresent(mn) )
		    continue;

		mns.removeSingle( mnidx );
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
				     Array2D<int>& presence ) const
{
    presence.setAll( -1 );

    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd, topnm,
							       botnm, true );
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
	    const Well::Log* log = wd->logs().getLog( lognm.buf() );
	    if ( !log )
		continue;

	    const Interval<float> logrg = log->dahRange();
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


void WellDataFilter::getLogPresenceForMnems( const BufferStringSet& wellnms,
				     const char* topnm, const char* botnm,
				     const MnemonicSelection& mns,
				     Array2D<int>& presence ) const
{
    presence.setAll( -1 );

    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd, topnm,
							       botnm, true );
	if ( markerrg.isUdf() )
	{
	    for ( int mnidx=0; mnidx<mns.size(); mnidx++ )
		presence.set( widx, mnidx, 100 );

	    continue;
	}

	const float markerwidth = markerrg.width();
	for ( const auto* mn : mns )
	{
	    const Well::Log* log = wd->logs().getLog( *mn );
	    if ( !log )
		continue;

	    const Interval<float> logrg = log->dahRange();
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

	    presence.set( widx, mns.indexOf(mn), perc );
	}
    }
}


void WellDataFilter::getLogPresenceFromValFilter(
					const BufferStringSet& wellnms,
					const BufferStringSet& lognms,
					const BufferStringSet& alllognms,
					const MnemonicSelection& mns,
					const TypeSet<Interval<float>> valrg,
					Array2D<int>& presence ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const int perc = 0;
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	{

	    for ( int lidx=0; lidx<alllognms.size(); lidx++ )
		presence.set( widx, lidx, perc );
	}

	for ( const auto* mn : mns )
	{
	    const int idx = mns.indexOf( mn );
	    const TypeSet<int> logidxs = wd->logs().getSuitable( *mn );
	    for ( const auto& logidx : logidxs )
	    {
		const Well::Log* log = &wd->logs().getLog( logidx);
		const int lidx = alllognms.indexOf( log->name() );
		if ( !lognms.isPresent(log->name()) )
		{
		    presence.set( widx, lidx, perc );
		    continue;
		}

		const bool perczero = !log ||
				      !lognms.isPresent( log->name() ) ||
				      !valrg[idx].includes( log->valueRange() );
		if ( perczero )
		    presence.set( widx, lidx, perc );
	    }
	}
    }
}


void WellDataFilter::getLogsInMarkerZone( BufferStringSet& wellnms,
				      const char* topnm, const char* botnm,
				      BufferStringSet& lognms ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd, topnm,
							       botnm, false );
	if ( markerrg.isUdf() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	BufferStringSet currwelllognms;
	for ( int idx=0; idx<wd->logs().size(); idx++ )
	{
	    const Well::Log* log = &wd->logs().getLog( idx );
	    if ( !log )
		continue;

	    const Interval<float> logrg = log->dahRange();
	    if ( !logrg.overlaps(markerrg,false) )
		continue;

	    currwelllognms.add( log->name() );
	}

	if ( currwelllognms.isEmpty() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	lognms.add( currwelllognms, false );
    }
}


void WellDataFilter::getMnemsInMarkerZone( BufferStringSet& wellnms,
					 const char* topnm, const char* botnm,
					 MnemonicSelection& mns ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd, topnm,
							       botnm, false );
	if ( markerrg.isUdf() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	MnemonicSelection currwellmns;
	for ( int idx=0; idx<wd->logs().size(); idx++ )
	{
	    const Well::Log* log = &wd->logs().getLog( idx );
	    if ( !log )
		continue;

	    const Interval<float> logrg = log->dahRange();
	    if ( !logrg.overlaps(markerrg,false) )
		continue;

	    currwellmns.addIfNew( log->mnemonic() );
	}

	if ( currwellmns.isEmpty() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	for ( const auto* mn : currwellmns )
	    mns.addIfNew( mn );
    }
}



void WellDataFilter::getMnemsInDepthInterval( const Interval<float> depthrg,
				     BufferStringSet& wellnms,
				     MnemonicSelection& mns ) const
{
    for ( const auto* wd : allwds_ )
    {
	MnemonicSelection currwellmns;
	for ( int idx=0; idx<wd->logs().size(); idx++ )
	{
	    const Well::Log* log = &wd->logs().getLog( idx );
	    if ( !log )
		continue;

	    Interval<float> logrg = log->dahRange();
	    logrg.start = wd->track().getPos(logrg.start).z;
	    logrg.stop = wd->track().getPos(logrg.stop).z;
	    if ( !logrg.overlaps(depthrg,false) )
		continue;

	    currwellmns.addIfNew( log->mnemonic() );
	}

	if ( currwellmns.isEmpty() )
	    continue;

	wellnms.add( wd->name() );
	for ( const auto* mn : currwellmns )
	    mns.addIfNew( mn );
    }
}


void WellDataFilter::getLogsInDepthInterval( const Interval<float> depthrg,
				     BufferStringSet& wellnms,
				     BufferStringSet& lognms ) const
{
    for ( const auto* wd : allwds_ )
    {
	BufferStringSet currwelllognms;
	for ( int idx=0; idx<wd->logs().size(); idx++ )
	{
	    const Well::Log* log = &wd->logs().getLog( idx );
	    if ( !log )
		continue;

	    Interval<float> logrg = log->dahRange();
	    logrg.start = wd->track().getPos(logrg.start).z;
	    logrg.stop = wd->track().getPos(logrg.stop).z;
	    if ( !logrg.overlaps(depthrg,false) )
		continue;

	    currwelllognms.add( log->name() );
	}

	if ( currwelllognms.isEmpty() )
	    continue;

	wellnms.add( wd->name() );
	lognms.add( currwelllognms, false ); 
    }
}


void WellDataFilter::getLogsInValRange( const MnemonicSelection& mns,
					const TypeSet<Interval<float>> valrg,
					BufferStringSet& wellnms,
					BufferStringSet& lognms ) const
{
    for ( const auto* wd : allwds_ )
    {
	BufferStringSet lognames;
	for ( const auto* mn : mns )
	{
	    const int idx = mns.indexOf( mn );
	    const TypeSet<int> logidxs = wd->logs().getSuitable( *mn );
	    for ( const auto& logidx : logidxs )
	    {
		const Well::Log* log = &wd->logs().getLog( logidx);
		if ( !log )
		    continue;

		if ( !valrg[idx].includes(log->valueRange()) )
		    continue;

		lognames.add( log->name() );
	    }
	}

	if ( lognames.isEmpty() )
	    continue;

	wellnms.add( wd->name() );
	lognms.add( lognames, false );
    }
}


Interval<float> WellDataFilter::getDepthRangeFromMarkers(
					const Well::Data* wd,
					const char* topnm, const char* botnm,
					bool vertical ) const
{
    Interval<float> markerrg = Interval<float>::udf();
    if ( StringView(topnm) == Well::ZRangeSelector::sKeyDataStart() )
	markerrg.start = vertical ? wd->track().zRange().start
				  : wd->track().dahRange().start;
    else
    {
	const Well::Marker* marker = wd->markers().getByName( topnm );
	if ( marker )
	{
	    float mrkrdahstart = marker->dah();
	    markerrg.start = vertical ? wd->track().getPos(mrkrdahstart).z
				      : mrkrdahstart;
	}
    }

    if ( StringView(botnm) == Well::ZRangeSelector::sKeyDataEnd() )
	markerrg.stop = vertical ? wd->track().zRange().stop
				 : wd->track().dahRange().stop;
    else
    {
	const Well::Marker* marker = wd->markers().getByName( botnm );
	if ( marker )
	{
	    float mrkrdahstop = marker->dah();
	    markerrg.stop = vertical ? wd->track().getPos(mrkrdahstop).z
				     : mrkrdahstop;
	}
    }

    return markerrg;
}


void WellDataFilter::getLogsForMnems( const MnemonicSelection& mns,
				      BufferStringSet& lognms ) const
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


void WellDataFilter::getWellsOfType( const OD::WellType wt,
				     BufferStringSet& wellnms ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( wd->info().welltype_ == wt )
	    wellnms.add( wd->name() );
    }
}

} // namespace Well
