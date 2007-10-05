/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.26 2007-10-05 11:11:09 cvsbert Exp $";

#include "seisseqio.h"
#include "seisread.h"
#include "seiswrite.h"
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


static BufferStringSet& seisInpClassNames()
{
    static BufferStringSet* nms = 0;
    if ( !nms ) nms = new BufferStringSet;
    return *nms;
}
static ObjectSet<Seis::SeqInp>& seisInpClasses()
{
    static ObjectSet<Seis::SeqInp>* clsss = 0;
    if ( !clsss ) clsss = new ObjectSet<Seis::SeqInp>;
    return *clsss;
}
BufferStringSet& Seis::SeqInp::classNames()
{ return seisInpClassNames(); }
Seis::SeqInp* Seis::SeqInp::make( const char* nm )
{
    const ObjectSet<Seis::SeqInp>& clsss = seisInpClasses();
    if ( !nm ) nm = "";
    for ( int idx=0; idx<clsss.size(); idx++ )
    {
	if ( !strcmp(nm,clsss[idx]->type()) )
	    return (Seis::SeqInp*)clsss[idx]->makeNew();
    }
    return 0;
}
void Seis::SeqInp::addClass( Seis::SeqInp* si )
{ seisInpClasses() += si; }


static BufferStringSet& seisOutClassNames()
{
    static BufferStringSet* nms = 0;
    if ( !nms ) nms = new BufferStringSet;
    return *nms;
}
static ObjectSet<Seis::SeqOut>& seisOutClasses()
{
    static ObjectSet<Seis::SeqOut>* clsss = 0;
    if ( !clsss ) clsss = new ObjectSet<Seis::SeqOut>;
    return *clsss;
}
BufferStringSet& Seis::SeqOut::classNames()
{ return seisOutClassNames(); }
Seis::SeqOut* Seis::SeqOut::make( const char* nm )
{
    const ObjectSet<Seis::SeqOut>& clsss = seisOutClasses();
    if ( !nm ) nm = "";
    for ( int idx=0; idx<clsss.size(); idx++ )
    {
	if ( !strcmp(nm,clsss[idx]->type()) )
	    return (Seis::SeqOut*)clsss[idx]->makeNew();
    }
    return 0;
}
void Seis::SeqOut::addClass( Seis::SeqOut* so )
{ seisOutClasses() += so; }


const SeisSelData& Seis::SeqInp::selData() const
{
    static SeisSelData emptyseldata;
    return emptyseldata;
}


Seis::ODSeqInp::~ODSeqInp()
{
    delete rdr_;
}


const SeisSelData& Seis::ODSeqInp::selData() const
{
    static SeisSelData emptyseldata;
    return rdr_ && rdr_->selData() ? *rdr_->selData() : Seis::SeqInp::selData();
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
    pErrMsg("Not impl");
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


Seis::ODSeqOut::~ODSeqOut()
{
    delete wrr_;
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
    pErrMsg("Not impl");
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

