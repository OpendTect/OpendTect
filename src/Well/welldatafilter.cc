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
				       BufferStringSet& wellnms,
				       bool shouldhavewholeset ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	if ( !wd )
	    continue;

	const Well::LogSet& logs = wd->logs();
	BufferStringSet wdlognms;
	logs.getNames( wdlognms );
	bool addwell = false;
	for ( const auto* lognm : lognms )
	{
	    const bool wdhaslog = wdlognms.isPresent( lognm->buf() );
	    if ( !wdhaslog && shouldhavewholeset )
	    {
		addwell = false;
		break;
	    }
	    else if ( wdhaslog && !shouldhavewholeset )
	    {
		addwell = true;
		break;
	    }
	    else if ( wdhaslog && shouldhavewholeset )
	    {
		addwell = true;
		continue;
	    }
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsFromMnems( const MnemonicSelection& mns,
				       BufferStringSet& wellnms,
				       bool shouldhavewholeset ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( !wd )
	    continue;

	const Well::LogSet& logs = wd->logs();
	MnemonicSelection wdmns;
	logs.getAllAvailMnems( wdmns );
	bool addwell = false;
	for ( const auto* mn : mns )
	{
	    const bool wdhasmn = wdmns.isPresent( mn );
	    if (!wdhasmn && shouldhavewholeset)
	    {
		addwell = false;
		break;
	    }
	    else if (wdhasmn && !shouldhavewholeset)
	    {
		addwell = true;
		break;
	    }
	    else if (wdhasmn && shouldhavewholeset)
	    {
		addwell = true;
		continue;
	    }
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsWithNoLogs( BufferStringSet& wellnms ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( wd && wd->logs().isEmpty() )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsFromMarkers( const BufferStringSet& markernms,
					  BufferStringSet& wellnms,
					  bool shouldhavewholeset ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const Well::MarkerSet& ms = wd->markers();
	BufferStringSet wdmarkernms; ms.getNames( wdmarkernms );
	bool addwell = false;
	for ( int midx=0; midx<markernms.size(); midx++ )
	{
	    const BufferString& markernm = markernms.get( midx );
	    const bool wdhasmrkr =
			markernm==Well::ZRangeSelector::sKeyDataStart() ||
			markernm==Well::ZRangeSelector::sKeyDataEnd() ||
			wdmarkernms.isPresent( markernm );
	    if (!wdhasmrkr && shouldhavewholeset)
	    {
		addwell = false;
		break;
	    }
	    else if (wdhasmrkr && !shouldhavewholeset)
	    {
		addwell = true;
		break;
	    }
	    else if (wdhasmrkr && shouldhavewholeset)
	    {
		addwell = true;
		continue;
	    }
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getMarkersLogsMnemsFromWells(
					const BufferStringSet& wellnms,
					BufferStringSet& lognms,
					MnemonicSelection& mns,
					BufferStringSet& markernms,
					bool getonlycommon ) const
{
    bool first = true;
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wd && wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Well::LogSet& logs = wd->logs();
	if ( first )
	{
	    logs.getNames( lognms );
	    logs.getAllAvailMnems( mns );
	    wd->markers().getNames( markernms );
	    first = false;
	}
	else
	{
	    BufferStringSet wdlognms, wdmarkernms;
	    MnemonicSelection wdmns;
	    logs.getNames( wdlognms );
	    logs.getAllAvailMnems( wdmns );
	    wd->markers().getNames( wdmarkernms );
	    if ( !getonlycommon )
	    {
		lognms.add( wdlognms, false );
		for ( const auto* mn : wdmns )
		    mns.addIfNew( mn );

		markernms.add( wdmarkernms, false );
		continue;
	    }

	    for ( int lidx=lognms.size()-1; lidx>=0; lidx-- )
	    {
		const BufferString& lognm = lognms.get( lidx );
		if ( wdlognms.isPresent(lognm) )
		    continue;

		lognms.removeSingle( lidx );
	    }

	    for ( int mnidx=mns.size()-1; mnidx>=0; mnidx-- )
	    {
		const Mnemonic* mn = mns.get( mnidx );
		if ( wdmns.isPresent(mn) )
		    continue;

		mns.removeSingle( mnidx );
	    }

	    for ( int midx=markernms.size()-1; midx>=0; midx-- )
	    {
		const BufferString& markernm = markernms.get( midx );
		if ( wdmarkernms.isPresent(markernm) )
		    continue;

		markernms.removeSingle( midx );
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
	ConstRefMan<Well::Data> wd = allwds_[widx];
	const bool haswellnm = wd && wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd.ptr(),
							   topnm, botnm, false);
	if ( markerrg.isUdf() )
	{
	    for ( int lidx=0; lidx<alllognms.size(); lidx++ )
		presence.set( widx, lidx, 100 );
	    continue;
	}

	const float markerwidth = markerrg.width();
	const Well::LogSet& logs = wd->logs();
	for ( int lidx=0; lidx<alllognms.size(); lidx++ )
	{
	    const BufferString& lognm = alllognms.get( lidx );
	    if ( !logs.isPresent(lognm.buf()) )
		continue;

	    const Interval<float> logrg = logs.getDahRangeForLog( lognm.buf() );
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
	ConstRefMan<Well::Data> wd = allwds_[widx];
	const bool haswellnm = wd && wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd.ptr(),
							   topnm, botnm, false);
	if ( markerrg.isUdf() )
	{
	    for ( int mnidx=0; mnidx<mns.size(); mnidx++ )
		presence.set( widx, mnidx, 100 );

	    continue;
	}

	const float markerwidth = markerrg.width();
	const Well::LogSet& logs = wd->logs();
	for ( const auto* mn : mns )
	{
	    const BufferString lognm = logs.getLogNameFor( *mn );
	    if ( lognm.isEmpty() )
		continue;

	    const Interval<float> logmdrg =
					logs.getDahRangeForLog( lognm.buf() );
	    if ( logmdrg.isUdf() )
		continue;

	    int perc = 0;
	    if ( logmdrg.includes(markerrg,false) )
		perc = 100;
	    else if ( !logmdrg.overlaps(markerrg,false) )
		perc = 0;
	    else
	    {
		Interval<float> avlogrg = logmdrg;
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
					const TypeSet<Interval<float>> valrgs,
					Array2D<int>& presence ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const int perc = 0;
	ConstRefMan<Well::Data> wd = allwds_[widx];
	if ( !wd )
	    continue;

	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	{
	    for ( int lidx=0; lidx<alllognms.size(); lidx++ )
		presence.set( widx, lidx, perc );
	}

	const Well::LogSet& logs = wd->logs();
	for ( const auto* mn : mns )
	{
	    const int idx = mns.indexOf( mn );
	    const TypeSet<int> logidxs = logs.getSuitable( *mn );
	    for ( const auto& logidx : logidxs )
	    {
		const BufferString lognm = logs.getLogNameByIdx( logidx );
		const int lidx = alllognms.indexOf( lognm );
		if ( !lognms.isPresent(lognm.buf()) )
		{
		    presence.set( widx, lidx, perc );
		    continue;
		}

		const Interval<float> valrg =
				logs.getValueRangeForLog( lognm.buf() );
		const bool perczero = !valrgs[idx].includes( valrg );
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
	ConstRefMan<Well::Data> wd = allwds_.get( widx );
	const bool haswellnm = wd && wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd.ptr(),
							  topnm, botnm, false );
	if ( markerrg.isUdf() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	BufferStringSet currwelllognms;
	const Well::LogSet& logs = wd->logs();
	for ( int logidx=0; logidx<logs.size(); logidx++ )
	{
	    const BufferString lognm = logs.getLogNameByIdx( logidx );
	    const Interval<float> logmdrg =
					logs.getDahRangeForLog( lognm.buf() );
	    if ( logmdrg.isUdf() || !logmdrg.overlaps(markerrg,false) )
		continue;

	    currwelllognms.add( lognm.buf() );
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
	ConstRefMan<Well::Data> wd = allwds_.get( widx );
	const bool haswellnm = wd && wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	const Interval<float> markerrg = getDepthRangeFromMarkers( wd.ptr(),
							topnm, botnm, false );
	if ( markerrg.isUdf() )
	{
	    wellnms.remove( wd->name() );
	    continue;
	}

	MnemonicSelection currwellmns;
	const Well::LogSet& logs = wd->logs();
	for ( int logidx=0; logidx<logs.size(); logidx++ )
	{
	    const BufferString lognm = logs.getLogNameByIdx( logidx );
	    const Interval<float> logmdrg =
					logs.getDahRangeForLog( lognm.buf() );
	    if ( logmdrg.isUdf() || !logmdrg.overlaps(markerrg,false) )
		continue;

	    currwellmns.addIfNew( logs.getMnemonicOfLog(lognm.buf()) );
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
	if ( !wd )
	    continue;

	MnemonicSelection currwellmns;
	const Well::LogSet& logs = wd->logs();
	for ( int logidx=0; logidx<logs.size(); logidx++ )
	{
	    const BufferString lognm = logs.getLogNameByIdx( logidx );
	    Interval<float> logmdrg = logs.getDahRangeForLog( lognm.buf() );
	    if ( logmdrg.isUdf() )
		continue;

	    logmdrg.start_ = wd->track().getPos(logmdrg.start_).z_;
	    logmdrg.stop_ = wd->track().getPos(logmdrg.stop_).z_;
	    if ( !logmdrg.overlaps(depthrg,false) )
		continue;

	    currwellmns.addIfNew( logs.getMnemonicOfLog(lognm.buf()) );
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
	if ( !wd )
	    continue;

	BufferStringSet currwelllognms;
	const Well::LogSet& logs = wd->logs();
	for ( int logidx=0; logidx<logs.size(); logidx++ )
	{
	    const BufferString lognm = logs.getLogNameByIdx( logidx );
	    Interval<float> logmdrg = logs.getDahRangeForLog( lognm.buf() );
	    if ( logmdrg.isUdf() )
		continue;

	    logmdrg.start_ = wd->track().getPos(logmdrg.start_).z_;
	    logmdrg.stop_ = wd->track().getPos(logmdrg.stop_).z_;
	    if ( !logmdrg.overlaps(depthrg,false) )
		continue;

	    currwelllognms.add( lognm.buf() );
	}

	if ( currwelllognms.isEmpty() )
	    continue;

	wellnms.add( wd->name() );
	lognms.add( currwelllognms, false );
    }
}


void WellDataFilter::getLogsInValRange( const MnemonicSelection& mns,
					const TypeSet<Interval<float>> valrgs,
					BufferStringSet& wellnms,
					BufferStringSet& lognms ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( !wd )
	    continue;

	BufferStringSet lognames;
	const Well::LogSet& logs = wd->logs();
	for ( const auto* mn : mns )
	{
	    const int idx = mns.indexOf( mn );
	    const TypeSet<int> logidxs = logs.getSuitable( *mn );
	    for ( const auto& logidx : logidxs )
	    {
		const BufferString lognm = logs.getLogNameByIdx( logidx );
		const Interval<float> logvalrg =
				logs.getValueRangeForLog( lognm.buf());
		if ( logvalrg.isUdf() )
		    continue;

		if ( !valrgs[idx].includes(logvalrg) )
		    continue;

		lognames.add( lognm.buf() );
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
	markerrg.start_ = vertical ? wd->track().zRange().start_
				   : wd->track().dahRange().start_;
    else
    {
	const Well::Marker* marker = wd->markers().getByName( topnm );
	if ( marker )
	{
	    float mrkrdahstart = marker->dah();
            markerrg.start_ = vertical ? wd->track().getPos(mrkrdahstart).z_
				      : mrkrdahstart;
	}
    }

    if ( StringView(botnm) == Well::ZRangeSelector::sKeyDataEnd() )
	markerrg.stop_ = vertical ? wd->track().zRange().stop_
				  : wd->track().dahRange().stop_;
    else
    {
	const Well::Marker* marker = wd->markers().getByName( botnm );
	if ( marker )
	{
	    float mrkrdahstop = marker->dah();
            markerrg.stop_ = vertical ? wd->track().getPos(mrkrdahstop).z_
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
	if ( !wd )
	    continue;

	const Well::LogSet& logs = wd->logs();
	for ( const auto* mn : mns )
	{
	    const TypeSet<int> idxs = logs.getSuitable( *mn );
	    for ( const auto& logidx : idxs )
		lognms.addIfNew( logs.getLogNameByIdx(logidx) );
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
