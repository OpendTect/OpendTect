/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.11 2004-08-25 12:27:06 bert Exp $";

#include "seisstor.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";


SeisStoreAccess::SeisStoreAccess( const IOObj* ioob )
	: ioobj(0)
	, trl(0)
	, lgrp(0)
	, seldata(0)
	, selcomp(-1)
	, is2d(false)
{
    setIOObj( ioob );
}


SeisStoreAccess::~SeisStoreAccess()
{
    cleanUp( true );
}


void SeisStoreAccess::setIOObj( const IOObj* ioob )
{
    close();
    if ( !ioob ) return;
    ioobj = ioob->clone();
    is2d = SeisTrcTranslator::is2D( *ioobj );

    if ( is2d )
	lgrp = new Seis2DLineGroup( ioobj->fullUserExpr(true) );
    else
    {
	trl = (SeisTrcTranslator*)ioobj->getTranslator();
	if ( !trl )
	    { delete ioobj; ioobj = 0; }
	else
	    trl->setSelData( seldata );
    }
}


const Conn* SeisStoreAccess::curConn() const
{ return trl ? trl->curConn() : 0; }
Conn* SeisStoreAccess::curConn()
{ return trl ? trl->curConn() : 0; }


void SeisStoreAccess::setSelData( SeisSelData* tsel )
{
    delete seldata; seldata = tsel;
    if ( trl ) trl->setSelData( seldata );
}


void SeisStoreAccess::cleanUp( bool alsoioobj )
{
    delete trl; trl = 0;
    delete lgrp; lgrp = 0;
    nrtrcs = 0;
    if ( alsoioobj )
    {
	delete ioobj; ioobj = 0;
	delete seldata; seldata = 0;
    }
    init();
}


void SeisStoreAccess::close()
{
    cleanUp( false );
}


void SeisStoreAccess::fillPar( IOPar& iopar ) const
{
    if ( ioobj ) iopar.set( "ID", ioobj->key() );
}


void SeisStoreAccess::usePar( const IOPar& iopar )
{
    const char* res = iopar["ID"];
    if ( *res )
    {
	IOObj* ioob = IOM().get( res );
	if ( ioob && (!ioobj || ioobj->key() != ioob->key()) )
	    setIOObj( ioob );
	delete ioob;
    }

    if ( !seldata ) seldata = new SeisSelData;
    if ( !seldata->usePar(iopar) )
	{ delete seldata; seldata = 0; }

    if ( trl )
    {
	trl->setSelData( seldata );
	trl->usePar( iopar );
    }

    iopar.get( "Selected component", selcomp );
}
