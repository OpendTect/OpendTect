/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/


#include "seisseqio.h"
#include "seiscbvs.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisbounds.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seispsioprov.h"
#include "seisselectionimpl.h"
#include "seisbuf.h"
#include "iostrm.h"
#include "iopar.h"
#include "ioman.h"
#include "strmprov.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "keystrs.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";
const char* Seis::SeqIO::sKeyODType = "OpendTect";


SeisStoreAccess::SeisStoreAccess( const IOObj* ioob )
	: ioobj_(0)
	, trl_(0)
	, dataset_(0)
	, seldata_(0)
	, is2d_(false)
	, psioprov_(0)
{
    setIOObj( ioob );
}


SeisStoreAccess::SeisStoreAccess( const char* fnm, bool isps, bool is_2d )
	: ioobj_(0)
	, trl_(0)
	, dataset_(0)
	, seldata_(0)
	, is2d_(is_2d)
	, psioprov_(0)
{
    IOStream iostrm( "_tmp_SeisStoreAccess", toString(IOObj::tmpID()) );
    iostrm.setGroup( !isps ?
	   ( is2d_ ? mTranslGroupName(SeisTrc2D) : mTranslGroupName(SeisTrc) )
	 : ( is2d_ ? mTranslGroupName(SeisPS2D) : mTranslGroupName(SeisPS3D)) );
    iostrm.setTranslator( CBVSSeisTrcTranslator::translKey() );
    iostrm.fileSpec().setFileName( fnm && *fnm ? fnm
						: StreamProvider::sStdIO() );
    setIOObj( &iostrm );
}


SeisStoreAccess::~SeisStoreAccess()
{
    cleanUp( true );
}


SeisTrcTranslator* SeisStoreAccess::strl() const
{
    Translator* nctrl_ = const_cast<Translator*>( trl_ );
    mDynamicCastGet(SeisTrcTranslator*,ret,nctrl_)
    return ret;
}


void SeisStoreAccess::setIOObj( const IOObj* ioob )
{
    close();
    if ( !ioob ) return;
    ioobj_ = ioob->clone();
    is2d_ = SeisTrcTranslator::is2D( *ioobj_, true );
    const bool isps = SeisTrcTranslator::isPS( *ioobj_ );

    trl_ = ioobj_->createTranslator();
    if ( isps )
	psioprov_ = SPSIOPF().provider( ioobj_->translator() );
    else if ( is2d_ )
    {
	dataset_ = new Seis2DDataSet( *ioobj_ );
	if ( !ioobj_->name().isEmpty() )
	    dataset_->setName( ioobj_->name() );

	mDynamicCastGet(SeisTrc2DTranslator*,trtrl2d,trl_)
	if ( trtrl2d ) trtrl2d->setDataSet( *dataset_ );
    }
    else
    {
	if ( !trl_ )
	    { delete ioobj_; ioobj_ = 0; }
	else if ( strl() )
	    strl()->setSelData( seldata_ );
    }
}


const Conn* SeisStoreAccess::curConn3D() const
{ return !is2d_ && strl() ? strl()->curConn() : 0; }
Conn* SeisStoreAccess::curConn3D()
{ return !is2d_ && strl() ? strl()->curConn() : 0; }


void SeisStoreAccess::setSelData( Seis::SelData* tsel )
{
    delete seldata_; seldata_ = tsel;
    if ( strl() ) strl()->setSelData( seldata_ );
}


bool SeisStoreAccess::cleanUp( bool alsoioobj_ )
{
    bool ret = true;
    if ( strl() )
	{ ret = strl()->close(); if ( !ret ) errmsg_ = strl()->errMsg(); }
    delete trl_; trl_ = 0;
    delete dataset_; dataset_ = 0;
    psioprov_ = 0;
    nrtrcs_ = 0;

    if ( alsoioobj_ )
    {
	delete ioobj_; ioobj_ = 0;
	delete seldata_; seldata_ = 0;
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
    if ( ioobj_ ) iopar.set( sKey::ID(), ioobj_->key() );
}


void SeisStoreAccess::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( sKey::ID() );
    BufferString tmp;
    if ( !res )
    {
	res = iopar.find( sKey::Name() );
	if ( res && *res )
	{
	    IOM().to( SeisTrcTranslatorGroup::ioContext().getSelKey() );
	    const IOObj* tryioobj_ = IOM().getLocal( res, 0 );
	    if ( !tryioobj_ )
		res = 0;
	    else
	    {
		tmp = tryioobj_->key();
		res = tmp.buf();
	    }
	}
    }

    if ( res && *res )
    {
	IOObj* ioob = IOM().get( res );
	if ( ioob && (!ioobj_ || ioobj_->key() != ioob->key()) )
	    setIOObj( ioob );
	delete ioob;
    }

    if ( !seldata_ )
	seldata_ = Seis::SelData::get( iopar );
    if ( seldata_->isAll() && (seldata_->geomID()<0) )
	{ delete seldata_; seldata_ = 0; }

    if ( strl() )
    {
	strl()->setSelData( seldata_ );
	strl()->usePar( iopar );
    }
}

//--- SeqIO

void Seis::SeqIO::fillPar( IOPar& iop ) const
{
    Seis::putInPar( geomType(), iop );
}

mImplFactory( Seis::SeqInp, Seis::SeqInp::factory );
mImplFactory( Seis::SeqOut, Seis::SeqOut::factory );


void Seis::SeqInp::fillPar( IOPar& iop ) const
{
    Seis::SeqIO::fillPar( iop );
    Seis::putInPar( geomType(), iop );
}


Seis::GeomType Seis::SeqInp::getGeomType( const IOPar& iop )
{
    Seis::GeomType gt;
    return Seis::getFromPar(iop,gt) ? gt : Seis::Vol;
}


Seis::ODSeqInp::ODSeqInp()
    : rdr_(0)
    , psrdr_(0)
    , gath_(*new SeisTrcBuf(true))
    , curposidx_(-1)
    , segidx_(0)
    , ldidx_(0)
{
}

Seis::ODSeqInp::~ODSeqInp()
{
    delete rdr_;
    delete psrdr_;
    delete &gath_;
}


Seis::GeomType Seis::ODSeqInp::geomType() const
{
    return rdr_		?				rdr_->geomType()
	: (psrdr_	? (psrdr_->is2D()	?	Seis::LinePS
						:	Seis::VolPS)
						:	Seis::Vol);
}


void Seis::ODSeqInp::initClass()
{
    Seis::SeqInp::factory().addCreator( create, sKeyODType );
}


bool Seis::ODSeqInp::usePar( const IOPar& iop )
{
    if ( rdr_ ) delete rdr_; rdr_ = 0;
    if ( psrdr_ ) delete psrdr_; psrdr_ = 0;

    Seis::GeomType gt = getGeomType( iop );
    if ( !Seis::isPS(gt) )
    {
	rdr_ = new SeisTrcReader;
	rdr_->usePar( iop );
	if ( !rdr_->errMsg().isEmpty() )
	{
	    errmsg_ = rdr_->errMsg();
	    delete rdr_; rdr_ = 0;
	}
	return rdr_;
    }

    errmsg_ = tr("TODO: create PS Reader from IOPar");
    return false;
}


void Seis::ODSeqInp::fillPar( IOPar& iop ) const
{
    Seis::SeqInp::fillPar( iop );
    if ( rdr_ ) rdr_->fillPar( iop );
}


bool Seis::ODSeqInp::get( SeisTrc& trc ) const
{
    if ( !rdr_ && !psrdr_ )
    { errmsg_ = tr("No reader available"); return false; }

    if ( rdr_ )
    {
	if ( rdr_->get(trc) )
	    return true;
	errmsg_ = rdr_->errMsg();
	return false;
    }

    if ( !gath_.isEmpty() )
    {
	trc = *gath_.get( 0 );
	delete gath_.remove( ((int)0) );
	return true;
    }

    curposidx_++;
    if ( psrdr_->is2D() )
    {
	const SeisPS2DReader& rdr = *((const SeisPS2DReader*)psrdr_);
	const PosInfo::Line2DPos& lp = rdr.posData().positions()[curposidx_];
	if ( !rdr.getGath(lp.nr_,gath_) )
	{
	    errmsg_ = rdr.errMsg();
	    return false;
	}
    }
    else
    {
	const SeisPS3DReader& rdr = *((const SeisPS3DReader*)psrdr_);
	const PosInfo::LineData& ld = *rdr.posData()[ldidx_];
	const PosInfo::LineData::Segment& seg = ld.segments_[segidx_];
	if ( curposidx_ <= seg.nrSteps() )
	{
	    const BinID curpos( ld.linenr_, seg.atIndex(curposidx_) );
	    if ( !rdr.getGather(curpos,gath_) )
	    {
		errmsg_ = rdr.errMsg();
		return false;
	    }
	}
	else
	{
	    segidx_++; curposidx_ = -1;
	    if ( segidx_ >= ld.segments_.size() )
	    {
		segidx_ = 0;
		ldidx_++;
		if ( ldidx_ >= rdr.posData().size() )
		    return false;
	    }
	}
    }

    return get( trc );
}


Seis::Bounds* Seis::ODSeqInp::getBounds() const
{
    if ( rdr_ ) return rdr_->getBounds();
    return 0; //TODO PS bounds
}


int Seis::ODSeqInp::estimateTotalNumber() const
{
    Seis::Bounds* bds = getBounds();
    return bds ? bds->expectedNrTraces() : -1;
}


Seis::ODSeqOut::~ODSeqOut()
{
    delete wrr_;
}


Seis::GeomType Seis::ODSeqOut::geomType() const
{
    return wrr_ ? wrr_->geomType() : Seis::Vol;
}


void Seis::ODSeqOut::initClass()
{
    Seis::SeqOut::factory().addCreator( create, sKeyODType );
}


bool Seis::ODSeqOut::usePar( const IOPar& iop )
{
    if ( wrr_ ) delete wrr_;
    IOObj* i = 0; wrr_ = new SeisTrcWriter( i );
    wrr_->usePar( iop );
    if ( !wrr_->errMsg().isEmpty() )
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
