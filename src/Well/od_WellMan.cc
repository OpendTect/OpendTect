/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltrack.h"

#include "commandlineparser.h"
#include "dbkey.h"
#include "ioman.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"
#include "scaler.h"
#include "unitofmeasure.h"


static const int cProtocolNr = 1;

static const char* sListWellsCmd	= ServerProgTool::sListUsrCmd();
static const char* sInfoCmd		= ServerProgTool::sInfoUsrCmd();
static const char* sListLogsCmd		= "list-logs";
static const char* sListMarkersCmd	= "list-markers";
static const char* sReadTrackCmd	= "read-track";
static const char* sReadLogCmd		= "read-log";

static const char* sNoTVDArg		= "no-tvd";

using namespace Well;


class WellServerTool : public ServerProgTool
{
public:
			WellServerTool(int,char**);

    void		listWells();
    void		getWellInfo();
    void		getTrack();
    void		listLogs();
    void		listMarkers();
    void		readLog(const DBKey&,const char*,bool notvd);

protected:

    ConstRefMan<Data>	wd_;

    BufferString	getSpecificUsage() const override;
    void		getWD(const DBKey&,const LoadReqs&);
    void		getWD(const char*,const LoadReqs&);

};


WellServerTool::WellServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"Well")
{
    initParsing( cProtocolNr );
}


void WellServerTool::listWells()
{
    TypeSet<MultiID> wellids;
    Well::Man::getWellKeys( wellids );
    set( sKey::Size(), wellids.size() );
    DBKeySet dbkeys; dbkeys = wellids;
    set( sKey::IDs(), dbkeys );
    BufferStringSet nms;
    for ( auto wellid : wellids )
	nms.add( IOM().nameOf(wellid) );
    set( sKey::Names(), nms );
    respondInfo( true );
}


void WellServerTool::getWD( const DBKey& wellid, const LoadReqs& lreqs )
{
    wd_ = MGR().get( wellid, lreqs );
    if ( !wd_ )
    {
	uiRetVal uirv( toUiString(MGR().errMsg()) );
	respondError( uirv );
    }
}


void WellServerTool::getWD( const char* cmd, const LoadReqs& lreqs )
{
    getWD( getDBKey(cmd), lreqs );
}


void WellServerTool::getWellInfo()
{
    getWD( sInfoCmd, LoadReqs(Inf) );

    set( sKey::ID(), wd_->multiID().buf() );
    set( sKey::Name(), wd_->name() );

    const Info& inf = wd_->info();
    const BufferString uwi = inf.uwid;
    if ( !uwi.isEmpty() )
	set( Info::sKeyUwid(), uwi );
    const Info::WellType wt = inf.welltype_;
    if ( wt != Info::None )
	set( sKey::Type(), Info::toString(wt) );

    set( sKey::X(), inf.surfacecoord.x );
    set( sKey::Y(), inf.surfacecoord.y );

    respondInfo( true );
}


void WellServerTool::listLogs()
{
    const DBKey wellid = getDBKey( sListLogsCmd );
    BufferStringSet lognms;
    MGR().getLogNames( wellid, lognms );
    set( sKey::ID(), wellid );
    set( sKey::Size(), lognms.size() );
    set( sKey::Names(), lognms );
    respondInfo( true );
}


void WellServerTool::listMarkers()
{
    getWD( sListMarkersCmd, LoadReqs(Mrkrs) );

    BufferStringSet nms;
    TypeSet<Color> colors;
    TypeSet<float> mds;
    wd_->markers().getNamesColorsMDs( nms, colors, mds );
    BufferStringSet colstrs;
    Color::convertToStr( colors, colstrs );
    set( sKey::ID(), DBKey(wd_->multiID()) );
    set( sKey::Size(), nms.size() );
    set( sKey::Names(), nms );
    set( sKey::Color(), colstrs );
    set( sKey::MD(2), mds );
    respondInfo( true );
}


void WellServerTool::getTrack()
{
    getWD( sReadTrackCmd, LoadReqs(Trck) );

    const Track& track = wd_->track();
    TypeSet<float> mds, tvds;
    TypeSet<double> xs, ys;
    const int sz = track.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const float md = track.dah( idx );
	const Coord3& pos = track.pos( idx );
	mds += md;
	xs += pos.x;
	ys += pos.y;
	tvds += sCast(float,pos.z);
    }

    set( sKey::Size(), mds.size() );
    set( sKey::MD(2), mds );
    set( sKey::TVD(2), tvds );
    set( sKey::XCoords(), xs );
    set( sKey::YCoords(), ys );

    respondInfo( true );
}


void WellServerTool::readLog( const DBKey& wellid, const char* lognm,
			      bool notvd )
{
    ConstRefMan<Data> wd = MGR().get( wellid, Logs );
    const Log* wl = wd ? wd->logs().getLog( lognm ) : nullptr;
    if ( !wl )
	respondError( "Log not found" );

    if ( !notvd )
	getWD( wellid, LoadReqs(Inf,Trck) );

    const auto sz = wl->size();
    set( sKey::Well(), wd->name() );
    set( sKey::Name(), wl->name() );
    set( sKey::Size(), wl->size() );
    const auto* uom = wl->unitOfMeasure();
    set( sKey::Unit(), uom ? uom->symbol() : "" );
    set( sKey::Scale(), uom ? uom->scaler().scale(1) : double(1) );

    TypeSet<float> mds, vals, tvds;
    const Track* trck = wd_ ? &wd_->track() : nullptr;
    set( sKey::Size(), sz );
    for ( auto idx=0; idx<sz; idx++ )
    {
	const float md = wl->dah( idx );
	mds += md;
	vals += wl->value( idx );
	if ( trck )
	    tvds += trck->getPos( md ).z;
    }

    set( sKey::MD(2), mds );
    set( sKey::Values(), vals );
    if ( trck )
	set( sKey::TVD(2), tvds );

    respondInfo( true );
}


BufferString WellServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListWellsCmd, "" );
    addToUsageStr( ret, sInfoCmd, "well_id" );
    addToUsageStr( ret, sListLogsCmd, "well_id" );
    addToUsageStr( ret, sListMarkersCmd, "well_id" );
    addToUsageStr( ret, sReadTrackCmd, "well_id" );
    BufferString argstr( "well_id log_name [--", sNoTVDArg, "]" );
    addToUsageStr( ret, sReadLogCmd, argstr );
    return ret;
}


int main( int argc, char** argv )
{
    WellServerTool st( argc, argv );
    auto& clp = st.clp();

    if ( clp.hasKey(sListWellsCmd) )
	st.listWells();

    if ( clp.hasKey(sInfoCmd) )
	st.getWellInfo();
    if ( clp.hasKey(sReadTrackCmd) )
	st.getTrack();
    else if ( clp.hasKey(sListLogsCmd) )
	st.listLogs();
    else if ( clp.hasKey(sListMarkersCmd) )
	st.listMarkers();
    else if ( clp.hasKey(sReadLogCmd) )
    {
	clp.setKeyHasValue( sReadLogCmd, 2 );
	DBKey wellid;
	clp.getVal( sReadLogCmd, wellid );
	BufferString lognm;
	clp.getVal( sReadLogCmd, lognm, false, 1 );
	const bool notvd = clp.hasKey( sNoTVDArg );
	st.readLog( wellid, lognm, notvd );
    }

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
