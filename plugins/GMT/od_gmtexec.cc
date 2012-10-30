/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman K Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"
#include "file.h"
#include "filepath.h"
#include "gmtpar.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "oddirs.h"
#include "timefun.h"
#include "strmdata.h"
#include "strmprov.h"
#include "moddepmgr.h"

#include <iostream>

#define mErrFatalRet(msg) \
{ \
    strm << msg << std::endl; \
    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream(); \
    outputfp.setFileName( ".gmtcommands4" ); \
    if ( File::exists(outputfp.fullPath()) ) \
	StreamProvider( outputfp.fullPath() ).remove(); \
	\
    File::changeDir( cwd.buf() ); \
    *sd.ostrm << "Failed" << std::endl; \
    sd.close(); \
    finishmsg_ = "Failed to create map"; \
    return false; \
}


bool BatchProgram::go( std::ostream& strm )
{
    OD::ModDeps().ensureLoaded( "EarthModel" );
    GMT::initStdClasses();
    finishmsg_ = "Map created successfully";
    const char* psfilenm = pars().find( sKey::FileName() );
    FilePath outputfp( psfilenm );
    const BufferString cwd = File::getCurrentPath();
    if ( cwd.size() > 255 )
	mErrStrmRet("Error: Current working directory path length too big")

    File::changeDir( outputfp.pathOnly() );
    if ( !psfilenm || !*psfilenm )
	mErrStrmRet("Output PS file missing")

    FilePath tmpfp( psfilenm );
    tmpfp.setExtension( "tmp" );
    IOPar legendspar;
    int legendidx = 0;
    legendspar.set( ODGMT::sKeyGroupName(), "Legend" );
    for ( int idx=0; ; idx++ )
    {
	IOPar* iop = pars().subselect( idx );
	if ( !iop ) break;

	PtrMan<GMTPar> par = GMTPF().create( *iop );
	if ( !idx && ( !par || par->find(ODGMT::sKeyGroupName()) != "Basemap" ) )
	    mErrFatalRet("Basemap parameters missing")

	if ( !par->execute(strm,psfilenm) )
	{
	    BufferString msg = "Failed to post ";
	    msg += iop->find( ODGMT::sKeyGroupName() );
	    strm << msg << std::endl;
	    if ( idx )
		continue;
	    else
		mErrFatalRet("Please check your GMT installation")
	}

	IOPar legpar;
	if ( par->fillLegendPar( legpar ) )
	{
	    legendspar.mergeComp( legpar, toString(legendidx++) );
	}

	if ( idx == 0 )
	{
	    Interval<int> xrg, yrg;
	    Interval<float> mapdim;
	    par->get( ODGMT::sKeyXRange(), xrg );
	    par->get( ODGMT::sKeyYRange(), yrg );
	    par->get( ODGMT::sKeyMapDim(), mapdim );
	    legendspar.set( ODGMT::sKeyMapDim(), mapdim );
	    legendspar.set( ODGMT::sKeyXRange(), xrg );
	    legendspar.set( ODGMT::sKeyYRange(), yrg );
	}
    }

    PtrMan<GMTPar> par = GMTPF().create( legendspar );
    if ( !par || !par->execute(strm, psfilenm) )
	strm << "Failed to post legends";

    outputfp.setFileName( ".gmtcommands4" );
    StreamProvider( outputfp.fullPath() ).remove();
    File::changeDir( cwd.buf() );
    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
    *sd.ostrm << "Finished" << std::endl;
    sd.close();
    return true;
}
