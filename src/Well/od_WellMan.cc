/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "serverprogtool.h"

#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "dbkey.h"
#include "ioman.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"
#include "stringbuilder.h"
#include "scaler.h"
#include "unitofmeasure.h"


static const int cProtocolNr = 1;

static const char* sListWellsCmd	= ServerProgTool::sListUsrCmd();
static const char* sInfoCmd		= ServerProgTool::sInfoUsrCmd();
static const char* sListLogsCmd		= "list-logs";
static const char* sListMarkersCmd	= "list-markers";
static const char* sReadTrackCmd	= "read-track";
static const char* sReadLogCmd		= "read-log";
static const char* sReadLogsCmd		= "read-logs";

static const char* sWithTVDArg		= "with-tvd";
static const char* sZstep		= "zstep";


using namespace Well;


class WellServerTool : public ServerProgTool
{
public:
			WellServerTool(int,char**);
			~WellServerTool();

    void		listWells();
    void		getWellInfo();
    void		getTrack();
    void		listLogs();
    void		listMarkers();
    void		readLog(const DBKey&,const char*,bool withtvd);
    void		readLogs(const DBKey&, const TypeSet<int>&,
				 const float, bool withtvd);

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


WellServerTool::~WellServerTool()
{
    Well::MGR().cleanup();
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
    if ( !wd_ )	return;

    set( sKey::ID(), wd_->multiID() );
    set( sKey::Name(), wd_->name() );

    const Info& inf = wd_->info();
    const BufferString uwi = inf.uwid_;
    if ( !uwi.isEmpty() )
	set( Info::sKeyUwid(), uwi );
    const OD::WellType wt = inf.welltype_;
    if ( wt != OD::UnknownWellType )
	set( sKey::Type(), OD::toString(wt) );

    set( sKey::X(), inf.surfacecoord_.x );
    set( sKey::Y(), inf.surfacecoord_.y );

    respondInfo( true );
}


void WellServerTool::listLogs()
{
    const DBKey wellid = getDBKey( sListLogsCmd );
    BufferStringSet lognms;
    MGR().getLogNamesByID( wellid, lognms );
    set( sKey::ID(), wellid );
    set( sKey::Size(), lognms.size() );
    set( sKey::Names(), lognms );
    respondInfo( true );
}


void WellServerTool::listMarkers()
{
    getWD( sListMarkersCmd, LoadReqs(Mrkrs) );
    if ( !wd_ )	return;

    BufferStringSet nms;
    TypeSet<OD::Color> colors;
    TypeSet<float> mds;
    wd_->markers().getNamesColorsMDs( nms, colors, mds );
    BufferStringSet colstrs;
    OD::Color::convertToStr( colors, colstrs );
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
    if ( !wd_ )	return;

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
			      bool withtvd )
{
    getWD( wellid, LoadReqs(Inf, Trck) );
    if ( !wd_ )	return;

    const Log* wl = wd_->getLog( lognm );
    if ( !wl )
    {
	respondError( "Log not found" );
	return;
    }

    const auto sz = wl->size();
    set( sKey::Well(), wd_->name() );
    set( sKey::Name(), wl->name() );
    set( sKey::Size(), wl->size() );
    const auto* uom = wl->unitOfMeasure();
    set( sKey::Unit(), uom ? uom->symbol() : "" );
    set( sKey::Scale(), uom ? uom->scaler().scale(1) : double(1) );

    TypeSet<float> mds, vals, tvds;
    const Track* trck = withtvd ? &wd_->track() : nullptr;
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


void WellServerTool::readLogs( const DBKey& wellid,
    const TypeSet<int>& logids,
    const float zstep, bool withtvd )
{
    getWD( wellid, LoadReqs( Inf, Trck, LogInfos ) );
    if ( !wd_ )	return;

    BufferStringSet lognms, sellognms;
    Well::MGR().getLogNamesByID( wellid, lognms );
    for ( auto idx = 0; idx < logids.size(); idx++ )
    {
	const int logid = logids[idx];
	if ( !lognms.validIdx( logid ) )
	{
	    respondError( "Invalid log index" );
	    return;
	}

	const BufferString lognm = lognms.get( logid );
	wd_->getLog( lognm );
	sellognms.add( lognm );
    }

    const Track* trck = withtvd ? &wd_->track() : nullptr;
    Well::ExtractParams extparams;
    Interval<float> extrange = extparams.calcFrom( *wd_, sellognms, false );
    extparams.setFixedRange( extrange, false );
    extparams.zstep_ = zstep;
    Well::LogSampler sampler( *wd_, extparams, sellognms );
    sampler.setMaxHoleSize( 1.05 );
    if ( !sampler.execute() )
    {
	respondError( sampler.errMsg() );
	return;
    }

    BufferStringSet outlognames;
    for ( auto idx=0; idx<sellognms.size(); idx++ )
    {
	StringBuilder lognm( sellognms.get( idx ) );
	StringBuilder uomlbl( sampler.uomLabel( idx ) );
	if ( !uomlbl.isEmpty() )
	    lognm.addSpace().add('(').add(uomlbl.str()).add(')');
	outlognames.add( lognm.result() );
    }
    TypeSet<float> mds( sampler.nrZSamples(), mUdf(float) );
    TypeSet<float> tvds( sampler.nrZSamples(), mUdf(float) );
    float* mdarr = mds.arr();
    float* tvdarr = tvds.arr();
    for ( auto zidx=0; zidx<sampler.nrZSamples(); zidx++ )
    {
	const float md = sampler.getDah(zidx);
	mdarr[zidx] = md;
	if ( trck )
	    tvdarr[zidx] = trck->getPos( md ).z;
    }
    set( sKey::Well(), wd_->name() );
    set( sKey::Size(), sampler.nrZSamples() );
    set( sKey::Logs(), sellognms.size() );
    set( sKey::Names(), outlognames );
    set( sKey::MD(2), mds );
    for ( auto idx=0; idx<sellognms.size(); idx++ )
    {
	TypeSet<float> vals( sampler.nrZSamples(), mUdf(float) );
	float* valarr = vals.arr();
	for ( auto zidx=0; zidx<sampler.nrZSamples(); zidx++ )
	    valarr[zidx] = sampler.getLogVal( idx, zidx );

	BufferString tmp( "Log_", idx );
	set( tmp, vals );
    }
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
    BufferString argstr( "well_id log_name [--", sWithTVDArg, "]" );
    addToUsageStr( ret, sReadLogCmd, argstr );
    argstr = BufferString( "well_id log_idx_list [--zstep step] ");
    argstr.add("[--").add(sWithTVDArg).add("]");
    addToUsageStr( ret, sReadLogsCmd, argstr );
    return ret;
}


int mProgMainFnName( int argc, char** argv )
{
    ApplicationData app;
    WellServerTool st( argc, argv );
    CommandLineParser& clp = st.clp();

    if ( clp.hasKey(sListWellsCmd) )
    {
	st.listWells();
	return app.exec();
    }

    if ( clp.hasKey(sInfoCmd) )
	st.getWellInfo();
    else if ( clp.hasKey(sReadTrackCmd) )
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
	clp.getVal( sReadLogCmd, lognm, false, 2 );
	const bool withtvd = clp.hasKey( sWithTVDArg );
	st.readLog( wellid, lognm, withtvd );
    }
    else if ( clp.hasKey(sReadLogsCmd) )
    {
	clp.setKeyHasValue( sReadLogsCmd, 2 );
	DBKey wellid;
	clp.getVal( sReadLogsCmd, wellid );
	BufferString logid_str;
	clp.getVal( sReadLogsCmd, logid_str, false, 2 );
	BufferStringSet logids_strs;
	logids_strs.unCat( logid_str, "," );
	TypeSet<int> logids;
	for ( auto idx = 0; idx < logids_strs.size(); idx++ )
	    logids.addIfNew( toInt( logids_strs.get( idx ) ) );
	const bool withtvd = clp.hasKey( sWithTVDArg );
	float zstep = 0.5;
	if ( clp.hasKey(sZstep) )
	{
	    clp.setKeyHasValue( sZstep, 1 );
	    clp.getVal( sZstep, zstep );
	}
	st.readLogs( wellid, logids, zstep, withtvd );
    }
    else
	st.exitWithUsage();

    return app.exec();
}
