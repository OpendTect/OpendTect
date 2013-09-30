/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "emhorizon2d.h"
#include "gridcreator.h"
#include "hor2dfrom3dcreator.h"
#include "iopar.h"
#include "ptrman.h"
#include "moddepmgr.h"

#include <iostream>


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "EarthModel" );

    PtrMan<IOPar> seispar = pars().subselect( "Seis" );
    if ( !seispar )
	{ strm << "Incomplete parameter file" << od_endl; return false; }

    bool res = true;
    TextTaskRunner tr( strm );
    strm.add( "Creating 2D Grid ...\n" ).flush();
    Seis2DGridCreator* seiscr = new Seis2DGridCreator( *seispar );
    res = tr.execute( *seiscr );
    if ( !res )
	{ strm << "  failed.\nProcess stopped" << od_endl; return false; }

    delete seiscr;

    PtrMan<IOPar> horpar = pars().subselect( "Horizon" );
    if ( !horpar )
	return res;

    strm << "\n\nCreating 2D Horizon(s) ..." << od_endl;
    Horizon2DGridCreator horcr;
    horcr.init( *horpar, &tr );
    res = tr.execute( horcr );
    if ( !res ) 
	{ strm << "  failed.\nProcess stopped\n" << od_endl; return false; }

    res = horcr.finish( &tr );
    return res;
}
