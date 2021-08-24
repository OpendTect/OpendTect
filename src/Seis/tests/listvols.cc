/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/


#include "testprog.h"

#include "iodirentry.h"
#include "ioman.h"
#include "iodir.h"
#include "segydirecttr.h"
#include "seiscbvs.h"
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

	od_cout() << ioobj.name() << " (" << translnm << ')' << od_endl;
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
	od_cout() << de.name() << " (" << de.ioobj->translator()
		  << ')' << od_endl;
    }
    return entrylist.size();
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    od_Seis_initStdClasses();

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    ctxt.forread = true;
    BufferStringSet allowedtransls;
    allowedtransls.add( CBVSSeisTrcTranslator::translKey() );
    allowedtransls.add( SEGYDirectSeisTrcTranslator::translKey() );
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );

    od_cout() << "Via IOM():" << od_endl;
    const int nritemsiom = runIOM( ctxt, allowedtransls );
    od_cout() << od_endl;

    FileMultiString fms( allowedtransls.get(0) );
    fms += allowedtransls.get(1);
    ctxt.toselect.allowtransls_ = fms;
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );

    od_cout() << "Via EntryList:" << od_endl;
    const int nritemsel = runEntryList( ctxt );
    od_cout() << od_endl;
    if ( nritemsiom != nritemsel )
    {
	od_cout() << "Error: nr items from IOM=" << nritemsiom << ",\n"
	          << "\t\tfrom EntryList=" << nritemsel << od_endl;
	return 1;
    }

    return 0;
}
