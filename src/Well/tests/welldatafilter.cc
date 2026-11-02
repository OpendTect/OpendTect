/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Batch test for Well::WellDataFilter. Uses F3_Test_Survey wells
 (same MultiIDs as test_welldata / test_welltablemodel).

-*/

#include "batchprog.h"

#include "arrayndimpl.h"
#include "oddirs.h"
#include "plugins.h"
#include "welldatafilter.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"


MultiID F034mid_, wellAmid_, F061mid_, wellBmid_, wellCmid_, wellDmid_;


static bool loadOpendTectPlugins( const char* piname )
{
    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( piname, libnm.getCStr(), libnm.bufSize() );
    FilePath libfp( GetLibPlfDir(), libnm );
    if ( !libfp.exists() )
	libfp.setPath( PIM().getAutoDir(true) );

    return libfp.exists()
		? PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
			      PI_AUTO_INIT_EARLY )
		: false;
}


static BufferString sizeMismatchErr( int expected, int actual )
{
    BufferString err( "Expected size: ", expected );
    err.add( "; actual size: " ).add( actual );
    return err;
}


static bool loadWells( const TypeSet<MultiID>& keys,
		       RefObjectSet<Well::Data>& wds )
{
    wds.setEmpty();
    Well::LoadReqs reqs( Well::Inf, Well::Trck, Well::Mrkrs );
    reqs.add( Well::LogInfos );
    MultiWellReader rdr( keys, wds, reqs );
    mRunStandardTestWithError( rdr.execute() && !rdr.hasFails(),
			       "Load survey wells for WellDataFilter",
			       rdr.allMessages().getText() )
    mRunStandardTestWithError( wds.size()==keys.size(),
			       "Loaded well count matches request",
			       sizeMismatchErr(keys.size(),wds.size()) )
    return true;
}


static void collectLogNames( const ObjectSet<Well::Data>& wds,
			     BufferStringSet& lognms )
{
    lognms.setEmpty();
    for ( const auto* wd : wds )
    {
	if ( !wd )
	    continue;

	BufferStringSet nms;
	wd->logs().getNames( nms );
	lognms.add( nms, false );
    }
}


static void collectMnemonics( const ObjectSet<Well::Data>& wds,
			      MnemonicSelection& mns )
{
    mns.setEmpty();
    for ( const auto* wd : wds )
    {
	if ( !wd )
	    continue;

	MnemonicSelection wdmns;
	wd->logs().getAllAvailMnems( wdmns );
	for ( const auto* mn : wdmns )
	    mns.addIfNew( mn );
    }
}


static void collectWellNames( const ObjectSet<Well::Data>& wds,
			      BufferStringSet& wellnms )
{
    wellnms.setEmpty();
    for ( const auto* wd : wds )
    {
	if ( wd )
	    wellnms.add( wd->name() );
    }
}


static bool testWellSelection( const ObjectSet<Well::Data>& wds,
			       const Well::WellDataFilter& wdf )
{
    BufferStringSet alllognms;
    collectLogNames( wds, alllognms );
    mRunStandardTest( !alllognms.isEmpty(),
		      "Survey wells provide at least one log name" )

    BufferStringSet wellnms;
    wdf.getWellsFromLogs( BufferStringSet(alllognms.get(0).buf()),
			  wellnms, false );
    mRunStandardTest( !wellnms.isEmpty(),
		      "getWellsFromLogs finds wells for a known log" )

    BufferStringSet markernms;
    for ( const auto* wd : wds )
    {
	if ( wd && !wd->markers().isEmpty() )
	{
	    wd->markers().getNames( markernms );
	    break;
	}
    }

    mRunStandardTest( !markernms.isEmpty(),
		      "Survey wells provide at least one marker" )
    wellnms.setEmpty();
    wdf.getWellsFromMarkers( BufferStringSet(markernms.first()->buf()),
			     wellnms, false );
    mRunStandardTest( !wellnms.isEmpty(),
		      "getWellsFromMarkers finds wells for a known marker" )

    BufferStringSet selectedwells;
    collectWellNames( wds, selectedwells );
    BufferStringSet outlogs, outmarkernms;
    MnemonicSelection outmns;
    wdf.getMarkersLogsMnemsFromWells( selectedwells, outlogs, outmns,
				      outmarkernms, false );
    mRunStandardTest( !outlogs.isEmpty() && !outmarkernms.isEmpty(),
		      "getMarkersLogsMnemsFromWells returns union attributes" )

    return true;
}


static bool testLogPresenceSize( const ObjectSet<Well::Data>& wds,
				 const Well::WellDataFilter& wdf )
{
    BufferStringSet alllognms;
    collectLogNames( wds, alllognms );
    BufferStringSet wellnms;
    collectWellNames( wds, wellnms );
    MnemonicSelection mns;
    collectMnemonics( wds, mns );

    mRunStandardTest( !alllognms.isEmpty() && !mns.isEmpty(),
		      "Logs and mnemonics available for presence size tests" )

    const int nrwells = wds.size();
    const int nrlogs = alllognms.size();
    const int nrmns = mns.size();
    const char* topnm = Well::ZRangeSelector::sKeyDataStart();
    const char* botnm = Well::ZRangeSelector::sKeyDataEnd();

    Array2DImpl<int> presence( nrwells, nrlogs );
    mRunStandardTestWithError(
	presence.info().getSize(0)==nrwells &&
	presence.info().getSize(1)==nrlogs,
	"Presence array sized to nrwells x nrlogs",
	sizeMismatchErr(nrwells*nrlogs,
			presence.info().getSize(0)*presence.info().getSize(1)))

    wdf.getLogPresence( wellnms, topnm, botnm, alllognms, presence );
    mRunStandardTestWithError(
	presence.info().getSize(0)==nrwells &&
	presence.info().getSize(1)==nrlogs,
	"getLogPresence keeps array size nrwells x nrlogs",
	sizeMismatchErr(nrwells,presence.info().getSize(0)) )

    bool allvalid = true;
    bool sawdefined = false;
    for ( int widx=0; widx<nrwells; widx++ )
    {
	for ( int lidx=0; lidx<nrlogs; lidx++ )
	{
	    if ( !presence.info().validPos(widx,lidx) )
	    {
		allvalid = false;
		break;
	    }

	    const int perc = presence.get( widx, lidx );
	    if ( perc < -1 || perc > 100 )
	    {
		allvalid = false;
		break;
	    }

	    if ( perc >= 0 )
		sawdefined = true;
	}
    }

    mRunStandardTest( allvalid,
		      "getLogPresence values are in [-1,100] at "
		      "valid positions")
    mRunStandardTest( sawdefined,
		      "getLogPresence sets at least one defined presence value")

    Array2DImpl<int> mnempresence( nrwells, nrmns );
    mRunStandardTestWithError(
	mnempresence.info().getSize(0)==nrwells &&
	mnempresence.info().getSize(1)==nrmns,
	"Mnemonic presence array sized to nrwells x nrmns",
	sizeMismatchErr(nrmns,mnempresence.info().getSize(1)) )

    wdf.getLogPresenceForMnems( wellnms, topnm, botnm, mns, mnempresence );
    mRunStandardTestWithError(
	mnempresence.info().getSize(0)==nrwells &&
	mnempresence.info().getSize(1)==nrmns,
	"getLogPresenceForMnems keeps array size nrwells x nrmns",
	sizeMismatchErr(nrmns,mnempresence.info().getSize(1)) )

    Array2DImpl<int> valpresence( nrwells, nrlogs );
    valpresence.setAll( -1 );
    TypeSet<Interval<float>> valrgs;
    for ( int idx=0; idx<nrmns; idx++ )
	valrgs += Interval<float>( -1e6f, 1e6f );

    wdf.getLogPresenceFromValFilter( wellnms, alllognms, alllognms, mns,
				     valrgs, valpresence );
    mRunStandardTestWithError(
	valpresence.info().getSize(0)==nrwells &&
	valpresence.info().getSize(1)==nrlogs,
	"getLogPresenceFromValFilter keeps array size nrwells x nrlogs",
	sizeMismatchErr(nrlogs,valpresence.info().getSize(1)) )

    return true;
}


mLoad1Module("Well")

bool BatchProgram::doWork( od_ostream& /*strm*/ )
{
    if ( !loadOpendTectPlugins("ODHDF5") )
	return false;

    mRunStandardTest( pars().get("F03-4",F034mid_), "MultiID for F03-4" )
    mRunStandardTest( pars().get("Well A",wellAmid_), "MultiID for Well A" )
    mRunStandardTest( pars().get("F06-1",F061mid_), "MultiID for F06-1" )
    mRunStandardTest( pars().get("Well B",wellBmid_), "MultiID for Well B" )
    mRunStandardTest( pars().get("Well C",wellCmid_), "MultiID for Well C" )
    mRunStandardTest( pars().get("Well D",wellDmid_), "MultiID for Well D" )

    TypeSet<MultiID> keys;
    keys += F034mid_;
    keys += wellAmid_;
    keys += F061mid_;
    keys += wellBmid_;

    RefObjectSet<Well::Data> wds;
    if ( !loadWells(keys,wds) )
	return false;

    const Well::WellDataFilter wdf( wds );
    if ( !testWellSelection(wds,wdf) ||
	 !testLogPresenceSize(wds,wdf) )
	return false;

    return true;
}
