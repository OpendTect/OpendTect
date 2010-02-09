/*+
________________________________________________________________________

 (C) dGB Beheer B.V.
 Author:	Raman K Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_gmtexec.cc,v 1.9 2010-02-09 06:30:37 cvsraman Exp $";

#include "batchprog.h"
#include "filepath.h"
#include "gmtpar.h"
#include "initearthmodel.h"
#include "initgeneral.h"
#include "initgmt.h"
#include "keystrs.h"
#include "oddirs.h"
#include "timefun.h"
#include "strmdata.h"
#include "strmprov.h"

#include <iostream>

bool BatchProgram::go( std::ostream& strm )
{
    General::initStdClasses();
    EarthModel::initStdClasses();
    GMT::initStdClasses();
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

	PtrMan<GMTPar> par = GMTPF().create( *iop );
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

    FilePath gmtcommandsfnm( GetBinPlfDir() );
    gmtcommandsfnm.add( ".gmtcommands4" );
    StreamProvider( gmtcommandsfnm.fullPath() ).remove();
    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
    *sd.ostrm << "Finished" << std::endl;
    sd.close();
    return true;
}

