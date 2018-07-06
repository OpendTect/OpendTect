/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/


#include "seisseqio.h"
#include "seisblockstr.h"
#include "seiscbvs.h"
#include "seiswrite.h"
#include "seisbounds.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seispsioprov.h"
#include "seisselectionimpl.h"
#include "seisbuf.h"
#include "iostrm.h"
#include "iopar.h"
#include "dbman.h"
#include "dbdir.h"
#include "strmprov.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "keystrs.h"

const char* SeisStoreAccess::sNrTrcs = "Nr of traces";
const char* Seis::SeqIO::sKeyODType = "OpendTect";


SeisStoreAccess::SeisStoreAccess( const DBKey& dbky )
	: ioobj_(0)
	, trl_(0)
	, dataset_(0)
	, seldata_(0)
	, is2d_(false)
	, psioprov_(0)
{
    PtrMan<IOObj> inpioobj = DBM().get( dbky );
    setIOObj( inpioobj );
}


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
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    IOStream iostrm( "_tmp_SeisStoreAccess", dbdir ? dbdir->newTmpKey()
						   : DBKey::getInvalid() );
    iostrm.setGroup( !isps ?
	   ( is2d_ ? mTranslGroupName(SeisTrc2D) : mTranslGroupName(SeisTrc) )
	 : ( is2d_ ? mTranslGroupName(SeisPS2D) : mTranslGroupName(SeisPS3D)) );
    iostrm.setTranslator( is2d_ || isps	? CBVSSeisTrcTranslator::translKey()
					: BlocksSeisTrcTranslator::translKey());
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

    const SeisIOObjInfo info( ioobj_ );
    is2d_ = info.is2D();
    const bool isps = info.isPS();

    trl_ = ioobj_->createTranslator();
    if ( isps )
	psioprov_ = SPSIOPF().provider( ioobj_->translator() );
    else if ( is2d_ )
    {
	dataset_ = new Seis2DDataSet( *ioobj_ );
	if ( !ioobj_->name().isEmpty() )
	    dataset_->setName( ioobj_->name() );
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
	    const IOObj* tryioobj = DBM().getByName( IOObjContext::Seis, res );
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
	IOObj* ioob = DBM().get( DBKey::getFromStr(res) );
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

mImplClassFactory( Seis::SeqInp, factory );
mImplClassFactory( Seis::SeqOut, factory );


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
