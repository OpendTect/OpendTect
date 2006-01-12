/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.22 2006-01-12 12:21:38 cvshelene Exp $";

#include "seisstor.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seis2dline.h"
#include "seispsioprov.h"
#include "seisbuf.h"
#include "iostrm.h"
#include "iopar.h"
#include "ioman.h"
#include "iodir.h"
#include "strmprov.h"
#include "keystrs.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";


SeisStoreAccess::SeisStoreAccess( const IOObj* ioob )
	: ioobj(0)
	, trl(0)
	, lset(0)
	, seldata(0)
	, selcomp(-1)
	, is2d(false)
	, psioprov(0)
{
    setIOObj( ioob );
}


SeisStoreAccess::SeisStoreAccess( const char* fnm, bool isps )
	: ioobj(0)
	, trl(0)
	, lset(0)
	, seldata(0)
	, selcomp(-1)
	, is2d(false)
	, psioprov(0)
{
    IOStream iostrm( "_tmp_SeisStoreAccess", getStringFromInt(0,IOObj::tmpID) );
    iostrm.setGroup( isps ? mTranslGroupName(SeisPS)
	    		  : mTranslGroupName(SeisTrc) );
    iostrm.setTranslator( "CBVS" );
    iostrm.setFileName( fnm && *fnm ? fnm : StreamProvider::sStdIO );
    setIOObj( &iostrm );
}


SeisStoreAccess::~SeisStoreAccess()
{
    cleanUp( true );
}


SeisTrcTranslator* SeisStoreAccess::strl() const
{
    Translator* nctrl = const_cast<Translator*>( trl );
    mDynamicCastGet(SeisTrcTranslator*,ret,nctrl)
    return ret;
}


void SeisStoreAccess::setIOObj( const IOObj* ioob )
{
    close();
    if ( !ioob ) return;
    ioobj = ioob->clone();
    is2d = SeisTrcTranslator::is2D( *ioobj, true );

    trl = ioobj->getTranslator();
    if ( is2d )
    {
	lset = new Seis2DLineSet( ioobj->fullUserExpr(true) );
	if ( ioobj->name() != "" )
	    lset->setName( ioobj->name() );
    }
    else if ( !strcmp(ioobj->group(),mTranslGroupName(SeisPS)) )
	psioprov = SPSIOPF().provider( ioobj->translator() );
    else
    {
	if ( !trl )
	    { delete ioobj; ioobj = 0; }
	else if ( strl() )
	    strl()->setSelData( seldata );
    }
}


const Conn* SeisStoreAccess::curConn3D() const
{ return !is2d && strl() ? strl()->curConn() : 0; }
Conn* SeisStoreAccess::curConn3D()
{ return !is2d && strl() ? strl()->curConn() : 0; }


void SeisStoreAccess::setSelData( SeisSelData* tsel )
{
    delete seldata; seldata = tsel;
    if ( strl() ) strl()->setSelData( seldata );
}


bool SeisStoreAccess::cleanUp( bool alsoioobj )
{
    bool ret;
    if ( strl() )
	{ ret = strl()->close(); if ( !ret ) errmsg = strl()->errMsg(); }
    delete trl; trl = 0;
    delete lset; lset = 0;
    psioprov = 0;
    nrtrcs = 0;

    if ( alsoioobj )
    {
	delete ioobj; ioobj = 0;
	delete seldata; seldata = 0;
    }
    init();

    return ret;
}


bool SeisStoreAccess::close()
{
    return cleanUp( false );
}


void SeisStoreAccess::fillPar( IOPar& iopar ) const
{
    if ( ioobj ) iopar.set( "ID", ioobj->key() );
}


void SeisStoreAccess::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    BufferString tmp;
    if ( !res )
    {
	res = iopar.find( sKey::Name );
	if ( res && *res )
	{
	    IOM().to( SeisTrcTranslatorGroup::ioContext().stdSelKey() );
	    const IOObj* tryioobj = (*IOM().dirPtr())[ res ];
	    if ( !tryioobj )
		res = 0;
	    else
	    {
		tmp = tryioobj->key();
		res = tmp.buf();
	    }
	}
    }

    if ( res && *res )
    {
	IOObj* ioob = IOM().get( res );
	if ( ioob && (!ioobj || ioobj->key() != ioob->key()) )
	    setIOObj( ioob );
	delete ioob;
    }

    if ( !seldata ) seldata = new SeisSelData;
    if ( !seldata->usePar(iopar) )
	{ delete seldata; seldata = 0; }

    if ( strl() )
    {
	strl()->setSelData( seldata );
	strl()->usePar( iopar );
    }

    iopar.get( "Selected component", selcomp );
}
