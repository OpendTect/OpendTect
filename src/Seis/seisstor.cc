/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.32 2007-11-23 11:59:06 cvsbert Exp $";

#include "seisseqio.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisbounds.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seispsioprov.h"
#include "seisselectionimpl.h"
#include "seisbuf.h"
#include "iostrm.h"
#include "iopar.h"
#include "ioman.h"
#include "iodir.h"
#include "strmprov.h"
#include "keystrs.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";
const char* Seis::SeqIO::sKeyODType = "OpendTect";


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
    IOStream iostrm( "_tmp_SeisStoreAccess", getStringFromInt(IOObj::tmpID) );
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
	if ( !ioobj->name().isEmpty() )
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


void SeisStoreAccess::setSelData( Seis::SelData* tsel )
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
	    IOM().to( SeisTrcTranslatorGroup::ioContext().getSelKey() );
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

    if ( !seldata )
	seldata = Seis::SelData::get( iopar );
    if ( seldata->isAll() )
	{ delete seldata; seldata = 0; }

    if ( strl() )
    {
	strl()->setSelData( seldata );
	strl()->usePar( iopar );
    }

    iopar.get( "Selected component", selcomp );
}

//--- SeqIO

void Seis::SeqIO::fillPar( IOPar& iop ) const
{
    Seis::putInPar( geomType(), iop );
}

mImplFactory( Seis::SeqInp, Seis::SeqInp::factory );
mImplFactory( Seis::SeqOut, Seis::SeqOut::factory );

Seis::SelData& Seis::SeqInp::emptySelData() const
{
    static Seis::RangeSelData sd(false);
    sd.setIsAll( true );
    return sd;
}


void Seis::SeqInp::fillPar( IOPar& iop ) const
{
    Seis::SeqIO::fillPar( iop );
    selData().fillPar( iop );
}


Seis::ODSeqInp::~ODSeqInp()
{
    delete rdr_;
}


Seis::GeomType Seis::ODSeqInp::geomType() const
{
    return rdr_ ? rdr_->geomType() : Seis::Vol;
}


void Seis::ODSeqInp::initClass()
{
    Seis::SeqInp::factory().addCreator( create, sKeyODType );
}


const Seis::SelData& Seis::ODSeqInp::selData() const
{
    return rdr_ && rdr_->selData() ? *rdr_->selData() : emptySelData();
}


void Seis::ODSeqInp::setSelData( const Seis::SelData& sd )
{
    if ( rdr_ )
	rdr_->setSelData( sd.clone() );
}


bool Seis::ODSeqInp::usePar( const IOPar& iop )
{
    if ( rdr_ ) delete rdr_;
    rdr_ = new SeisTrcReader;
    rdr_->usePar( iop );
    if ( rdr_->errMsg() && *rdr_->errMsg() )
    {
	errmsg_ = rdr_->errMsg();
	delete rdr_; rdr_ = 0;
    }
    return rdr_;
}


void Seis::ODSeqInp::fillPar( IOPar& iop ) const
{
    Seis::SeqInp::fillPar( iop );
    if ( rdr_ ) rdr_->fillPar( iop );
}


bool Seis::ODSeqInp::get( SeisTrc& trc ) const
{
    if ( !rdr_ ) return false;

    if ( !rdr_->get(trc) )
    {
	errmsg_ = rdr_->errMsg();
	return false;
    }
    return true;
}


Seis::Bounds* Seis::ODSeqInp::getBounds() const
{
    return rdr_ ? rdr_->getBounds() : 0;
}


Seis::ODSeqOut::~ODSeqOut()
{
    delete wrr_;
}


Seis::GeomType Seis::ODSeqOut::geomType() const
{
    return wrr_ ? wrr_->geomType() : Seis::Vol;
}


bool Seis::ODSeqOut::usePar( const IOPar& iop )
{
    if ( wrr_ ) delete wrr_;
    IOObj* i = 0; wrr_ = new SeisTrcWriter( i );
    wrr_->usePar( iop );
    if ( wrr_->errMsg() && *wrr_->errMsg() )
    {
	errmsg_ = wrr_->errMsg();
	delete wrr_; wrr_ = 0;
    }
    return wrr_;
}


void Seis::ODSeqOut::fillPar( IOPar& iop ) const
{
    Seis::SeqOut::fillPar( iop );
    if ( wrr_ ) wrr_->fillPar( iop );
}


bool Seis::ODSeqOut::put( const SeisTrc& trc )
{
    if ( !wrr_ ) return false;

    if ( !wrr_->put(trc) )
    {
	errmsg_ = wrr_->errMsg();
	return false;
    }
    return true;
}

