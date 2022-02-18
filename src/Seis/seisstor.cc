/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/


#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbounds.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seisseqio.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"

#include "coordsystem.h"
#include "iostrm.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "strmprov.h"
#include "survgeom.h"
#include "uistrings.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";
const char* Seis::SeqIO::sKeyODType = "OpendTect";

const char* SeisStoreAccess::sKeyHeader()
{
    return "Header";
}


SeisStoreAccess::Setup::Setup( const IOObj& ioobj, const Seis::GeomType* gt )
    : ioobj_(ioobj.clone())
    , geomid_(mUdfGeomID)
    , compnr_(-1)
{
    const bool tempobj = ioobj.isTmp();
    Seis::GeomType usedgt = Seis::Vol;
    if ( gt )
    {
	usedgt = *gt;
	if ( !tempobj )
	{
	    const Seis::GeomType objgt =
			    Seis::geomTypeOf( SeisTrcTranslator::is2D(ioobj),
					      SeisTrcTranslator::isPS(ioobj) );
	    if ( objgt != *gt )
		{ pErrMsg("Incompatible arguments provided"); }
	}
    }
    else
    {
	if ( tempobj )
	    { pErrMsg("GeomType for temporary object must be provided"); }
	else
	{
	    usedgt = Seis::geomTypeOf( SeisTrcTranslator::is2D(ioobj),
				       SeisTrcTranslator::isPS(ioobj) );
	}
    }

    geomtype( usedgt );
}


SeisStoreAccess::Setup::Setup( const IOObj& ioobj, Pos::GeomID gid,
			       const Seis::GeomType* gt )
    : Setup(ioobj,gt)
{
    if ( Seis::is2D(geomtype_) != Survey::is2DGeom(gid) ||
	 (gt && Seis::is2D(*gt) != Survey::is2DGeom(gid)) )
	{ pErrMsg("Incompatible arguments provided"); }

    geomid( gid );
}


SeisStoreAccess::Setup::Setup( const Setup& oth )
{
    *this = oth;
}


SeisStoreAccess::Setup::~Setup()
{
    delete ioobj_;
    delete seldata_;
}


SeisStoreAccess::Setup& SeisStoreAccess::Setup::operator =( const Setup& oth )
{
    if ( this != &oth )
    {
	deleteAndZeroPtr( ioobj_ );
	if ( oth.ioobj_ )
	    ioobj( *oth.ioobj_ );

	geomtype( oth.geomtype_ );
	geomid( oth.geomid_ );
	coordsys_ = nullptr;
	if ( oth.coordsys_ )
	    coordsys( *oth.coordsys_.ptr() );
	seldata( oth.seldata_ );
	hdrtxt( oth.hdrtxt_ );
	compnr( oth.compnr_ );
    }

    return *this;
}


PtrMan<IOObj> SeisStoreAccess::Setup::getIOObj() const
{
    PtrMan<IOObj> ret = ioobj_ ? ioobj_->clone() : nullptr;
    if ( !ret )
	return nullptr;

    IOPar& iop = ret->pars();
    IOPar seistrpar;
    SeisTrcTranslator::setType( geomtype_, seistrpar );
    SeisTrcTranslator::setGeomID( geomid_, seistrpar );
    if ( coordsys_ )
	SeisTrcTranslator::setCoordSys( *coordsys_.ptr(), seistrpar );

    if ( !hdrtxt_.isEmpty() )
	seistrpar.set( SeisStoreAccess::sKeyHeader(), hdrtxt_ );

    iop.mergeComp( seistrpar, SeisTrcTranslator::sKeySeisTrPars() );

    return ret;
}


void SeisStoreAccess::Setup::usePar( const IOPar& iop )
{
    if ( Seis::getFromPar(iop,geomtype_) )
	geomtype( geomtype_ );

    iop.get( sKey::GeomID(), geomid_ );
    PtrMan<Seis::SelData> sd = Seis::SelData::get( iop );
    if ( sd && !sd->isAll() )
	seldata( sd.ptr() );

    ConstRefMan<Coords::CoordSystem> crssys =
		Coords::CoordSystem::createSystem( iop );
    if ( crssys )
	coordsys( *crssys.ptr() );

    BufferString hdr;
    if ( iop.get(sKeyHeader(),hdr) && !hdr.isEmpty() )
	hdrtxt( hdr );

    //TODO: support compnr_?
    if ( ioobj_->isTmp() || ioobj_->isProcTmp() )
	ioobj_->pars().merge( iop );
}


SeisStoreAccess::Setup& SeisStoreAccess::Setup::ioobj( const IOObj& obj )
{
    delete ioobj_;
    ioobj_ = obj.clone();
    return *this;
}


SeisStoreAccess::Setup& SeisStoreAccess::Setup::geomtype( Seis::GeomType gt )
{
    geomtype_ = gt;
    if ( Seis::is3D(geomtype_) )
	geomid( Survey::default3DGeomID() );

    return *this;
}


SeisStoreAccess::Setup& SeisStoreAccess::Setup::seldata(
						const Seis::SelData* sd )
{
    delete seldata_;
    seldata_ = sd ? sd->clone() : nullptr;
    return *this;
}


SeisStoreAccess::Setup& SeisStoreAccess::Setup::coordsys(
					const Coords::CoordSystem& crssys )
{
    coordsys_ = &crssys;
    return *this;
}


// SeisStoreAccess

SeisStoreAccess::SeisStoreAccess( const MultiID& dbkey, Seis::GeomType gt )
{
    PtrMan<IOObj> ioobj = IOM().get( dbkey );
    if ( !ioobj )
    {
	errmsg_ = uiStrings::phrCannotFindObjInDB();
	return;
    }

    const Setup ssasu( *ioobj, &gt );
    setIOObj( ssasu );
}


SeisStoreAccess::SeisStoreAccess( const IOObj* ioobj, const Seis::GeomType* gt )
{
    if ( !ioobj )
	return;

    const Setup ssasu( *ioobj, gt );
    setIOObj( ssasu );
}


SeisStoreAccess::SeisStoreAccess( const IOObj* ioobj, Pos::GeomID gid,
				  const Seis::GeomType* gt )
{
    if ( !ioobj )
	return;

    Setup ssasu( *ioobj, gt );
    ssasu.geomid( gid );
    setIOObj( ssasu );
}


SeisStoreAccess::SeisStoreAccess( const Setup& ssasu )
{
    setIOObj( ssasu );
}


IOObj& SeisStoreAccess::getTmp( const char* fnm, bool isps, bool is2d )
{
    static PtrMan<IOStream> iostrm =
	   new IOStream( "_tmp_SeisStoreAccess", toString(IOObj::tmpID()) );
    iostrm->setGroup( !isps ?
	   ( is2d ? mTranslGroupName(SeisTrc2D) : mTranslGroupName(SeisTrc) )
	 : ( is2d ? mTranslGroupName(SeisPS2D) : mTranslGroupName(SeisPS3D)) );
    iostrm->setTranslator( CBVSSeisTrcTranslator::translKey() );
    iostrm->fileSpec().setFileName( fnm && *fnm ? fnm
						: StreamProvider::sStdIO() );

    return *iostrm.ptr();
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


void SeisStoreAccess::setIOObj( const Setup& ssasu )
{
    PtrMan<IOObj> ioobj = ssasu.getIOObj();
    setIOObj( ioobj.ptr() );
    if ( ssasu.seldata_ && !ssasu.seldata_->isAll() )
	setSelData( ssasu.seldata_->clone() );
}


void SeisStoreAccess::setIOObj( const IOObj* ioobj )
{
    close();
    if ( !ioobj )
	return;

    ioobj_ = ioobj->clone();
    is2d_ = SeisTrcTranslator::is2D( *ioobj_, true );
    const bool isps = SeisTrcTranslator::isPS( *ioobj_, true );

    trl_ = ioobj_->createTranslator();
    if ( isps )
	psioprov_ = SPSIOPF().provider( ioobj_->translator() );
    else if ( is2d_ || SeisTrcTranslator::is2D(*ioobj_) )
    {
	dataset_ = new Seis2DDataSet( *ioobj_ );
	if ( !ioobj_->name().isEmpty() )
	    dataset_->setName( ioobj_->name() );

	mDynamicCastGet(SeisTrc2DTranslator*,trtrl2d,trl_)
	if ( trtrl2d )
	    trtrl2d->setDataSet( *dataset_ );
    }
    else
    {
	if ( !trl_ )
	    deleteAndZeroPtr( ioobj_ );
	else if ( strl() && seldata_ && !seldata_->isAll() )
	    strl()->setSelData( seldata_ );
    }
}


const Conn* SeisStoreAccess::curConn3D() const
{ return !is2d_ && strl() ? strl()->curConn() : nullptr; }
Conn* SeisStoreAccess::curConn3D()
{ return !is2d_ && strl() ? strl()->curConn() : nullptr; }


Pos::GeomID SeisStoreAccess::geomID() const
{
    const SeisTrcTranslator* seistrl = seisTranslator();
    if ( seistrl && Survey::isValidGeomID(seistrl->curGeomID()) )
	return seistrl->curGeomID();
    else if ( seldata_ )
	return seldata_->geomID();
    else if ( ioobj_ )
    {
	Pos::GeomID gid;
	if ( ioobj_->pars().get(sKey::GeomID(),gid) &&
	     Survey::isValidGeomID(gid) )
	{
	    if ( (is2d_ && Survey::is2DGeom(gid)) ||
		 (!is2d_ && Survey::is3DGeom(gid)) )
		return gid;
	}

	return Survey::GM().getGeomID( ioobj_->name() );
    }

    return mUdfGeomID;
}


void SeisStoreAccess::setSelData( Seis::SelData* tsel )
{
    delete seldata_; seldata_ = tsel;
    if ( strl() )
	strl()->setSelData( seldata_ );
}


bool SeisStoreAccess::cleanUp( bool alsoioobj_ )
{
    bool ret = true;
    if ( strl() )
	{ ret = strl()->close(); if ( !ret ) errmsg_ = strl()->errMsg(); }

    deleteAndZeroPtr( trl_ );
    deleteAndZeroPtr( dataset_ );
    psioprov_ = nullptr;
    nrtrcs_ = 0;

    if ( alsoioobj_ )
    {
	deleteAndZeroPtr( ioobj_ );
	deleteAndZeroPtr( seldata_ );
    }

    return ret;
}


bool SeisStoreAccess::close()
{
    return cleanUp( false );
}


void SeisStoreAccess::fillPar( IOPar& iopar ) const
{
    if ( ioobj_ )
	iopar.set( sKey::ID(), ioobj_->key() );
}


PtrMan<IOObj> SeisStoreAccess::getFromPar( const IOPar& iop )
{
    PtrMan<IOObj> ret;

    MultiID dbkey;
    if ( !iop.get(sKey::ID(),dbkey) )
    {
	BufferString dsnm;
	if ( iop.get(sKey::Name(),dsnm) && !dsnm.isEmpty() )
	{
	    IOM().to( IOObjContext::Seis );
	    ret = IOM().getLocal( dsnm.str(), nullptr );
	    if ( ret )
		dbkey = ret->key();
	}
    }

    ret = IOM().get( dbkey );
    return ret;
}


void SeisStoreAccess::usePar( const IOPar& iop )
{
    PtrMan<IOObj> newobj = getFromPar( iop );
    if ( newobj && (!ioobj_ || !areEqual(newobj.ptr(),ioobj_)) )
	setIOObj( newobj.ptr() );

    if ( !seldata_ )
	seldata_ = Seis::SelData::get( iop );
    if ( seldata_ && seldata_->isAll() )
	deleteAndZeroPtr( seldata_ );

    if ( strl() )
    {
	strl()->usePar( iop );
	strl()->setSelData( seldata_ );
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
    deleteAndZeroPtr( rdr_ );
    deleteAndZeroPtr( psrdr_ );

    const Seis::GeomType gt = getGeomType( iop );
    if ( !Seis::isPS(gt) )
    {
	PtrMan<IOObj> ioobj = SeisStoreAccess::getFromPar( iop );
	if ( !ioobj )
	    return false;

	rdr_ = new SeisTrcReader( *ioobj.ptr(), &gt );
	rdr_->usePar( iop );
	if ( !rdr_->errMsg().isEmpty() )
	{
	    errmsg_ = rdr_->errMsg();
	    deleteAndZeroPtr( rdr_ );;
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
    deleteAndZeroPtr( wrr_ );
    PtrMan<IOObj> ioobj = SeisStoreAccess::getFromPar( iop );
    if ( !ioobj )
	return false;

    Seis::GeomType gt = Seis::Vol;
    Seis::getFromPar( iop, gt );
    wrr_ = new SeisTrcWriter( *ioobj.ptr(), &gt );
    wrr_->usePar( iop );
    if ( !wrr_->errMsg().isEmpty() )
    {
	errmsg_ = wrr_->errMsg();
	deleteAndZeroPtr( wrr_ );
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
