/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.2 2000-01-24 16:46:05 bert Exp $";

#include "seisstor.h"
#include "seistrctr.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"


SeisStorage::SeisStorage( const IOObj* ioob )
	: conn(0)
	, trl(0)
	, ioobj(0)
	, errmsg(0)
{
    open( ioob );
}


void SeisStorage::open( const IOObj* ioob )
{
    delete ioobj; ioobj = 0;
    delete trl; trl = 0;
    if ( !ioob ) return;
    ioobj = ioob->cloneStandAlone();
    trl = (SeisTrcTranslator*)ioobj->getTranslator();
    if ( !trl )
	{ delete ioobj; ioobj = 0; }
}


SeisStorage::~SeisStorage()
{
    delete conn;
    delete trl;
    delete ioobj;
}


void SeisStorage::close()
{
    delete conn; conn = 0;
    init();
}


const StorageLayout& SeisStorage::storageLayout() const
{
    static StorageLayout dum_slo;
    return trl ? trl->storageLayout() : dum_slo;
}


void SeisStorage::fillPar( IOPar& iopar ) const
{
    if ( ioobj ) iopar.set( "ID", ioobj->unitID() );
}


void SeisStorage::usePar( const IOPar& iopar )
{
    const char* res = iopar["ID"];
    if ( *res )
    {
	IOObj* ioob = IOM().get( res );
	if ( ioob && (!ioobj || ioobj->unitID() != ioob->unitID()) )
	{
	    close();
	    open( ioob );
	}
	delete ioob;
    }
    if ( trl ) trl->usePar( &iopar );
}
