/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: od_gmtexec.cc,v 1.1 2008-08-01 08:28:27 cvsraman Exp $
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
	    return false;
	}
    }

    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
    *sd.ostrm << "Finished" << std::endl;
    sd.close();
    return true;
}

