/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "createattriblog.h"
#include "createlogcube.h"
#include "file.h"
#include "filepath.h"
#include "mnemonics.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "oddirs.h"
#include "plugins.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"

#include <QDir>

#include <csignal>
#include <cstdlib>


BufferString surveydir_;
MultiID wellAmid_, wellBmid_, wellCmid_, wellDmid_;

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


void cleanup()
{
    if ( surveydir_.isEmpty() )
	return;

    const FilePath soncubefp( surveydir_, "Seismics", "Sonic_log_cube.cbvs" );
    if ( soncubefp.exists() )
	File::remove( soncubefp.fullPath() );

    const FilePath aicubefp( surveydir_,"Seismics","P-Impedance_log_cube.cbvs");
    if ( aicubefp.exists() )
	File::remove( aicubefp.fullPath() );

    const FilePath welllogfp( surveydir_, "WellInfo", "Well_A^5.wll" );
    if ( welllogfp.exists() )
	File::remove( welllogfp.fullPath() );

    if ( !QDir::setCurrent(QString::fromLocal8Bit(surveydir_.buf())) )
	return;

    const FilePath seisomf( surveydir_,"Seismics", ".omf*" );
    std::string command = "git checkout -- ";
    command += seisomf.fullPath().str();
    std::system( command.c_str() );

    const FilePath wellsomf( surveydir_, "WellInfo", "Well_*.*" );
    command = "git checkout -- ";
    command += wellsomf.fullPath().str();
    std::system( command.c_str() );
}


void signalHandler( int signum )
{
    errStream() << "Interrupt signal (" << signum << ") received." << od_endl;
    exit( signum );
}


static Well::ExtractParams getWellLogExtractPars( float zstep )
{
    Well::ExtractParams pars;
    pars.zselection_ = Well::ExtractParams::Markers;
    pars.setTopMarker( Well::ExtractParams::sKeyDataStart(), 0.f );
    pars.setBotMarker( Well::ExtractParams::sKeyDataEnd(), 0.f );
    pars.snapZRangeToSurvey( false );
    pars.zstep_ = zstep;
    pars.extractzintime_ = false;
    pars.samppol_ = Stats::UseAvg;

    return pars;
}


static bool testGetWellPosition( const MultiID& wellid, TrcKey& tk )
{
    const Well::LoadReqs lreqs( Well::Inf );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wellid, lreqs );
    mRunStandardTestWithError( wd, "Fetch well information",
				Well::MGR().errMsg().getFullString() )
    tk.setIs3D().setFrom( wd->info().surfacecoord_ );
    mRunStandardTest( tk.is3D() && tk.inl() == 646 && tk.crl() == 880,
		      "Well position as TrcKey" )

    return true;
}


// Log(s) of multiple wells to Seismic dataset(s)

static bool testLogCubeCreator( const MultiID& wellida, const MultiID& wellidb,
				od_ostream& strm )
{
    const Well::ExtractParams pars = getWellLogExtractPars( 1.f );

    const BufferStringSet lognms( "Sonic", "P-Impedance" );
    TypeSet<MultiID> wellids;
    wellids += wellida;
    wellids += wellidb;
    const int nrtrcs = SI().crlStep();

    TextTaskRunner trun( strm );
    LogCubeCreator cr( lognms, wellids, pars, nrtrcs );
    uiStringSet existimpls;
    const uiRetVal uirv = cr.setOutputNm( "log cube", false, existimpls );
    mRunStandardTestWithError( uirv.isOK(), "Set Log Cube names",
			       uirv.getText() )

    mRunStandardTestWithError( trun.execute( cr ), "Log cube creator execution",
			       cr.allMessages().getText() )

    return true;
}


static bool testLogCubeIntegrity( const TrcKey& tk )
{
    TrcKeySampling tks;
    tks.set( tk ).step_ = SI().sampling(false).hsamp_.step_;
    const Seis::RangeSelData sd( tks );
    const float zval1 = 0.924f;
    const float zval2 = 0.948f;
    const int icomp = 0;
    const ZSampling& sizrg = SI().sampling(false).zsamp_;
    const float zeps = 1e-5f;

    // Sonic
    MultiID soncubeid;
    mRunStandardTest( BP().pars().get("Sonic Log Cube",soncubeid),
		      "MultiID for Sonic Log Cube" )

    SeisTrcReader sonrdr( soncubeid, Seis::Vol );
    sonrdr.setSelData( sd.clone() );
    mRunStandardTestWithError( sonrdr.prepareWork() && sonrdr.isOK(),
			       "Prepare sonic SeisTrc reader",
			       sonrdr.errMsg().getFullString() )
    SeisTrc sontrc;
    mRunStandardTestWithError( sonrdr.get(sontrc.info()) == 1 &&
				sonrdr.get(sontrc),
				"Read sonic log cube trace",
				sonrdr.errMsg().getFullString() )

    mRunStandardTest( sontrc.info().trcKey() == tk, "SeisTrc position" )
    mRunStandardTest( sontrc.zRange().isEqual(sizrg,zeps), "SeisTrc zRange" )
    mRunStandardTest( mIsEqual(sontrc.getValue(zval1,icomp),417.629547f,1e-3f),
		      "Sonic log cube value check #1" )
    mRunStandardTest( mIsEqual(sontrc.getValue(zval2,icomp),407.632538f,1e-3f),
		      "Sonic log cube value check #2" )

    // P-Impedance
    MultiID aicubeid;
    mRunStandardTest( BP().pars().get("P-Impedance Log Cube",aicubeid),
		      "MultiID for P-Impedance Log Cube" )

    SeisTrcReader airdr( aicubeid, Seis::Vol );
    airdr.setSelData( sd.clone() );
    mRunStandardTestWithError( airdr.prepareWork() && airdr.isOK(),
			       "Prepare AI SeisTrc reader",
			       airdr.errMsg().getFullString() )

    SeisTrc aitrc;
    mRunStandardTestWithError( airdr.get(aitrc.info()) == 1 &&
			       airdr.get(aitrc),
			       "Read P-Impedance log cube trace",
			       airdr.errMsg().getFullString() )

    mRunStandardTest( aitrc.info().trcKey() == tk, "SeisTrc position" )
    mRunStandardTest( aitrc.zRange().isEqual(sizrg,zeps), "SeisTrc zRange" )
    mRunStandardTest( mIsEqual(aitrc.getValue(zval1,icomp),4995353.0f,1.f),
		      "P-Impedance log cube value check #1" )
    mRunStandardTest( mIsEqual(aitrc.getValue(zval2,icomp),5149821.5f,1.f),
		      "P-Impedance log cube value check #2" )

    return true;
}


// Seismic to Log

static bool testAttribLogCreator( const MultiID& wellida,const MultiID& wellidb,
				  BufferStringSet& lognms, od_ostream& strm )
{
    PtrMan<IOPar> attrpar = BP().pars().subselect( sKey::Attributes() );
    mRunStandardTest( attrpar, "Attribute entries in IOPar" )

    Attrib::DescSet ads( false );
    ads.usePar( *attrpar );
    mRunStandardTest( ads.size() >= 1, "Attribute set size" )

    ConstRefMan<Attrib::Desc> desc = ads.desc( 0 );
    mRunStandardTest( desc, "Attribute DescSet Entry for Seismic" )

    Attrib::SelSpec as;
    as.set( *desc );
    const char* lognm = as.userRef();

    const Well::ExtractParams pars = getWellLogExtractPars( 0.1524f );
    AttribLogCreator::Setup su( &ads, &pars );
    su.selspec( &as ).lognm( lognm );

    const Mnemonic* outmn = MNC().getByName( "SEIS" );
    mRunStandardTest( outmn, "Has output mnemonic" )

    TypeSet<MultiID> wellids;
    wellids += wellida;
    wellids += wellidb;

    lognms.setEmpty();
    lognms.add( lognm );
    Well::LoadReqs lreqs( Well::Inf, Well::Trck, Well::D2T );
    lreqs.include( Well::LogInfos );
    RefObjectSet<Well::Data> wds;
    for ( const auto& wid : wellids )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( wid, lreqs );
	mRunStandardTestWithError( wd, "Has well data",
				   Well::MGR().errMsg().getFullString() )
	if ( wd->logs().isPresent(lognm) )
	{
	    mRunStandardTestWithError(
		    Well::MGR().deleteLogs( wid, lognms ),
		    "Delete pre-existing log",
		    Well::MGR().errMsg().getFullString() )
	}

	wds.add( wd.getNonConstPtr() );
    }
    mRunStandardTest( wds.size() == 2, "Well data set size" )
    wds.setEmpty();

    TextTaskRunner trun( strm );
    bool overwrite = false;
    BulkAttribLogCreator cr1( su, wellids, *outmn, overwrite );
    mRunStandardTestWithError( trun.execute( cr1 ), "Attribute to Log Creator",
			       cr1.allMessages().getText() )

    overwrite = true;
    BulkAttribLogCreator cr2( su, wellids, *outmn, overwrite );
    mRunStandardTestWithError( trun.execute( cr2 ),
			       "Attribute to Log Creator (overwrite)",
			       cr2.allMessages().getText() )

    overwrite = false;
    BulkAttribLogCreator cr3( su, wellids, *outmn, overwrite );
    mRunStandardTest( !trun.execute( cr3 ) && cr3.details().isError(),
		      "Attribute to Log Creator (overwrite should fail)" )

    return true;
}


static bool testLogsIntegrity( const MultiID& wellida, const MultiID& wellidb,
			       const BufferStringSet& lognms )
{
    const Well::LoadReqs lreqs( lognms );
    ConstRefMan<Well::Data> wda = Well::MGR().get( wellida, lreqs );
    mRunStandardTestWithError( wda, "Well data A",
			       Well::MGR().errMsg().getFullString() )

    ConstRefMan<Well::Data> wdb = Well::MGR().get( wellidb, lreqs );
    mRunStandardTestWithError( wdb, "Well data B",
			       Well::MGR().errMsg().getFullString() )

    const Well::Log* loga = wda->logs().getLog( lognms.first()->buf() );
    const Well::Log* logb = wdb->logs().getLog( lognms.first()->buf() );
    mRunStandardTest( loga && logb, "Has logs" )
    const Mnemonic* outmn = MNC().getByName( "SEIS" );
    mRunStandardTest( outmn && loga->mnemonic() == outmn,
		      "Output mnemonic for Well A" )
    mRunStandardTest( outmn && logb->mnemonic() == outmn,
		      "Output mnemonic for Well B" )

    const Interval<float> mdrg( 441.655212f, 1060.55164f );
    mRunStandardTest( loga->dahRange().isEqual(mdrg,1e-2f),
		      "Output MD range for Well A" )
    mRunStandardTest( logb->dahRange().isEqual(mdrg,1e-2f),
		      "Output MD range for Well B" )

    const Interval<float> valrg( -18937.8652f, 14107.8291f );
    mRunStandardTest( loga->valueRange().isEqual(valrg,1e-1f),
		      "Output value range for Well A" )
    mRunStandardTest( logb->valueRange().isEqual(valrg,1e-1f),
		      "Output value range for Well B" )

    const int explogsz = 4062;
    mRunStandardTest( loga->size() == explogsz, "Seismic log size for Well A" )
    mRunStandardTest( logb->size() == explogsz, "Seismic log size for Well B" )

    mRunStandardTest( mIsEqual(loga->value(1036),5106.09375f,1e-2f),
		      "Log value at index for Well A" )
    mRunStandardTest( mIsEqual(logb->value(1036),5106.09375f,1e-2f),
		      "Log value at index for Well B" )
    wda = nullptr;
    wdb = nullptr;

    mRunStandardTestWithError( Well::MGR().deleteLogs(wellida,lognms),
			       "Delete created log for Well A",
			       Well::MGR().errMsg().getFullString() )
    mRunStandardTestWithError( Well::MGR().deleteLogs(wellidb,lognms),
			       "Delete created log for Well B",
			       Well::MGR().errMsg().getFullString() )

    return true;
}


// Log to Attribute

static bool testLogAttribute( const MultiID& wellid )
{
    //TODO
    return true;
}


mLoad1Module("WellAttrib")

bool BatchProgram::doWork( od_ostream& strm )
{
    if ( !loadOpendTectPlugins("ODHDF5") )
	return false;

    signal( SIGABRT, signalHandler );
    signal( SIGTERM, signalHandler );
#ifdef __lux64__
    signal( SIGKILL, signalHandler );
#endif
    atexit( cleanup );
    surveydir_ = SI().diskLocation().fullPath();

    mRunStandardTest( pars().get("Well A",wellAmid_), "MultiID for well A" )
    mRunStandardTest( pars().get("Well B",wellBmid_), "MultiID for well B" )

    TrcKey tk;
    BufferStringSet lognms;
    if ( !testGetWellPosition(wellAmid_,tk) ||
	 !testLogCubeCreator(wellAmid_,wellBmid_,strm) ||
	 !testLogCubeIntegrity(tk) ||
	 !testAttribLogCreator(wellAmid_,wellBmid_,lognms,strm) ||
	 !testLogsIntegrity(wellAmid_,wellBmid_,lognms) ||
	 !testLogAttribute(wellAmid_) ||
	 !testLogAttribute(wellBmid_) )
	return false;

    return true;
}
