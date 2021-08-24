/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman K Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "batchprog.h"

#include "file.h"
#include "filepath.h"
#include "gmtpar.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "oddirs.h"
#include "moddepmgr.h"
#include "od_ostream.h"

#define mErrFatalRet(msg) \
{ \
    strm << msg << od_newline; \
    od_ostream tmpstrm( tmpfp.fullPath() ); \
    tmpstrm << "Failed" << od_newline; \
    tmpstrm << "Failed to create map"; \
    return false; \
}


mLoad2Modules("MPEEngine","Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    const char* psfilenm = pars().find( sKey::FileName() );
    const BufferString workdir( GetProcFileName(nullptr) );
    if ( workdir.size() > 255 )
	mErrStrmRet("Error: Current working directory path length too big")

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

	PtrMan<GMTPar> par = GMTPF().create( *iop, workdir );
	if ( idx == 0 )
	{
	    FixedString bmres( par ? par->find(ODGMT::sKeyGroupName()) : 0 );
	    if ( bmres.isEmpty() )
		mErrFatalRet("Basemap parameters missing")
	}

	if ( !par->execute(strm,psfilenm) )
	{
	    BufferString msg = "Failed to post ";
	    msg += iop->find( ODGMT::sKeyGroupName() );
	    strm << msg << od_newline;
	    if ( idx )
		continue;
	    else
		mErrFatalRet("Please check your GMT installation")
	}

	IOPar legpar;
	if ( par->fillLegendPar( legpar ) )
	{
	    legendspar.mergeComp( legpar, ::toString(legendidx++) );
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

    PtrMan<GMTPar> par = GMTPF().create( legendspar, workdir );
    if ( !par || !par->execute(strm, psfilenm) )
	strm << "Failed to post legends";
    par = nullptr;

    strm << "Map created successfully" << od_endl;

    od_ostream sd( tmpfp.fullPath() );
    sd << "Finished." << od_endl;
    sd.close();

    return true;
}
