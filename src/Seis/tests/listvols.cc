/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "iodirentry.h"
#include "ioman.h"
#include "iodir.h"
#include "segydirecttr.h"
#include "seiscbvs.h"
#include "keystrs.h"
#include "separstr.h"
#include "bufstringset.h"

#include <iostream>

mExternC(Seis) void od_Seis_initStdClasses();


static int runIOM( const IOObjContext& ctxt,
			const BufferStringSet& allowedtransls )
{
    IOM().to( ctxt.getSelKey() );

    const ObjectSet<IOObj>& objs = IOM().dirPtr()->getObjs();
    int ret = 0;
    for (int idx=0; idx<objs.size(); idx++ )
    {
	const IOObj& ioobj = *objs[idx];
	const char* translnm = ioobj.translator();
	if ( !allowedtransls.isPresent( translnm ) )
	    continue;
	const char* res = ioobj.pars().find( sKey::Type() );
	if ( res && !strcmp(res,sKey::Steering()) )
	    continue;

	std::cout << ioobj.name() << " (" << translnm << ')' << std::endl;
	ret++;
    }
    return ret;
}


static int runEntryList( const IOObjContext& ctxt )
{
    IODirEntryList entrylist( IOM().dirPtr(), ctxt );
    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	const IODirEntry& de( *entrylist[idx] );
	std::cout << de.name() << " (" << de.ioobj->translator()
	    	  << ')' << std::endl;
    }
    return entrylist.size();
}

    
int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    od_Seis_initStdClasses();

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    ctxt.forread = true;
    BufferStringSet allowedtransls;
    allowedtransls.add( CBVSSeisTrcTranslator::translKey() );
    allowedtransls.add( SEGYDirectSeisTrcTranslator::translKey() );
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );

    std::cout << "Via IOM():" << std::endl;
    const int nritemsiom = runIOM( ctxt, allowedtransls );
    std::cout << std::endl;

    FileMultiString fms( allowedtransls.get(0) );
    fms += allowedtransls.get(1);
    ctxt.toselect.allowtransls_ = fms;
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );

    std::cout << "Via EntryList:" << std::endl;
    const int nritemsel = runEntryList( ctxt );
    std::cout << std::endl;
    if ( nritemsiom != nritemsel )
    {
	std::cout << "Error: nr items from IOM=" << nritemsiom << ",\n"
	          << "\t\tfrom EntryList=" << nritemsel << std::endl;
	ExitProgram( 1 );
    }

    ExitProgram( 0 );
}
