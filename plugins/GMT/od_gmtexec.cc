/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: od_gmtexec.cc,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/

#include "batchprog.h"
#include "filepath.h"
#include "gmtpar.h"
#include "initstdgmt.h"
#include "keystrs.h"
#include "timefun.h"
#include "strmdata.h"
#include "strmprov.h"

bool BatchProgram::go( std::ostream& strm )
{
    initStdGMTClasses();
    finishmsg_ = "Map created successfully";
    const char* psfilenm = pars().find( sKey::FileName );
    if ( !psfilenm || !*psfilenm )
	mErrStrmRet("Output PS file missing")

    FilePath tmpfp( psfilenm );
    tmpfp.setExtension( "tmp" );
    for ( int idx=0; ; idx++ )
    {
	IOPar* iop = pars().subselect( idx );
	if ( !iop ) break;

	GMTPar* par = GMTPF().create( *iop );
	if ( !par || !par->execute(strm,psfilenm) )
	{
	    BufferString msg = "Failed to post ";
	    msg += iop->find( ODGMT::sKeyGroupName );
	    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
	    strm << msg << std::endl;
	    *sd.ostrm << "Failed" << std::endl;
	    sd.close();
	    finishmsg_ = "Failed to create map";
	    return false;
	}
    }

    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
    *sd.ostrm << "Finished" << std::endl;
    sd.close();
    return true;
}

