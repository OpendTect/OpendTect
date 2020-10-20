/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
________________________________________________________________________

-*/

#include "batchprog.h"

#include "emhorizon2d.h"
#include "gridcreator.h"
#include "hor2dfrom3dcreator.h"
#include "iopar.h"
#include "ptrman.h"
#include "moddepmgr.h"


mLoad2Modules("EarthModel","Seis")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<IOPar> seispar = pars().subselect( "Seis" );
    if ( !seispar )
	{ strm << "Incomplete parameter file" << od_endl; return false; }

    bool res = true;
    LoggedTaskRunnerProvider trprov( strm );
    strm.add( "Creating 2D Grid ...\n" ).flush();
    Seis2DGridCreator* seiscr = new Seis2DGridCreator( *seispar );
    BufferString warningmsg;
    if ( seiscr->hasWarning(warningmsg) )
	 strm << warningmsg.buf() << od_endl;
    res = trprov.execute( *seiscr );
    if ( !res )
	{ strm << "  failed.\nProcess stopped" << od_endl; return false; }

    delete seiscr;

    PtrMan<IOPar> horpar = pars().subselect( "Horizon" );
    if ( !horpar )
	return res;

    strm << "\n\nCreating 2D Horizon(s) ..." << od_endl;
    Horizon2DGridCreator horcr;
    horcr.init( *horpar, trprov );
    res = trprov.execute( horcr );
    if ( !res )
	{ strm << "  failed.\nProcess stopped\n" << od_endl; return false; }

    res = horcr.finish( trprov );
    return res;
}
