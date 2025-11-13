/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "executor.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "oddirs.h"
#include "plugins.h"
#include "surveydisklocation.h"
#include "welldata.h"
#include "welldisp.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"


MultiID F034mid_, F061mid_, wellAmid_, wellBmid_, wellCmid_, wellDmid_;

static Well::LoadReqs inforeqs_( false );
static Well::LoadReqs trackreqs_( false );
static Well::LoadReqs d2treqs_( false );
static Well::LoadReqs csmdlreqs_( false );
static Well::LoadReqs markersreqs_( false );
static Well::LoadReqs loginfosreqs_( false );
static Well::LoadReqs logsreqs_( false );
static Well::LoadReqs dispprop2dreqs_( false );
static Well::LoadReqs dispprop3dreqs_( false );

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


static bool testLoadReqs()
{
    mRunStandardTest( inforeqs_.includes(Well::Inf), "Info LoadReqs" )
    mRunStandardTest( trackreqs_.includes(Well::Trck), "Track LoadReqs" )
    mRunStandardTest( d2treqs_.includes(Well::D2T),
		      "Depth-to-time model LoadReqs" )
    mRunStandardTest( csmdlreqs_.includes(Well::CSMdl),
		      "CheckShot model LoadReqs" )
    mRunStandardTest( markersreqs_.includes(Well::Mrkrs), "Markers LoadReqs" )
    mRunStandardTest( loginfosreqs_.includes(Well::LogInfos),
		      "Log infos LoadReqs" )
    mRunStandardTest( logsreqs_.includes(Well::Logs), "Logs LoadReqs" )
    mRunStandardTest( dispprop2dreqs_.includes(Well::DispProps2D),
		      "Display 2D properties LoadReqs" )
    mRunStandardTest( dispprop3dreqs_.includes(Well::DispProps3D),
		      "Display 3D properties LoadReqs" )

    const Well::LoadReqs sallreqs = Well::LoadReqs::All();
    mRunStandardTest( sallreqs.includes(Well::Inf) &&
		      sallreqs.includes(Well::Trck) &&
		      sallreqs.includes(Well::D2T) &&
		      sallreqs.includes(Well::CSMdl) &&
		      sallreqs.includes(Well::Mrkrs) &&
		      sallreqs.includes(Well::Logs) &&
		      sallreqs.includes(Well::LogInfos) &&
		      sallreqs.includes(Well::DispProps2D) &&
		      sallreqs.includes(Well::DispProps3D) &&
		      sallreqs.logNames().isEmpty(),
		      "Static Well::LoadReqs::All() includes everything" )

    const Well::LoadReqs allreqs = Well::LoadReqs( true );
    mRunStandardTest( allreqs.includes(Well::Inf) &&
		      allreqs.includes(Well::Trck) &&
		      allreqs.includes(Well::D2T) &&
		      allreqs.includes(Well::CSMdl) &&
		      allreqs.includes(Well::Mrkrs) &&
		      allreqs.includes(Well::Logs) &&
		      allreqs.includes(Well::LogInfos) &&
		      allreqs.includes(Well::DispProps2D) &&
		      allreqs.includes(Well::DispProps3D) &&
		      allreqs.logNames().isEmpty(),
		      "Well::LoadReqs(true) includes everything" )

    const Well::LoadReqs noreqs = Well::LoadReqs( false );
    mRunStandardTest( !noreqs.includes(Well::Inf) &&
		      !noreqs.includes(Well::Trck) &&
		      !noreqs.includes(Well::D2T) &&
		      !noreqs.includes(Well::CSMdl) &&
		      !noreqs.includes(Well::Mrkrs) &&
		      !noreqs.includes(Well::Logs) &&
		      !noreqs.includes(Well::LogInfos) &&
		      !noreqs.includes(Well::DispProps2D) &&
		      !noreqs.includes(Well::DispProps3D) &&
		      noreqs.logNames().isEmpty(),
		      "Well::LoadReqs(false) includes nothing" )

    const Well::LoadReqs sallnologsreqs = Well::LoadReqs::AllNoLogs();
    mRunStandardTest( sallnologsreqs.includes(Well::Inf) &&
		      sallnologsreqs.includes(Well::Trck) &&
		      sallnologsreqs.includes(Well::D2T) &&
		      sallnologsreqs.includes(Well::CSMdl) &&
		      sallnologsreqs.includes(Well::Mrkrs) &&
		      !sallnologsreqs.includes(Well::Logs) &&
		      sallnologsreqs.includes(Well::LogInfos) &&
		      sallnologsreqs.includes(Well::DispProps2D) &&
		      sallnologsreqs.includes(Well::DispProps3D) &&
		      sallnologsreqs.logNames().isEmpty(),
      "Static Well::LoadReqs::AllNoLogs() includes everything but logs" )

    Well::LoadReqs lreqs = Well::LoadReqs::All();
    lreqs.setEmpty();
    mRunStandardTest( lreqs == noreqs, "Well::LoadReqs::setEmpty()" )

    lreqs = noreqs;
    lreqs.setToAll();
    mRunStandardTest( lreqs == allreqs, "Well::LoadReqs::setToAll()" )

    lreqs = sallnologsreqs;
    lreqs.exclude( Well::LoadReqs( Well::LogInfos ) );
    mRunStandardTest( !lreqs.includes(Well::LogInfos) &&
		      !lreqs.includes(Well::Logs) &&
		      lreqs.logNames().isEmpty(),
		      "Well::LoadReqs with neither LogInfos nor Logs" )

    const Well::LoadReqs nologsreqs = lreqs;
    Well::LoadReqs singlelogreqs1 = nologsreqs, singlelogreqs2 = nologsreqs;
    singlelogreqs1.addLog( "Density" );
    singlelogreqs2.addLog( "PHI" );
    Well::LoadReqs multilogreqs1 = nologsreqs, multilogreqs2 = nologsreqs,
		   multilogreqs3 = nologsreqs;
    multilogreqs1.addLogs( BufferStringSet("Density","DT","GR") );
    multilogreqs2.addLogs( BufferStringSet("GR","PHI","SW") );
    multilogreqs3.addLogs( BufferStringSet("SW","GR","PHI") );
    mRunStandardTest( singlelogreqs1.includes(Well::LogInfos) &&
		      !singlelogreqs1.includes(Well::Logs) &&
		      singlelogreqs1.logNames().size() == 1,
		      "Single log Well::LoadReqs" )
    mRunStandardTest( multilogreqs1.includes(Well::LogInfos) &&
		      !multilogreqs1.includes(Well::Logs) &&
		      multilogreqs1.logNames().size() == 3,
		      "Multi logs Well::LoadReqs" )
    mRunStandardTest( multilogreqs1.includes(singlelogreqs1) &&
		      !multilogreqs1.includes(singlelogreqs2),
		      "Multi to single Well::LoadReqs includes" )
    mRunStandardTest( !singlelogreqs1.includes(multilogreqs1) &&
		      !singlelogreqs1.includes(multilogreqs2),
		      "Single to multi Well::LoadReqs includes" )
    mRunStandardTest( !multilogreqs1.includes(multilogreqs2),
		      "Multi to multi Well::LoadReqs includes (false)" )
    mRunStandardTest( multilogreqs3.includes(multilogreqs2),
		      "Multi to multi Well::LoadReqs includes (true)" )

    multilogreqs3.include( singlelogreqs1 );
    mRunStandardTest( multilogreqs3.logNames().size() == 4 &&
		      multilogreqs3.includes(singlelogreqs1),
		      "Well::LoadReqs include (one more log)" )

    singlelogreqs2 = noreqs;
    singlelogreqs2.addLog( "PHI" );
    multilogreqs3.exclude( singlelogreqs2 );
    mRunStandardTest( multilogreqs3.includes(nologsreqs) &&
		      multilogreqs3.includes(Well::LogInfos) &&
		      !multilogreqs3.includes(Well::Logs) &&
		      multilogreqs3.logNames().size() == 3,
		      "Well::LoadReqs include (one log less)" )

    return true;
}


static bool testIOAttribs()
{
    mRunStandardTest( Well::Reader::canReadInParallel( wellAmid_ ),
		      "OpendTect wells support parallel read" )
    mRunStandardTest( !Well::Reader::canReadInParallel( wellBmid_ ),
		      "HDF5 wells do not support parallel read" )
    mRunStandardTest( Well::Writer::canWriteInParallel( wellAmid_ ),
		      "OpendTect wells support parallel write" )
    mRunStandardTest( !Well::Writer::canWriteInParallel( wellBmid_ ),
		      "HDF5 wells do not support parallel write" )
    mRunStandardTest( Well::Writer::canRenameLogs( wellAmid_ ),
		      "OpendTect wells support log renaming" )
    mRunStandardTest( Well::Writer::canRenameLogs( wellBmid_ ),
		      "HDF5 wells support log renaming" )

    return true;
}


static bool isWellMGREmpty()
{
    return Well::MGR().wells().size() < 1;
}


static bool getAllWD()
{
    TypeSet<MultiID> wellids;
    mRunStandardTest( Well::Man::getWellKeys(wellids) && wellids.size()==6 &&
		      isWellMGREmpty(), "getWellKeys -> size 6" )
    mRunStandardTest( wellids.isPresent(wellAmid_) &&
		      wellids.isPresent(wellBmid_) &&
		      wellids.isPresent(wellCmid_) &&
		      wellids.isPresent(wellDmid_),
		      "Has all required well IDs" )

    BufferStringSet wellnms;
    mRunStandardTest( Well::Man::getWellNames(wellnms) && wellnms.size()==6 &&
		      isWellMGREmpty(), "getWellNames -> size 6" )

    BufferStringSet allmarkernms;
    mRunStandardTest( Well::Man::getAllMarkerNames(allmarkernms) &&
		      allmarkernms.size() == 18 && isWellMGREmpty(),
		      "Get markers from all wells -> size 18" )

    TypeSet<OD::Color> allcols; allmarkernms.setEmpty();
    mRunStandardTest( Well::Man::getAllMarkerInfo(allmarkernms,allcols) &&
		      allmarkernms.size() == 18 && allcols.size() == 18 &&
		      isWellMGREmpty(),
		      "Get markers & colors from all wells -> size 18" )

    BufferStringSet alllognms;
    mRunStandardTest( Well::Man::getAllLogNames(alllognms) &&
		      alllognms.size() == 7 && isWellMGREmpty(),
		      "Get log names from all wells -> size 7" )

    MnemonicSelection mns;
    mRunStandardTest( Well::Man::getAllMnemonics(mns) && mns.size() == 6 &&
		      isWellMGREmpty(),
		      "Get mnemonic selection from all wells -> size 6" )

    return true;
}


static bool getNothing()
{
    const bool onlyloaded = true;
    TypeSet<MultiID> wellids;
    mRunStandardTest( Well::Man::getWellKeys(wellids,onlyloaded) &&
		      wellids.isEmpty() && isWellMGREmpty(),
		      "getWellKeys from no loaded wells should be empty" )

    BufferStringSet wellnms;
    mRunStandardTest( Well::Man::getWellNames(wellnms,onlyloaded) &&
		      wellnms.isEmpty() && isWellMGREmpty(),
		      "getWellNames from no loaded wells should be empty" )

    BufferStringSet allmarkernms;
    mRunStandardTest( Well::Man::getAllMarkerNames(allmarkernms,onlyloaded) &&
		      allmarkernms.isEmpty() && isWellMGREmpty(),
		      "Get markers from no loaded wells should be empty" )

    TypeSet<OD::Color> allcols;
    mRunStandardTest(
	    Well::Man::getAllMarkerInfo(allmarkernms,allcols,onlyloaded) &&
	    allmarkernms.isEmpty() && allcols.isEmpty() && isWellMGREmpty(),
	    "Get markers & colors from no loaded wells should be empty" )

    BufferStringSet alllognms;
    mRunStandardTest( Well::Man::getAllLogNames(alllognms,onlyloaded) &&
		      alllognms.isEmpty() && isWellMGREmpty(),
		      "Get log names from no loaded wells should be empty" )

    MnemonicSelection mns;
    mRunStandardTest( Well::Man::getAllMnemonics(mns,onlyloaded) &&
		      mns.isEmpty() && isWellMGREmpty(),
	      "Get mnemonic selection from no loaded wells should be empty" )

    return true;
}


static bool getSomeWD()
{
    const bool onlyloaded = true;
    TypeSet<MultiID> wellids;
    mRunStandardTest( Well::Man::getWellKeys(wellids,onlyloaded) &&
		      wellids.size()==2 && !isWellMGREmpty(),
		      "getWellKeys -> size 2" )
    mRunStandardTest( wellids.isPresent(wellAmid_) &&
		      wellids.isPresent(wellBmid_) &&
		      !wellids.isPresent(wellCmid_) &&
		      !wellids.isPresent(wellDmid_),
		      "Has all required well IDs (2)" )

    BufferStringSet wellnms;
    mRunStandardTest( Well::Man::getWellNames(wellnms,onlyloaded) &&
		      wellnms.size()==2 && !isWellMGREmpty(),
		      "getWellNames -> size 2" )

    BufferStringSet allmarkernms;
    mRunStandardTest( Well::Man::getAllMarkerNames(allmarkernms,onlyloaded) &&
		      allmarkernms.size() == 7 && !isWellMGREmpty(),
		      "Get markers from two wells -> size 7" )

    TypeSet<OD::Color> allcols; allmarkernms.setEmpty();
    mRunStandardTest(
	    Well::Man::getAllMarkerInfo(allmarkernms,allcols,onlyloaded) &&
		      allmarkernms.size() == 7 && allcols.size() == 7 &&
		      !isWellMGREmpty(),
		      "Get markers & colors from two wells -> size 7" )

    BufferStringSet alllognms;
    mRunStandardTest( Well::Man::getAllLogNames(alllognms,onlyloaded) &&
		      alllognms.size() == 4 && !isWellMGREmpty(),
		      "Get log names from two wells -> size 4" )

    MnemonicSelection mns;
    mRunStandardTest( Well::Man::getAllMnemonics(mns,onlyloaded) &&
		      mns.size() == 4 && !isWellMGREmpty(),
		      "Get mnemonic selection from two wells -> size 4" )

    return true;
}


static bool getWD( const MultiID& wid, bool onlytrackd2t )
{
    RefMan<Well::Data> wd = Well::MGR().get( wid, inforeqs_ );
    mRunStandardTestWithError( wd && wd->info().isLoaded(),
		      BufferString("Has loaded well info: ", wid.toString()),
		      Well::MGR().errMsg().getFullString() )

    const BufferString wellnm = wd->name();

    wd = Well::MGR().get( wid, trackreqs_ );
    mRunStandardTestWithError( wd && wd->track().size() == 2,
		      BufferString("Has loaded track: ", wellnm.buf()),
		      Well::MGR().errMsg().getFullString() )

    wd = Well::MGR().get( wid, d2treqs_ );
    const int mind2tsize = onlytrackd2t ? 2 : 1475;
    mRunStandardTestWithError( wd && wd->d2TModel() &&
			       wd->d2TModel()->size() >= mind2tsize &&
			       wd->nrD2TModels() == 1,
		     BufferString("Has loaded time-depth model: ",wellnm.buf()),
		     Well::MGR().errMsg().getFullString() )

    wd = Well::MGR().get( wid, csmdlreqs_ );
    if ( onlytrackd2t )
    {
	mRunStandardTest( wd && !wd->checkShotModel(),
			  "Reading missing checkshot should be OK/nullptr "
			  "but return Well::Data" )
    }
    else
    {
	mRunStandardTestWithError( wd && wd->checkShotModel() &&
				   wd->checkShotModel()->size() == 17,
		  BufferString("Has loaded checkshot model: ", wellnm.buf()),
		  Well::MGR().errMsg().getFullString() )
    }

    wd = Well::MGR().get( wid, markersreqs_ );
    const int expmarkersz = onlytrackd2t ? 0 : 7;
    mRunStandardTestWithError( wd && wd->markers().size() == expmarkersz,
	    BufferString("Has loaded markers: ", wellnm.buf()),
	    Well::MGR().errMsg().getFullString() )

    wd = Well::MGR().get( wid, loginfosreqs_ );
    const int explogsz = onlytrackd2t ? 0 : 4;
    mRunStandardTestWithError( wd && wd->logs().size() == explogsz,
	    BufferString("Has logs information: ", wellnm.buf()),
	    Well::MGR().errMsg().getFullString() )

    const Well::LogSet& logs = wd->logs();
    mRunStandardTest( logs.nrLoaded() == 0,
		      BufferString("Has no loaded logs: ", wellnm.buf()) )

    const BufferStringSet lognms( "Sonic", "P-Impedance" );
    const Well::LoadReqs twologsreqs( lognms );
    RefMan<Well::Data> keepwd;
    if ( onlytrackd2t )
	keepwd = wd.ptr();

    wd = Well::MGR().get( wid, twologsreqs );
    if ( onlytrackd2t )
    {
	const uiString errmsg = Well::MGR().errMsg();
	mRunStandardTest( !wd && !errmsg.isEmpty(),
			  "Required missing logs should not be found" )
	wd = keepwd.ptr();
	keepwd = nullptr;
    }
    else
    {
	mRunStandardTestWithError( wd && logs.isLoaded( "Sonic" ) &&
		       logs.isLoaded( "P-Impedance" ) &&
		       logs.nrLoaded() == lognms.size(),
		       BufferString("Has two loaded logs: ", wellnm.buf()),
		       Well::MGR().errMsg().getFullString() )
    }

    const Well::LoadReqs onelogreqs( StringView("Density") );
    if ( onlytrackd2t )
	keepwd = wd.ptr();

    wd = Well::MGR().get( wid, onelogreqs );
    if ( onlytrackd2t )
    {
	const uiString errmsg = Well::MGR().errMsg();
	mRunStandardTest( !wd && !errmsg.isEmpty(),
			  "Required missing log should not be found" )
	wd = keepwd.ptr();
	keepwd = nullptr;
    }
    else
    {
	mRunStandardTestWithError( wd && logs.isLoaded( "Density" ) &&
			logs.isLoaded( "Sonic" ) &&
			logs.isLoaded( "P-Impedance" ) &&
			logs.nrLoaded() == lognms.size()+1,
			BufferString("Has three loaded logs: ", wellnm.buf()),
			Well::MGR().errMsg().getFullString() )
    }

    if ( onlytrackd2t )
	keepwd = wd.ptr();

    wd = Well::MGR().get( wid, logsreqs_ );
    mRunStandardTest( logs.nrLoaded() == explogsz,
		  BufferString("Has all possible logs loaded: ", wellnm.buf()) )
    if ( onlytrackd2t )
    {
	wd = keepwd.ptr();
	keepwd = nullptr;
    }

    wd = Well::MGR().get( wid, dispprop2dreqs_ );
    mRunStandardTestWithError( wd && wd->displayProperties(true).isValid(),
	       BufferString("Has 2D display properties loaded: ", wellnm.buf()),
	       Well::MGR().errMsg().getFullString() )

    wd = Well::MGR().get( wid, dispprop3dreqs_ );
    mRunStandardTestWithError( wd && wd->displayProperties(false).isValid(),
	       BufferString("Has 3D display properties loaded: ", wellnm.buf()),
	       Well::MGR().errMsg().getFullString() )

    return true;
}


static bool testInfoCollector( od_ostream& strm )
{
    return true;
}


static bool testMultiWellReader( od_ostream& strm )
{
    TypeSet<MultiID> wellids;
    wellids += wellAmid_;
    wellids += wellBmid_;
    wellids += wellCmid_;
    wellids += wellDmid_;

    Well::LoadReqs lreqs = Well::LoadReqs::AllNoLogs();
    Well::LoadReqs nodatalreqs = lreqs;
    nodatalreqs.exclude( Well::CSMdl ).exclude( Well::Mrkrs ).excludeLogSel();

    RefObjectSet<Well::Data> wds;
    MultiWellReader rdr( wellids, wds, lreqs );
    TextTaskRunner taskrunner( strm );
    bool execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres, "Multi-Well reader in current survey",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.size() == wellids.size(),
		      "Well::Data set size (4)" )
    mRunStandardTest( wds.get(0)->loadState() == lreqs &&
		      wds.get(1)->loadState() == lreqs &&
		      wds.get(2)->loadState() == nodatalreqs &&
		      wds.get(3)->loadState() == nodatalreqs,
		      "Load state for the four wells" )
    mRunStandardTest( wds.get(0)->logs().size() +
		      wds.get(1)->logs().size() +
		      wds.get(2)->logs().size() +
		      wds.get(3)->logs().size() == 8,
		      "Nr log infos == 8" )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 0,
		      "Nr loaded logs == 0" )

    execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres, "Multi-Well reader with non-empty set",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.size() == wellids.size(),
		      "Well::Data set size (4)" )
    mRunStandardTest( wds.get(0)->loadState() == lreqs &&
		      wds.get(1)->loadState() == lreqs &&
		      wds.get(2)->loadState() == nodatalreqs &&
		      wds.get(3)->loadState() == nodatalreqs,
		      "Load state for the four wells" )
    mRunStandardTest( wds.get(0)->logs().size() +
		      wds.get(1)->logs().size() +
		      wds.get(2)->logs().size() +
		      wds.get(3)->logs().size() == 8,
		      "Nr log infos == 8" )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 0,
		      "Nr loaded logs == 0" )

    rdr.forceRead( true );
    execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres,
			       "Multi-Well reader with non-empty set (reload)",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.size() == wellids.size(),
		      "Well::Data set size (4)" )
    mRunStandardTest( wds.get(0)->loadState() == lreqs &&
		      wds.get(1)->loadState() == lreqs &&
		      wds.get(2)->loadState() == nodatalreqs &&
		      wds.get(3)->loadState() == nodatalreqs,
		      "Load state for the four wells" )
    mRunStandardTest( wds.get(0)->logs().size() +
		      wds.get(1)->logs().size() +
		      wds.get(2)->logs().size() +
		      wds.get(3)->logs().size() == 8,
		      "Nr log infos == 8" )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 0,
		      "Nr loaded logs == 0" )
    rdr.forceRead( false );

    lreqs.addLog( "P-Impedance" );
    rdr.setReqs( lreqs ).allowMissingLogs( false );
    execres = taskrunner.execute( rdr ) && rdr.hasFails() &&
	      rdr.details().isError();
    mRunStandardTest( execres, "Multi-Well reader should have failures" )
    mRunStandardTest( wds.get(0)->logs().size() +
		      wds.get(1)->logs().size() +
		      wds.get(2)->logs().size() +
		      wds.get(3)->logs().size() == 8,
		      "Nr log infos == 8" )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 2,
		      "Nr loaded logs == 2" )

    rdr.allowMissingLogs( true );
    execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres,
			       "Multi-Well reader with one log in 2/4 wells",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.get(0)->logs().size() +
		      wds.get(1)->logs().size() +
		      wds.get(2)->logs().size() +
		      wds.get(3)->logs().size() == 8,
		      "Nr log infos == 8" )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 2,
		      "Nr loaded logs == 2" )

    lreqs.addLog( "Sonic" ).include( Well::Logs );
    rdr.setReqs( lreqs );
    execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres,
			       "Multi-Well reader with two logs in 2/4 wells",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 4,
		      "Nr loaded logs == 4" )

    lreqs = Well::LoadReqs::All();
    lreqs.allowMissingLogs( true );
    rdr.setReqs( lreqs );
    execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres,
			       "Multi-Well reader with all logs in 2/4 wells",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.get(0)->logs().nrLoaded() +
		      wds.get(1)->logs().nrLoaded() +
		      wds.get(2)->logs().nrLoaded() +
		      wds.get(3)->logs().nrLoaded() == 8,
		      "Nr loaded logs == 8" )

    return true;
}


static bool testMultiWellReaderExt( const SurveyDiskLocation& sdl,
				    od_ostream& strm )
{
    DBKeySet wellkeys;
    wellkeys += DBKey( F034mid_, sdl );
    wellkeys += DBKey( wellAmid_, sdl );
    wellkeys += DBKey( F061mid_, sdl );

    const Well::LoadReqs lreqs = Well::LoadReqs::AllNoLogs();
    RefObjectSet<Well::Data> wds;
    MultiWellReader rdr( wellkeys, wds, lreqs );
    TextTaskRunner taskrunner( strm );
    bool execres = taskrunner.execute( rdr ) && !rdr.hasFails();
    mRunStandardTestWithError( execres, "Multi-Well reader in external survey",
			       rdr.allMessages().getText() )
    mRunStandardTest( wds.size() == wellkeys.size(),
		      "Well::Data set size (3)" )
    const Coord surfcrda( 2044803.0839895001, 19955993.667978998 );
    const Coord surfcrdb( 2033952.5919999999, 19972460.039999999 );
    const Coord surfcrdc( 1994432.415, 19938362.859999999 );
    mRunStandardTest( wds.get(0)->info().surfacecoord_ == surfcrda,
		      "Surface coordinate for well F03-4" )
    mRunStandardTest( wds.get(1)->info().surfacecoord_ == surfcrdb,
		      "Surface coordinate for well Well A" )
    mRunStandardTest( wds.get(2)->info().surfacecoord_ == surfcrdc,
		      "Surface coordinate for well F06-1" )

    return true;
}


mLoad1Module("Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    if ( !loadOpendTectPlugins("ODHDF5") )
	return false;

    inforeqs_ = Well::LoadReqs( Well::Inf );
    trackreqs_ = Well::LoadReqs( Well::Trck );
    d2treqs_ = Well::LoadReqs( Well::D2T );
    csmdlreqs_ = Well::LoadReqs( Well::CSMdl );
    markersreqs_ = Well::LoadReqs( Well::Mrkrs );
    loginfosreqs_ = Well::LoadReqs(  Well::LogInfos );
    logsreqs_ = Well::LoadReqs( Well::Logs );
    dispprop2dreqs_ = Well::LoadReqs( Well::DispProps2D );
    dispprop3dreqs_ = Well::LoadReqs( Well::DispProps3D );

    if ( !testLoadReqs() )
	return false;

    mRunStandardTest( pars().get("F03-4",F034mid_), "MultiID for F03-4" )
    mRunStandardTest( pars().get("F06-1",F061mid_), "MultiID for F06-1" )
    mRunStandardTest( pars().get("Well A",wellAmid_), "MultiID for Well A" )
    mRunStandardTest( pars().get("Well B",wellBmid_), "MultiID for Well B" )
    mRunStandardTest( pars().get("Well C",wellCmid_), "MultiID for Well C" )
    mRunStandardTest( pars().get("Well D",wellDmid_), "MultiID for Well D" )

    if ( !testIOAttribs() || !getAllWD() || !getNothing() )
	return false;

    ConstRefMan<Well::Data> wda = Well::MGR().get( wellAmid_ );
    mRunStandardTestWithError( wda && wda->loadState() == Well::LoadReqs::All(),
			       "Well A is fully loaded",
			       Well::MGR().errMsg().getFullString() )
    ConstRefMan<Well::Data> wdb = Well::MGR().get( wellBmid_ );
    mRunStandardTestWithError( wdb && wdb->loadState() == Well::LoadReqs::All(),
			       "Well B is fully loaded",
			       Well::MGR().errMsg().getFullString() )
    if ( !getSomeWD() )
	return false;

    ConstRefMan<Well::Data> wdc = Well::MGR().get( wellCmid_ );
    mRunStandardTestWithError( wdc && wdc->loadState().includes(inforeqs_) &&
			       wdc->loadState().includes(trackreqs_) &&
			       wdc->loadState().includes(d2treqs_) &&
			       wdc->loadState().includes(dispprop2dreqs_) &&
			       wdc->loadState().includes(dispprop3dreqs_),
			       "Well C is fully loaded",
			       Well::MGR().errMsg().getFullString() )
    ConstRefMan<Well::Data> wdd = Well::MGR().get( wellDmid_ );
    mRunStandardTestWithError( wdd && wdd->loadState().includes(inforeqs_) &&
			       wdd->loadState().includes(trackreqs_) &&
			       wdd->loadState().includes(d2treqs_) &&
			       wdd->loadState().includes(dispprop2dreqs_) &&
			       wdd->loadState().includes(dispprop3dreqs_),
			       "Well D is fully loaded",
			       Well::MGR().errMsg().getFullString() )

    wda = nullptr;
    wdb = nullptr;
    wdc = nullptr;
    wdd = nullptr;
    mRunStandardTest( isWellMGREmpty(), "Well manager is empty again" )

    if ( !getWD(wellAmid_,false) ||
	 !getWD(wellBmid_,false) ||
	 !getWD(wellCmid_,true) ||
	 !getWD(wellDmid_,true) )
	return false;

    const SurveyDiskLocation timesdl( "F3_Test_Survey_XYinft" );
    const SurveyDiskLocation depthmsdl( "F3_Test_Survey_DepthM_XYinft" );
    const SurveyDiskLocation depthftsdl( "F3_Test_Survey_DepthFT__XYinft_" );
    mRunStandardTest( timesdl.exists() &&
		      depthmsdl.exists() && depthftsdl.exists(),
		      "All target surveys exist" )

    if ( !testInfoCollector(strm) ||
	 !testMultiWellReader(strm) ||
	 !testMultiWellReaderExt(timesdl,strm) ||
	 !testMultiWellReaderExt(depthmsdl,strm) ||
	 !testMultiWellReaderExt(depthftsdl,strm) )
	return false;

    return true;
}
