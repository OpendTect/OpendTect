/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seispsioprov.h"

#include "file.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "iox.h"
#include "posinfo.h"
#include "seisbuf.h"
#include "seiscbvsps.h"
#include "seispacketinfo.h"
#include "seispscubetr.h"
#include "seispsfact.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seisselection.h"
#include "seistrc.h"
#include "survinfo.h"
#include "unitofmeasure.h"


const char* SeisPSIOProvider::sKeyCubeID = "=Cube.ID";

namespace PreStack
{

static const char* sKeyIsCorr = "Is Corrected";
static const char* sKeyIsAngleGather = "Angle Gather";

}


Pos::GeomID SeisPS3DReader::geomID() const
{
    return Survey::default3DGeomID();
}


SeisPSIOProvider::SeisPSIOProvider( const char* t )
    : type_(t)
{}


SeisPSIOProvider::~SeisPSIOProvider()
{}


SeisPS3DReader*
	SeisPSIOProvider::get3DReader( const IOObj& ioobj, int inl ) const
{
    return make3DReader( ioobj.fullUserExpr(true), inl );
}


SeisPS2DReader* SeisPSIOProvider::get2DReader( const IOObj& ioobj,
						Pos::GeomID geomid ) const
{
    return make2DReader( ioobj.fullUserExpr(true), geomid );
}


SeisPS2DReader* SeisPSIOProvider::get2DReader( const IOObj& ioobj,
					       const char* lnm ) const
{
    return make2DReader( ioobj.fullUserExpr(true), lnm );
}


SeisPSWriter* SeisPSIOProvider::get3DWriter( const IOObj& ioobj ) const
{
    return make3DWriter( ioobj.fullUserExpr(false) );
}


SeisPSWriter* SeisPSIOProvider::get2DWriter( const IOObj& ioobj,
					     Pos::GeomID geomid ) const
{
    return make2DWriter( ioobj.fullUserExpr(false), geomid );
}


SeisPSWriter* SeisPSIOProvider::get2DWriter( const IOObj& ioobj,
					     const char* lnm ) const
{
    return make2DWriter( ioobj.fullUserExpr(false), lnm );
}


bool SeisPSIOProvider::fetchGeomIDs( const IOObj& ioobj,
				     TypeSet<Pos::GeomID>& geomids ) const
{
    return getGeomIDs( ioobj.fullUserExpr(true), geomids );
}


bool SeisPSIOProvider::fetchLineNames( const IOObj& ioobj,
				       BufferStringSet& lnms ) const
{
    return getLineNames( ioobj.fullUserExpr(true), lnms );
}


bool SeisPSIOProvider::getLineNames( const char* dirnm,
					BufferStringSet& linenms ) const
{
    linenms.setEmpty();
    TypeSet<Pos::GeomID> geomids;
    if ( !getGeomIDs(dirnm,geomids) )
	return false;

    for ( int idx=0; idx<geomids.size(); idx++ )
	linenms.add( Survey::GM().getName(geomids[idx]) );

    return linenms.size();
}


const UnitOfMeasure* SeisPSIOProvider::offsetUnit( const IOObj*, bool& isfound )
{
    //TODO impl from IOObj
    isfound = true;
    return UnitOfMeasure::surveyDefOffsetUnit();
}


const UnitOfMeasure* SeisPSIOProvider::offsetUnit( Seis::OffsetType typ )
{
    if ( typ == Seis::OffsetType::OffsetMeter )
	return UnitOfMeasure::meterUnit();
    if ( typ == Seis::OffsetType::OffsetFeet )
	return UnitOfMeasure::feetUnit();
    if ( typ == Seis::OffsetType::AngleRadians )
	return UnitOfMeasure::radiansUnit();
    if ( typ == Seis::OffsetType::AngleDegrees )
	return UnitOfMeasure::degreesUnit();

    return nullptr;
}


bool SeisPSIOProvider::getGatherOffsetType( const IOPar& par,
					    Seis::OffsetType& typ )
{
    BufferString offsetunit;
    const bool hasunit = par.get( sKeyOffsetUnit(), offsetunit ) &&
			 !offsetunit.isEmpty();
    if ( !hasunit )
    {
	bool offsetisangle;
	if ( !par.getYN(PreStack::sKeyIsAngleGather,offsetisangle) )
	    return false;

	typ = Seis::OffsetType::AngleDegrees;
	//Most usual case, but actually we do not know
	return true;
    }

    const UnitOfMeasure* offsuom = UoMR().get( Mnemonic::Dist,
					       offsetunit.str() );
    if ( offsuom && offsuom == UnitOfMeasure::meterUnit() )
    {
	typ = Seis::OffsetType::OffsetMeter;
	return true;
    }
    else if ( offsuom && offsuom == UnitOfMeasure::feetUnit() )
    {
	typ = Seis::OffsetType::OffsetFeet;
	return true;
    }

    const UnitOfMeasure* anguom = UoMR().get( Mnemonic::Ang,
					      offsetunit.str() );
    if ( anguom && anguom == UnitOfMeasure::radiansUnit() )
    {
	typ = Seis::OffsetType::AngleRadians;
	return true;
    }
    else if ( anguom && anguom == UnitOfMeasure::degreesUnit() )
    {
	typ = Seis::OffsetType::AngleDegrees;
	return true;
    }

    return false;
}


bool SeisPSIOProvider::getGatherCorrectedYN( const IOPar& par, bool& yn )
{
    bool iscorr;
    if ( !par.getYN(PreStack::sKeyIsCorr,iscorr) &&
	 !par.getYN("Is NMO Corrected",iscorr) )
	return false;

    yn = iscorr;
    return true;
}


void SeisPSIOProvider::setGatherOffsetType( Seis::OffsetType typ, IOPar& par )
{
    const bool isdist = Seis::isOffsetDist( typ );
    const bool isangle = Seis::isOffsetAngle( typ );
    if ( isdist || isangle )
    {
	const UnitOfMeasure* uom = offsetUnit( typ );
	par.set( sKeyOffsetUnit(), uom ? uom->name().str() : nullptr );
    }

    // For backward compatibility mainly:
    if ( isangle )
	par.setYN( PreStack::sKeyIsAngleGather, true );
    else
	par.removeWithKey( PreStack::sKeyIsAngleGather );
}


bool SeisPSIOProvider::setGatherOffsetType( Seis::OffsetType typ, IOObj& ioobj )
{
    setGatherOffsetType( typ, ioobj.pars() );
    return IOM().commitChanges( ioobj );
}


void SeisPSIOProvider::setGathersAreCorrected( bool yn, IOPar& par )
{
    par.setYN( PreStack::sKeyIsCorr, yn );
}


bool SeisPSIOProvider::setGathersAreCorrected( bool yn, IOObj& ioobj )
{
    setGathersAreCorrected( yn, ioobj.pars() );
    return IOM().commitChanges( ioobj );
}



SeisPSIOProviderFactory& SPSIOPF()
{
    mDefineStaticLocalObject( SeisPSIOProviderFactory, theinst, );
    return theinst;
}


SeisPSIOProviderFactory::~SeisPSIOProviderFactory()
{
    deepErase( provs_ );
}


const SeisPSIOProvider* SeisPSIOProviderFactory::provider( const char* t ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    if ( !t )
	return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
    {
	if ( provs_[idx]->type()==t )
	    return provs_[idx];
    }

    return nullptr;
}


SeisPSReader* SeisPSIOProviderFactory::getReader( const MultiID& dbkey,
						  const TrcKey& tk ) const
{
    PtrMan<IOObj> psseisobj = IOM().get( dbkey );
    return psseisobj ? getReader( *psseisobj.ptr(), tk ) : nullptr;
}


SeisPSReader* SeisPSIOProviderFactory::getReader( const IOObj& ioobj,
						  const TrcKey& tk ) const
{
    return tk.is3D() ? (SeisPSReader*)get3DReader( ioobj, tk.inl() )
		     : (tk.is2D()
			? (SeisPSReader*)get2DReader(ioobj,tk.geomID())
			: nullptr);
}


SeisPSWriter* SeisPSIOProviderFactory::getWriter( const MultiID& dbkey,
						  const TrcKey& tk ) const
{
    PtrMan<IOObj> psseisobj = IOM().get( dbkey );
    return psseisobj ? getWriter( *psseisobj.ptr(), tk ) : nullptr;
}


SeisPSWriter* SeisPSIOProviderFactory::getWriter( const IOObj& ioobj,
						  const TrcKey& tk ) const
{
    return tk.is3D() ? (SeisPSWriter*)get3DWriter( ioobj )
		     : (tk.is2D()
			? (SeisPSWriter*)get2DWriter(ioobj,tk.geomID())
			: nullptr);
}


void SeisPSIOProviderFactory::mk3DPostStackProxy( IOObj& ioobj )
{
    if ( ioobj.pars().hasKey(SeisPSIOProvider::sKeyCubeID) )
	return;

    IODir iodir( ioobj.key() );
    BufferString nm( "{" ); nm += ioobj.name(); nm += "}";
    auto* iox = new IOX( nm );
    iox->setTranslator( mTranslKey(SeisTrc,SeisPSCube) );
    iox->setGroup( mTranslGroupName(SeisTrc) );
    iox->acquireNewKeyIn( iodir.key() );
    ioobj.pars().set( SeisPSIOProvider::sKeyCubeID, iox->key() );
    iodir.commitChanges( &ioobj );
    iox->setOwnKey( ioobj.key() );
    iodir.addObj( iox, true );
}


bool SeisPSIOProviderFactory::getGeomIDs( const IOObj& ioobj,
					  TypeSet<Pos::GeomID>& geomids ) const
{
    if ( provs_.isEmpty() ) return false;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    return prov ? prov->fetchGeomIDs( ioobj, geomids )
		: false;
}


bool SeisPSIOProviderFactory::getLineNames( const IOObj& ioobj,
					    BufferStringSet& linenms ) const
{
    if ( provs_.isEmpty() ) return false;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    return prov ? prov->fetchLineNames( ioobj, linenms )
		: false;
}


SeisPS3DReader* SeisPSIOProviderFactory::get3DReader( const IOObj& ioobj,
						      int inl ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS3DReader* reader =
	prov ? prov->get3DReader( ioobj, inl ) : nullptr;

    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPS2DReader* SeisPSIOProviderFactory::get2DReader( const IOObj& ioobj,
						      Pos::GeomID geomid ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS2DReader* rdr = prov ? prov->get2DReader( ioobj, geomid ) : nullptr;
    if ( rdr )
	rdr->usePar( ioobj.pars() );

    return rdr;
}


SeisPS2DReader* SeisPSIOProviderFactory::get2DReader( const IOObj& ioobj,
						      const char* lnm ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPS2DReader* reader = prov ? prov->get2DReader( ioobj, lnm ) : nullptr;
    if ( reader )
	reader->usePar( ioobj.pars() );

    return reader;
}


SeisPSWriter* SeisPSIOProviderFactory::get3DWriter( const IOObj& ioobj ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer = prov ? prov->get3DWriter( ioobj ) : nullptr;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}


SeisPSWriter* SeisPSIOProviderFactory::get2DWriter( const IOObj& ioobj,
						    Pos::GeomID geomid ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer = prov ? prov->get2DWriter( ioobj, geomid ) : nullptr;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}


SeisPSWriter* SeisPSIOProviderFactory::get2DWriter( const IOObj& ioobj,
						    const char* lnm ) const
{
    if ( provs_.isEmpty() )
	return nullptr;

    const SeisPSIOProvider* prov = provider( ioobj.translator() );
    SeisPSWriter* writer = prov ? prov->get2DWriter( ioobj, lnm ) : nullptr;
    if ( writer )
	writer->usePar( ioobj.pars() );

    return writer;
}



// SeisPSReader
SeisPSReader::SeisPSReader()
{}


SeisPSReader::~SeisPSReader()
{}


SeisTrc* SeisPSReader::getTrace( const BinID& bid, int trcidx ) const
{
    SeisTrcBuf buf( true );
    if ( !getGather(bid,buf) || buf.size()<=trcidx )
	return nullptr;

    return buf.remove( trcidx );
}


ZSampling SeisPSReader::getZRange() const
{
    return SI().zRange( true );
}



// SeisPS3DTranslator
mDefSimpleTranslatorioContext(SeisPS3D,Seis)
mDefSimpleTranslatorioContext(SeisPS2D,Seis)


SeisPS3DTranslator::~SeisPS3DTranslator()
{}


bool SeisPS3DTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    if ( ioobj )
    {
	MultiID pseudocubeid;
	ioobj->pars().get( SeisPSIOProvider::sKeyCubeID, pseudocubeid );
	PtrMan<IOObj> pseudocubeobj = IOM().get( pseudocubeid );
	if ( pseudocubeobj )
	    IOM().permRemove( pseudocubeid );
    }

    return true;
}


bool SeisPS3DTranslator::implRename( const IOObj* ioobj,
				     const char* newnm ) const
{
    if ( ioobj )
    {
	MultiID pseudocubeid;
	ioobj->pars().get( SeisPSIOProvider::sKeyCubeID, pseudocubeid );
	PtrMan<IOObj> pseudocubeobj = IOM().get( pseudocubeid );
	if ( pseudocubeobj )
	{
	    BufferString newpseudocubenm( "{" );
	    newpseudocubenm.add( ioobj->name() ).add( "}" );
	    pseudocubeobj->setName( newpseudocubenm );
	    if ( !IOM().commitChanges(*pseudocubeobj) )
		return false;
	}
    }

    return Translator::implRename( ioobj, newnm );
}



// CBVSSeisPS3DTranslator
CBVSSeisPS3DTranslator::~CBVSSeisPS3DTranslator()
{}


bool CBVSSeisPS3DTranslator::implRemove( const IOObj* ioobj, bool deep ) const
{
    if ( !ioobj )
	return true;

    SeisPS3DTranslator::implRemove( ioobj, deep );

    const BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File::exists(fnm) )
	File::remove( fnm );

    return !File::exists(fnm);
}



// CBVSSeisPS2DTranslator
CBVSSeisPS2DTranslator::~CBVSSeisPS2DTranslator()
{}


bool CBVSSeisPS2DTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( File::exists(fnm) )
	File::remove( fnm );

    return !File::exists(fnm);
}



// SeisPSCubeSeisTrcTranslator
SeisPSCubeSeisTrcTranslator::SeisPSCubeSeisTrcTranslator( const char* nm,
							  const char* unm )
	: SeisTrcTranslator(nm,unm)
	, trc_(*new SeisTrc)
	, psrdr_(nullptr)
	, inforead_(false)
	, posdata_(*new PosInfo::CubeData)
{
}


SeisPSCubeSeisTrcTranslator::~SeisPSCubeSeisTrcTranslator()
{
    delete psrdr_;
    delete &trc_;
    delete &posdata_;
}


const char* SeisPSCubeSeisTrcTranslator::connType() const
{
    return XConn::sType();
}

static const char* sKeyOffsNr = "Default trace nr";


bool SeisPSCubeSeisTrcTranslator::initRead_()
{
    PtrMan<IOObj> ioobj = IOM().get( conn_->linkedTo() );
    if ( ioobj )
    {
	mDynamicCastGet(const IOX*,iox,ioobj.ptr())
	IOObj* useioobj = iox ? iox->getIOObj() : ioobj->clone();
	psrdr_ = SPSIOPF().get3DReader( *useioobj );
	int trcnr = -1;
	useioobj->pars().get( sKeyOffsNr, trcnr );
	if ( trcnr >= 0 )
	{
	    trcnrs_.erase();
	    trcnrs_ += trcnr;
	}

	delete useioobj;
    }
    else
    {
	mDynamicCastGet(StreamConn*,sconn,conn_->conn())
	if ( !sconn )
	{
	    errmsg_ = tr("Wrong connection from Object Manager");
	    return false;
	}

	psrdr_ = new SeisCBVSPS3DReader( sconn->fileName() );
    }

    conn_->close();
    errmsg_ = psrdr_ ? psrdr_->errMsg() : tr("Cannot find PS data store type");
    if ( !errmsg_.isEmpty() )
	return false;

    posdata_ = psrdr_->posData();
    posdata_.getInlRange( pinfo_.inlrg );
    posdata_.getCrlRange( pinfo_.crlrg );
    pinfo_.inlrg.sort(); pinfo_.crlrg.sort();
    curbinid_.inl() = pinfo_.inlrg.start;
    curbinid_.crl() = pinfo_.crlrg.start - pinfo_.crlrg.step;

    TypeSet<float> offss;
    if ( !doRead(trc_,&offss) )
	return false;

    insd_ = trc_.info().sampling;
    innrsamples_ = trc_.size();
    for ( int icomp=0; icomp<trc_.nrComponents(); icomp++ )
	addComp( trc_.data().getInterpreter(icomp)->dataChar(),
		 BufferString("O=",offss[icomp]) );

    curbinid_.inl() = pinfo_.inlrg.start;
    curbinid_.crl() = pinfo_.crlrg.start - pinfo_.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !posdata_.includes(bid.inl(),bid.crl()) )
	return false;

    curbinid_ = bid; curbinid_.crl() -= pinfo_.crlrg.step;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::toNext()
{
    for ( int crl=curbinid_.crl()+pinfo_.crlrg.step; crl<=pinfo_.crlrg.stop;
	    crl+=pinfo_.crlrg.step )
    {
	if ( posdata_.includes(curbinid_.inl(),crl) )
	{
	    BinID bid( curbinid_.inl(), crl );
	    if ( !seldata_ || seldata_->isOK(BinID(curbinid_.inl(),crl)) )
	    {
		curbinid_.crl() = crl;
		return true;
	    }
	}
    }

    curbinid_.inl() += pinfo_.inlrg.step;
    if ( curbinid_.inl() > pinfo_.inlrg.stop )
	return false;

    curbinid_.crl() = pinfo_.crlrg.start - pinfo_.crlrg.step;
    return toNext();
}


bool SeisPSCubeSeisTrcTranslator::commitSelections_()
{
    if ( !trcnrs_.isEmpty() )
	return true;

    for ( int idx=0; idx<tarcds_.size(); idx++ )
    {
	if ( tarcds_[idx]->destidx >= 0 )
	    trcnrs_ += idx;
    }

    return true;
}


bool SeisPSCubeSeisTrcTranslator::doRead( SeisTrc& trc, TypeSet<float>* offss )
{
    if ( !toNext() )
	return false;

    SeisTrc* newtrc = nullptr;
    if ( !trcnrs_.isEmpty() )
    {
	newtrc = psrdr_->getTrace( curbinid_, trcnrs_[0] );
	const int nrtrcnrs = trcnrs_.size();
	if ( nrtrcnrs > 1 )
	{
	    const int trcsz = newtrc->size();
	    const DataCharacteristics dc(
		    newtrc->data().getInterpreter(0)->dataChar() );
	    for ( int itrc=1; itrc<nrtrcnrs; itrc++ )
	    {
		PtrMan<SeisTrc> rdtrc =
		    psrdr_->getTrace( curbinid_, trcnrs_[itrc] );
		newtrc->data().addComponent( trcsz, dc, !rdtrc );
		if ( rdtrc )
		{
		    for ( int idx=0; idx<trcsz; idx++ )
			newtrc->set( idx, rdtrc->get(idx,0), itrc );
		}
	    }
	}
    }
    else
    {
	SeisTrcBuf tbuf( true );
	if ( psrdr_->getGather(curbinid_,tbuf) && !tbuf.isEmpty() )
	{
	    newtrc = new SeisTrc( *tbuf.get(0) );
	    const int trcsz = newtrc->size();
	    const DataCharacteristics trcdc(
		    newtrc->data().getInterpreter(0)->dataChar() );
	    if ( offss )
		*offss += newtrc->info().offset;

	    for ( int icomp=1; icomp<tbuf.size(); icomp++ )
	    {
		const SeisTrc& btrc = *tbuf.get(icomp);
		newtrc->data().addComponent( trcsz, trcdc );
		for ( int isamp=0; isamp<trcsz; isamp++ )
		    newtrc->set( isamp, btrc.get(isamp,0), icomp );

		if ( offss )
		    *offss += btrc.info().offset;
	    }
	}
    }

    if ( !newtrc )
	return false;

    trc = *newtrc;
    if ( seldata_ && !seldata_->isAll() )
    {
	trc.info() = newtrc->info();
	const Interval<float> zrg( seldata_->zRange() );
	trc.info().sampling.start = zrg.start;
	const float sr = trc.info().sampling.step;
	const int nrsamps = (int)(zrg.width() / sr + 1.5);
	trc.reSize( nrsamps, false );
	for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
	    for ( int idx=0; idx<nrsamps; idx++ )
		trc.set( idx, newtrc->getValue( zrg.start + idx * sr, icomp ),
			 icomp );
    }

    delete newtrc;
    return true;
}


bool SeisPSCubeSeisTrcTranslator::readInfo( SeisTrcInfo& inf )
{
    if ( !outcds_ )
	commitSelections();

    if ( inforead_ )
	return true;

    if ( !doRead(trc_) )
	return false;

    inforead_ = true;
    inf = trc_.info();
    return true;
}


bool SeisPSCubeSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !outcds_ )
	commitSelections();

    if ( inforead_ )
    {
	inforead_ = false;
	trc = trc_;
	return true;
    }

    inforead_ = false;
    return doRead( trc );
}


bool SeisPSCubeSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
    {
	if ( !toNext() )
	    return false;
    }

    return true;
}



// SeisPS2DTranslator
SeisPS2DTranslator::~SeisPS2DTranslator()
{}



// SeisPS3DReader
SeisPS3DReader::SeisPS3DReader()
{}


SeisPS3DReader::~SeisPS3DReader()
{}



// SeisPSWriter
SeisPSWriter::SeisPSWriter()
{}


SeisPSWriter::~SeisPSWriter()
{}
