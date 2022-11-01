/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribdataholder.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "linesetposinfo.h"
#include "seis2ddata.h"
#include "seisbounds.h"
#include "seisbufadapters.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seiscubeprov.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"


namespace Attrib
{

mAttrDefCreateInstance(StorageProvider)

void StorageProvider::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new SeisStorageRefParam() );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void StorageProvider::updateDesc( Desc& desc )
{
    updateDescAndGetCompNms( desc, 0 );
}


void StorageProvider::updateDescAndGetCompNms( Desc& desc,
					       BufferStringSet* compnms )
{
    const BufferString valstr( desc.getValParam(keyStr())->getStringValue(0) );
    const MultiID dbky( valstr.buf() );
    if ( dbky.isInMemoryID() )
    {
	const DataPack::FullID fid( dbky );
	if ( !DPM(fid).isPresent(fid) )
	    desc.setErrMsg( "Cannot find data in memory" );
	return;
    }
    else if ( !dbky.isDatabaseID() )
    {
	desc.setErrMsg( BufferString("Invalid MultiID: ",dbky.toString()) );
	return;
    }

    PtrMan<IOObj> ioobj = IOM().get( dbky );
    if ( !ioobj )
	return;

    SeisTrcReader rdr( *ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(Seis::PreScan) || rdr.isPS() )
    {
//	TODO desc.setErrMsg( rdr.errMsg() );
	return;
    }

    if ( compnms )
	compnms->erase();

    if ( rdr.is2D() )
    {
	if ( !rdr.dataSet() )
	{
	    uiString errmsg = tr("No dataset available for '%1'")
	                    .arg(ioobj->name());
//	    desc.setErrMsg( errmsg );
	    return;
	}

	StringView datatype = rdr.dataSet()->dataType();
	const bool issteering = datatype == sKey::Steering();
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl )
	{
	    if ( issteering )
	    {
		desc.setNrOutputs( Seis::Dip, 2 );
		if ( compnms )
		    compnms->add( rdr.dataSet()->name() );
	    }
	    else
		desc.setNrOutputs( Seis::UnknowData, 1 );
	}
	else if ( transl->componentInfo().isEmpty() )
	{
	    BufferStringSet complist;
	    SeisIOObjInfo::getCompNames( dbky, complist );
	    if ( complist.isEmpty() )
	    {
		if ( issteering )
		    desc.setNrOutputs( Seis::Dip, 2 );
		else
		    desc.setNrOutputs( Seis::UnknowData, 1 );
	    }
	    else
	    {
		desc.setNrOutputs( issteering ? Seis::Dip : Seis::UnknowData,
				   complist.size() );
		if ( compnms )
		    compnms->operator =( complist );
	    }
	}
	else
	{
	    for ( int idx=0; idx<transl->componentInfo().size(); idx++ )
		desc.addOutputDataType( (Seis::DataType)
					transl->componentInfo()[0]->datatype );
	}
    }
    else
    {
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl )
	{
	    uiString errmsg = tr("No data interpreter available for '%1'")
	                    .arg(ioobj->name());
//	    desc.setErrMsg ( errmsg );
	    return;
	}

	BufferString type;
	ioobj->pars().get( sKey::Type(), type );

	const int nrattribs = transl->componentInfo().size();
	if ( type == sKey::Steering() )
	    desc.setNrOutputs( Seis::Dip, nrattribs );
	else
	{
	    for ( int idx=1; idx<=nrattribs; idx++ )
		if ( desc.nrOutputs() < idx )
		    desc.addOutputDataType( (Seis::DataType)
				    transl->componentInfo()[idx-1]->datatype);
	}

	if ( compnms )
	    transl->getComponentNames( *compnms );
    }
}


StorageProvider::StorageProvider( Desc& desc )
    : Provider( desc )
    , stepoutstep_(-1,0)
{
    const MultiID dbky = getDBKey( &desc );
    const DataPack::FullID fid( dbky );
    isondisc_ = dbky.isInMemoryID() ? !DPM(fid).isPresent(fid) : true;
}


StorageProvider::~StorageProvider()
{
    delete mscprov_;
    delete ls2ddata_;
}


#undef mErrRet
#define mErrRet(s) { errmsg_ = s; deleteAndZeroPtr( mscprov_ ); return false; }

bool StorageProvider::checkInpAndParsAtStart()
{
    if ( status_!=Nada )
	return false;

    if ( !isondisc_ )
    {
	storedvolume_.zsamp_.start = 0;	//cover up for synthetics
	const DataPack::FullID fid = getDPID();
	auto stbdtp = DPM(fid).get<SeisTrcBufDataPack>( DataPack::getID(fid) );
	if ( !stbdtp )
	    return false;

	SeisPacketInfo si;
	stbdtp->trcBuf().fill( si );
	storedvolume_.hsamp_.setInlRange( si.inlrg );
	storedvolume_.hsamp_.setCrlRange( si.crlrg );
	storedvolume_.zsamp_ = si.zrg;
	return true;
    }

    const MultiID mid = getDBKey();
    if ( !mid.isDatabaseID() || !isOK() )
	return false;

    const SeisIOObjInfo seisinfo( mid );
    delete mscprov_;
    mscprov_ = new SeisMSCProvider( mid, seisinfo.geomType() );

    if ( !initMSCProvider() )
	mErrRet( mscprov_->errMsg() )

    const bool is2d = mscprov_->is2D();
    desc_.set2D( is2d );
    if ( !is2d )
	SeisTrcTranslator::getRanges( mid, storedvolume_, nullptr );
    else
    {
	Seis2DDataSet* dset = mscprov_->reader().dataSet();
	if ( !dset )
	    mErrRet( tr("2D seismic data/No data set found") );

	storedvolume_.hsamp_.start_.inl() = 0;
	storedvolume_.hsamp_.stop_.inl() = 1;
	storedvolume_.hsamp_.step_.inl() = 1;
	storedvolume_.hsamp_.include( BinID( 0,0 ) );
	storedvolume_.hsamp_.include( BinID( 0,SI().maxNrTraces(true) ) );
	storedvolume_.hsamp_.step_.crl() = 1; // what else?
	bool foundone = false;
	for ( int idx=0; idx<dset->nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = dset->geomID( idx );
	    StepInterval<int> trcrg; StepInterval<float> zrg;
	    if ( !dset->getRanges(geomid,trcrg,zrg) )
		continue;

	    if ( foundone )
	    {
		const int linenr = geomid.asInt();
		storedvolume_.hsamp_.include( BinID(linenr,trcrg.start) );
		storedvolume_.hsamp_.include( BinID(linenr,trcrg.stop) );
		storedvolume_.zsamp_.include( zrg );
	    }
	    else
	    {
		storedvolume_.hsamp_.setGeomID( geomid );
		storedvolume_.hsamp_.setTrcRange( trcrg );
		storedvolume_.zsamp_ = zrg;
		foundone = true;
	    }
	}
    }

    status_ = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    if ( status_==Nada && isondisc_ )
	return -1;

    if ( status_==StorageOpened )
    {
	if ( !setMSCProvSelData() )
	    return -1;

	status_ = Ready;
    }

    if ( !useshortcuts_ )
    {
	if ( getDesc().is2D() )
	    prevtrcnr_ = currentbid_.crl();

	bool validstartpos = startpos != BinID(-1,-1);
	if ( validstartpos && curtrcinfo_ && curtrcinfo_->binID() == startpos )
	{
	    alreadymoved_ = true;
	    return 1;
	}
    }

    bool advancefurther = true;
    while ( advancefurther )
    {
	if ( isondisc_ )
	{
	    SeisMSCProvider::AdvanceState res = mscprov_ ? mscprov_->advance()
						 : SeisMSCProvider::EndReached;
	    switch ( res )
	    {
		case SeisMSCProvider::Error:	{ errmsg_ = mscprov_->errMsg();
						      return -1; }
		case SeisMSCProvider::EndReached:	return 0;
		case SeisMSCProvider::Buffering:	continue;
						//TODO return 'no new position'

		case SeisMSCProvider::NewPosition:
		{
		    if ( useshortcuts_ )
			{ advancefurther = false; continue; }

		    SeisTrc* trc = mscprov_->get( 0, 0 );
		    if ( !trc ) continue; // should not happen

		    registerNewPosInfo( trc, startpos, firstcheck,
					advancefurther );
		}
	    }
	}
	else
	{
	    SeisTrc* trc = getTrcFromPack( BinID::noStepout(), 1 );
	    if ( !trc ) return 0;

	    status_ = Ready;
	    registerNewPosInfo( trc, startpos, firstcheck, advancefurther );
	}
    }

    setCurrentPosition( currentbid_ );
    alreadymoved_ = true;
    return 1;
}


void StorageProvider::registerNewPosInfo( SeisTrc* trc, const BinID& startpos,
					  bool firstcheck, bool& advancefurther)
{
    for ( int idx=0; idx<trc->nrComponents(); idx++ )
    {
	if ( datachar_.size()<=idx )
	    datachar_ +=
		trc->data().getInterpreter()->dataChar();
	else
	    datachar_[idx] =
		trc->data().getInterpreter()->dataChar();
    }

    curtrcinfo_ = 0;
    const SeisTrcInfo& newti = trc->info();
    currentbid_ = desc_.is2D()? BinID( 0, newti.trcNr() ) : newti.binID();
    trcinfobid_ = newti.binID();
    if ( firstcheck || startpos == BinID(-1,-1) || currentbid_ == startpos
	    || newti.binID() == startpos )
    {
	advancefurther = false;
	curtrcinfo_ = &trc->info();
    }
}


bool StorageProvider::getLine2DStoredVolume()
{
    if ( geomid_.isUdf() && desiredvolume_->hsamp_.is2D() )
	geomid_ = desiredvolume_->hsamp_.getGeomID();

    if ( geomid_.isUdf() )
	return true;

    Seis2DDataSet* dset = mscprov_->reader().dataSet();
    if ( !dset )
	mErrRet( tr("2D seismic data set not found") );

    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( !dset->getRanges(geomid_,trcrg,zrg) )
	mErrRet( tr("2D dataset %1 is not available for line %2")
		.arg(dset->name()).arg(Survey::GM().getName(geomid_)) );

    storedvolume_.hsamp_.setGeomID( geomid_ );
    storedvolume_.hsamp_.setTrcRange( trcrg );
    storedvolume_.zsamp_ = zrg;
    return true;
}


bool StorageProvider::getPossibleVolume( int, TrcKeyZSampling& globpv )
{
    if ( !possiblevolume_ )
	possiblevolume_ = new TrcKeyZSampling;

    const bool is2d = mscprov_ && mscprov_->is2D();
    if ( is2d && !getLine2DStoredVolume() )
	return false;

    *possiblevolume_ = storedvolume_;
    if ( is2d == globpv.hsamp_.is2D() )
	globpv.limitTo( *possiblevolume_ );
    if ( !isondisc_ )
	globpv = *possiblevolume_;
    return !globpv.isEmpty();
}


bool StorageProvider::initMSCProvider()
{
    if ( !mscprov_ || !mscprov_->prepareWork() )
	return false;

    updateStorageReqs();
    return true;
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl() <= prefix##bufferstepout_.inl() \
	    && ns.crl() <= prefix##bufferstepout_.crl() ) \
	return; \
\
    if ( ns.inl() > prefix##bufferstepout_.inl() ) \
	prefix##bufferstepout_.inl() = ns.inl(); \
    if ( ns.crl() > prefix##bufferstepout_.crl() ) \
	prefix##bufferstepout_.crl() = ns.crl();\
}


void StorageProvider::setReqBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(req);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::setDesBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(des);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::updateStorageReqs( bool )
{
    if ( !mscprov_ ) return;

    mscprov_->setStepout( desbufferstepout_.inl(), desbufferstepout_.crl(),
			  false );
    mscprov_->setStepout( reqbufferstepout_.inl(), reqbufferstepout_.crl(),
			  true );
}


SeisMSCProvider* StorageProvider::getMSCProvider( bool& needmscprov) const
{
    needmscprov = isondisc_;
    return mscprov_;
}


bool StorageProvider::setMSCProvSelData()
{
    if ( !mscprov_ ) return false;

    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.psIOProv() ) return false;

    const bool is2d = reader.is2D();
    const bool haveseldata = seldata_ && !seldata_->isAll();

    if ( haveseldata && seldata_->type() == Seis::Table )
	return setTableSelData();

    if ( is2d )
	return set2DRangeSelData();

    if ( !desiredvolume_ )
    {
	for ( int idp=0; idp<parents_.size(); idp++ )
	{
	    if ( !parents_[idp] ) continue;

	    if ( parents_[idp]->getDesiredVolume() )
	    {
		setDesiredVolume( *parents_[idp]->getDesiredVolume() );
		break;
	    }
	}
	if ( !desiredvolume_ )
	    return true;
    }

    if ( !checkDesiredVolumeOK() )
	return false;

    TrcKeyZSampling cs;
    cs.hsamp_.start_.inl() =
	desiredvolume_->hsamp_.start_.inl()<storedvolume_.hsamp_.start_.inl() ?
	storedvolume_.hsamp_.start_.inl() : desiredvolume_->hsamp_.start_.inl();
    cs.hsamp_.stop_.inl() =
	desiredvolume_->hsamp_.stop_.inl() > storedvolume_.hsamp_.stop_.inl() ?
	storedvolume_.hsamp_.stop_.inl() : desiredvolume_->hsamp_.stop_.inl();
    cs.hsamp_.stop_.crl() =
	desiredvolume_->hsamp_.stop_.crl() > storedvolume_.hsamp_.stop_.crl() ?
	storedvolume_.hsamp_.stop_.crl() : desiredvolume_->hsamp_.stop_.crl();
    cs.hsamp_.start_.crl() =
	desiredvolume_->hsamp_.start_.crl()<storedvolume_.hsamp_.start_.crl() ?
	storedvolume_.hsamp_.start_.crl() : desiredvolume_->hsamp_.start_.crl();
    cs.zsamp_.start = desiredvolume_->zsamp_.start < storedvolume_.zsamp_.start
		? storedvolume_.zsamp_.start : desiredvolume_->zsamp_.start;
    cs.zsamp_.stop = desiredvolume_->zsamp_.stop > storedvolume_.zsamp_.stop ?
		     storedvolume_.zsamp_.stop : desiredvolume_->zsamp_.stop;

    reader.setSelData( haveseldata ? seldata_->clone()
				   : new Seis::RangeSelData(cs) );

    SeisTrcTranslator* transl = reader.seisTranslator();
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] && transl && transl->componentInfo()[idx] )
	    transl->componentInfo()[idx]->destidx = -1;
    }

    return true;
}


bool StorageProvider::setTableSelData()
{
    if ( !isondisc_ ) return false;	//in this case we might not use a table

    Seis::SelData* seldata = seldata_->clone();
    seldata->extendZ( extraz_ );
    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.is2D() )
	seldata->setGeomID( geomid_ );

    reader.setSelData( seldata );
    SeisTrcTranslator* transl = reader.seisTranslator();
    if ( !transl ) return false;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] && transl->componentInfo().size()>idx )
	    transl->componentInfo()[idx]->destidx = -1;
    }
    return true;
}


bool StorageProvider::set2DRangeSelData()
{
    if ( !isondisc_ )
	return false;

    mDynamicCastGet(const Seis::RangeSelData*,rsd,seldata_)
    Seis::RangeSelData* seldata = rsd ? (Seis::RangeSelData*)rsd->clone()
				      : new Seis::RangeSelData( false );
    SeisTrcReader& reader = mscprov_->reader();
    Seis2DDataSet* dset = reader.dataSet();
    if ( !dset )
    {
	if ( !rsd && seldata )
	    delete seldata;
	return false;
    }

    if ( Survey::is2DGeom(geomid_) )
    {
	seldata->setGeomID( geomid_ );
	StepInterval<float> dszrg; StepInterval<int> trcrg;
	if ( dset->getRanges(geomid_,trcrg,dszrg) )
	{
	    if ( !checkDesiredTrcRgOK(trcrg,dszrg) )
		return false;

	    StepInterval<int> rg;
	    rg.start = desiredvolume_->hsamp_.start_.crl() < trcrg.start?
			trcrg.start : desiredvolume_->hsamp_.start_.crl();
	    rg.stop = desiredvolume_->hsamp_.stop_.crl() > trcrg.stop ?
			trcrg.stop : desiredvolume_->hsamp_.stop_.crl();
	    rg.step = desiredvolume_->hsamp_.step_.crl() > trcrg.step ?
			desiredvolume_->hsamp_.step_.crl() : trcrg.step;
	    seldata->setCrlRange( rg );
	    Interval<float> zrg;
	    zrg.start = desiredvolume_->zsamp_.start < dszrg.start ?
			dszrg.start : desiredvolume_->zsamp_.start;
	    zrg.stop = desiredvolume_->zsamp_.stop > dszrg.stop ?
			dszrg.stop : desiredvolume_->zsamp_.stop;
	    seldata->setZRange( zrg );
	}
	reader.setSelData( seldata );
    }
    else if ( !rsd && seldata )
	delete seldata;

    return true;
}


bool StorageProvider::checkDesiredVolumeOK()
{
    if ( !desiredvolume_ )
	return true;

    const bool inlwrong =
	desiredvolume_->hsamp_.start_.inl() > storedvolume_.hsamp_.stop_.inl()
     || desiredvolume_->hsamp_.stop_.inl() < storedvolume_.hsamp_.start_.inl();
    const bool crlwrong =
	desiredvolume_->hsamp_.start_.crl() > storedvolume_.hsamp_.stop_.crl()
     || desiredvolume_->hsamp_.stop_.crl() < storedvolume_.hsamp_.start_.crl();
    const float zepsilon = 1e-06f;
    const bool zwrong =
	desiredvolume_->zsamp_.start > storedvolume_.zsamp_.stop+zepsilon ||
	desiredvolume_->zsamp_.stop < storedvolume_.zsamp_.start-zepsilon;
    const float zstepratio =
		desiredvolume_->zsamp_.step / storedvolume_.zsamp_.step;
    const bool zstepwrong = zstepratio > 100 || zstepratio < 0.01;

    if ( !inlwrong && !crlwrong && !zwrong && !zstepwrong )
	return true;

    errmsg_ = tr("'%1' contains no data in selected area:\n")
		.arg( desc_.userRef() );

    if ( inlwrong )
	errmsg_.append( tr( "Inline range is: %1-%2 [%3]\n")
		      .arg( storedvolume_.hsamp_.start_.inl() )
		      .arg( storedvolume_.hsamp_.stop_.inl() )
		      .arg( storedvolume_.hsamp_.step_.inl() ) );
    if ( crlwrong )
	errmsg_.append( tr( "Crossline range is: %1-%2 [%3]\n")
		      .arg( storedvolume_.hsamp_.start_.crl() )
		      .arg( storedvolume_.hsamp_.stop_.crl() )
		      .arg( storedvolume_.hsamp_.step_.crl() ) );
    if ( zwrong )
	errmsg_.append( tr( "Z range is: %1-%2\n")
		      .arg( storedvolume_.zsamp_.start )
		      .arg( storedvolume_.zsamp_.stop ) );
    if ( inlwrong || crlwrong || zwrong )
    {
	setDataUnavailableFlag( true );
	return false;
    }

    if ( zstepwrong )
	errmsg_ = tr("Z-Step is not correct. The maximum resampling "
		     "allowed is a factor 100.\nProbably the data belongs to a"
		     " different Z-Domain");
    return false;
}


bool StorageProvider::checkDesiredTrcRgOK( StepInterval<int> trcrg,
				   StepInterval<float>zrg )
{
    if ( !desiredvolume_ )
    {
	errmsg_ = tr("internal error, '%1' has no desired volume\n")
		.arg( desc_.userRef() );
	return false;
    }

    const bool trcrgwrong =
	desiredvolume_->hsamp_.start_.crl() > trcrg.stop
     || desiredvolume_->hsamp_.stop_.crl() < trcrg.start;
    const bool zwrong =
	desiredvolume_->zsamp_.start > zrg.stop
     || desiredvolume_->zsamp_.stop < zrg.start;

    if ( !trcrgwrong && !zwrong )
	return true;

    setDataUnavailableFlag( true );
    errmsg_ = tr("'%1' contains no data in selected area:\n")
		.arg( desc_.userRef() );
    if ( trcrgwrong )
	errmsg_.append( tr( "Trace range is: %1-%2\n")
		      .arg( trcrg.start ).arg( trcrg.stop ) );
    if ( zwrong )
	errmsg_.append( tr( "Z range is: %1-%2\n")
		      .arg( zrg.start ).arg( zrg.stop ) );
    return false;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples, int threadid ) const
{
    const BinID bidstep = getStepoutStep();
    const SeisTrc* trc = 0;

    if ( isondisc_)
	trc = mscprov_->get( relpos.inl()/bidstep.inl(),
			     relpos.crl()/bidstep.crl() );
    else
	trc = getTrcFromPack( relpos, 0 );

    if ( !trc || !trc->size() )
	return false;

    return fillDataHolderWithTrc( trc, output );
}


MultiID StorageProvider::getDBKey( const Desc* desc ) const
{
    const Desc* usedesc = desc ? desc : &desc_;
    const BufferString valstr(
			    usedesc->getValParam(keyStr())->getStringValue(0) );
    return MultiID( valstr.buf() );
}


DataPack::FullID StorageProvider::getDPID( const Desc* desc ) const
{
    const Desc* usedesc = desc ? desc : &desc_;
    const MultiID dbky = getDBKey( usedesc );
    return dbky.isInMemoryID() ? dbky : DataPack::FullID::udf();
}


SeisTrc* StorageProvider::getTrcFromPack( const BinID& relpos, int relidx) const
{
    const DataPack::FullID fid = getDPID();
    auto stbdtp = DPM( fid ).get<SeisTrcBufDataPack>( DataPack::getID(fid) );
    if ( !stbdtp )
	return nullptr;

    int trcidx = stbdtp->trcBuf().find(currentbid_+relpos, desc_.is2D());
    if ( trcidx+relidx >= stbdtp->trcBuf().size() || trcidx+relidx<0 )
	return nullptr;

    return stbdtp->trcBuf().get( trcidx + relidx );
}


bool StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc,
					     const DataHolder& data ) const
{
    if ( !trc || data.isEmpty() )
	return false;

    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
        if ( outputinterest_[idx] && !data.series(idx) )
	    return false;
    }

    const int z0 = data.z0_;
    float extrazfromsamppos = 0;
    BoolTypeSet isclass( outputinterest_.size(), true );
    if ( needinterp_ )
    {
	ValueSeriesInterpolator<float>& intpol =
	    const_cast<ValueSeriesInterpolator<float>&>(trc->interpolator());
	intpol.udfval_ = mUdf(float);
	checkClassType( trc, isclass );
	extrazfromsamppos = getExtraZFromSampInterval( z0, data.nrsamples_ );
	const_cast<DataHolder&>(data).extrazfromsamppos_ = extrazfromsamppos;
    }

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001f * trc->info().sampling.step );
    int compidx = -1;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] )
	    continue;

	ValueSeries<float>* series = const_cast<DataHolder&>(data).series(idx);
	const bool isclss = isclass[idx];
	compidx++;

	for ( int sampidx=0; sampidx<data.nrsamples_; sampidx++ )
	{
	    const float curt = (float)(z0+sampidx)*refstep_ + extrazfromsamppos;
	    const int compnr = desc_.is2D() ? idx : compidx;
	    const float val = trcrange.includes(curt,false) ?
		(isclss ? trc->get(trc->nearestSample(curt),compnr)
		 : trc->getValue(curt,compnr)) : mUdf(float);

	    series->setValue( sampidx, val );
	}
    }

    return true;
}


BinDataDesc StorageProvider::getOutputFormat( int output ) const
{
    if ( output>=datachar_.size() )
	return Provider::getOutputFormat( output );

    return datachar_[output];
}


BinID StorageProvider::getStepoutStep() const
{
    if ( stepoutstep_.inl() >= 0 )
	return stepoutstep_;

    BinID& sos = const_cast<StorageProvider*>(this)->stepoutstep_;
    if ( mscprov_ )
    {
	PtrMan<Seis::Bounds> bds = mscprov_->reader().getBounds();
	if ( bds )
	{
	    //Remember: in 2D BinID contains ( linenr, trcnr )
	    //while Seis::2DBounds.step contains ( trcnr, 1 )
	    sos.inl() = bds->is2D() ? bds->step( false ) : bds->step( true );
	    sos.crl() = bds->is2D() ? bds->step( true ) : bds->step( false );
	}
    }
    else
	sos.inl() = sos.crl() = 1;

    return stepoutstep_;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    if ( !isondisc_ || !mscprov_ ) return;

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return;

    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( reader.dataSet()->getRanges(geomid_,trcrg,zrg) )
    {
	storedvolume_.hsamp_.setTrcRange( trcrg );
	storedvolume_.zsamp_ = zrg;
    }
}


Pos::GeomID StorageProvider::getGeomID() const
{
    if ( geomid_.isUdf() && desc_.is2D() && mscprov_ )
	return mscprov_->reader().geomID();

    return geomid_;
}


void StorageProvider::fillDataPackWithTrc( RegularSeisDataPack* dc ) const
{
    if ( !mscprov_ ) return;
    const SeisTrc* trc = mscprov_->get(0,0);
    if ( !trc ) return;

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001f * trc->info().sampling.step );
    const BinID bid = trc->info().binID();
    if ( !dc->sampling().hsamp_.includes(bid) )
	return;

    ValueSeriesInterpolator<float>& intpol =
	const_cast<ValueSeriesInterpolator<float>&>(trc->interpolator());
    intpol.udfval_ = mUdf(float);

    const TrcKeyZSampling& sampling = dc->sampling();
    const int inlidx = sampling.hsamp_.lineRange().nearestIndex( bid.inl() );
    const int crlidx = sampling.hsamp_.trcRange().nearestIndex( bid.crl() );
    int cubeidx = -1;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] )
	    continue;

	cubeidx++;
	if ( cubeidx>=dc->nrComponents() &&
		!dc->addComponent(sKey::EmptyString()) )
	    continue;

	for ( int zidx=0; zidx<sampling.nrZ(); zidx++ )
	{
	    const float curt = sampling.zsamp_.atIndex( zidx );
	    if ( !trcrange.includes(curt,false) )
		continue;

	    //the component index inthe trace is depending on outputinterest_,
	    //thus is the same as cubeidx
	    const float val = trc->getValue( curt, cubeidx );
	    dc->data(cubeidx).set( inlidx, crlidx, zidx, val );
	}
    }
}


void StorageProvider::checkClassType( const SeisTrc* trc,
				      BoolTypeSet& isclass ) const
{
    int idx = 0;
    bool foundneed = true;
    while ( idx<trc->size() && foundneed )
    {
	foundneed = false;
	int compidx = -1;
	for ( int ido=0; ido<outputinterest_.size(); ido++ )
	{
	    if ( outputinterest_[ido] )
	    {
		compidx++;
		if ( isclass[ido] )
		{
		    foundneed = true;
		    const float val  = trc->get( idx, compidx );
		    if ( !holdsClassValue( val) )
			isclass[ido] = false;
		}
	    }
	}
	idx++;
    }
}


bool StorageProvider::compDistBetwTrcsStats( bool force )
{
    if ( !mscprov_ ) return false;
    if ( ls2ddata_ && ls2ddata_->areStatsComputed() ) return true;

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return false;

    const Seis2DDataSet* dset = reader.dataSet();
    if ( !dset ) return false;

    if ( ls2ddata_ ) delete ls2ddata_;
    ls2ddata_ = new PosInfo::LineSet2DData();
    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = ls2ddata_->addLine(dset->lineName(idx));
	const Survey::Geometry* geom =
		Survey::GM().getGeometry( dset->geomID(idx) );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geom );
	if ( !geom2d ) continue;

	linegeom = geom2d->data();
	if ( linegeom.positions().isEmpty() )
	{
	    ls2ddata_->removeLine( dset->lineName(idx) );
	    return false;
	}
    }

    ls2ddata_->compDistBetwTrcsStats();
    return true;
}


void StorageProvider::getCompNames( BufferStringSet& nms ) const
{
    updateDescAndGetCompNms( desc_, &nms );
}


bool StorageProvider::useInterTrcDist() const
{
    if ( useintertrcdist_ )
	return true;

    const Desc& dsc = getDesc();
    if ( dsc.is2D() && nrOutputs() >=2 )
    {
	if ( dsc.dataType(0) == Seis::Dip && dsc.dataType(1) == Seis::Dip )
	{
	    const_cast<Attrib::StorageProvider*>(this)->useintertrcdist_ = true;
	    return useintertrcdist_;
	}
    }

    return false;
}


float StorageProvider::getDistBetwTrcs( bool ismax, const char* linenm ) const
{
    if ( !ls2ddata_ )
	const_cast<StorageProvider*>(this)->compDistBetwTrcsStats( true );

    return ls2ddata_ ? ls2ddata_->getDistBetwTrcs( ismax, linenm )
		     : mUdf(float);
}


BinID StorageProvider::getElementStepoutStoredSpecial() const
{
    if ( !ls2ddata_ )	//huh? should never happen, on the safe side anyway
	const_cast<StorageProvider*>(this)->compDistBetwTrcsStats( true );

    return ls2ddata_
		? ls2ddata_->getElementStepout( Survey::GM().getName(geomid_) )
		: BinID(1,1);
}


} // namespace Attrib
