/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/



#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribdataholder.h"
#include "cubesubsel.h"
#include "datainpspec.h"
#include "ioobj.h"
#include "linesubsel.h"
#include "linesetposinfo.h"
#include "seis2ddata.h"
#include "seisbounds.h"
#include "seisbufadapters.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seismscprov.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seisrangeseldata.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"


namespace Attrib
{

mAttrDefCreateInstance(StorageProvider)

void StorageProvider::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new SeisStorageRefParam(keyStr()) );
    desc->addOutputDataType( Seis::UnknownData );

    mAttrEndInitClass
}


void StorageProvider::updateDesc( Desc& desc )
{
    updateDescAndGetCompNms( desc, 0 );
}


void StorageProvider::updateDescAndGetCompNms( Desc& desc,
					       BufferStringSet* compnms )
{
    if ( compnms )
	compnms->erase();

    const StringPair strpair( desc.getValParam(keyStr())->getStringValue(0) );

    const BufferString storstr = strpair.first();
    if (  storstr.firstChar() == '#' )
    {
	DataPack::FullID fid = DataPack::FullID::getFromString(
							storstr.buf()+1 );
	if ( !DPM(fid).isPresent( fid ) )
	    desc.setErrMsg( uiStrings::phrCannotFind(tr("data in memory")) );
	return;
    }

    const DBKey key( storstr );
    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( key, &uirv );
    if ( !prov )
	{ desc.setErrMsg( uirv ); return; }

    BufferStringSet provcompnms; DataType dtyp;
    prov->getComponentInfo( provcompnms, &dtyp );

    if ( compnms )
	*compnms = provcompnms;

    for ( int idx=0; idx<desc.nrOutputs(); idx++ )
	desc.changeOutputDataType( idx, dtyp );

    //safety, might be removed afterwards but we still use old surveys/cubes
    if ( desc.isSteering() )
    {
	desc.changeOutputDataType( 0, Seis::Dip );
	desc.changeOutputDataType( 1, Seis::Dip );
	if ( compnms && compnms->size()>=2 )
	{
	    compnms->set( 0, new BufferString( "Inline Dip" ) );
	    compnms->set( 1, new BufferString( "Crossline Dip" ) );
	}
    }
}


StorageProvider::StorageProvider( Desc& desc )
    : Provider( desc )
    , mscprov_(0)
    , status_( None )
    , stepoutstep_(-1,0)
    , isondisk_(true)
    , useintertrcdist_(false)
    , ls2ddata_(0)
{
    storedsubsel_.setToAll( desc.is2D() );

    const StringPair strpair( desc.getValParam(keyStr())->getStringValue(0) );
    const BufferString storstr = strpair.first();
    if ( storstr.firstChar() == '#' )
    {
	DataPack::FullID fid = DataPack::FullID::getFromString(
							storstr.buf()+1 );
	isondisk_ = !DPM(fid).isPresent( fid );
    }
}


StorageProvider::~StorageProvider()
{
    if ( mscprov_ )
	delete mscprov_;
    if ( ls2ddata_ )
	delete ls2ddata_;
}


#undef mErrRet
#define mErrRet(s) { uirv_ = s; delete mscprov_; mscprov_= 0; return false; }

bool StorageProvider::checkInpAndParsAtStart()
{
    storedsubsel_.setToAll( is2D() );
    if ( status_!=None )
	return false;

    if ( !isondisk_ )
    {
	DataPack::FullID fid( getDPID() );
	auto stbdtp = DPM(fid).get<SeisTrcBufDataPack>(fid.packID());
	if ( !stbdtp || stbdtp->trcBuf().isEmpty() )
	    return false;

	SeisPacketInfo si;
	stbdtp->trcBuf().fill( si );
	const GeomID gid = stbdtp->trcBuf().isEmpty() ? GeomID::get3D()
				: stbdtp->trcBuf().first()->geomID();
	if ( is2D() )
	{
	    storedsubsel_ = FullSubSel( gid );
	    storedsubsel_.setTrcNrRange( si.crlrg );
	}
	else
	{
	    storedsubsel_.setInlRange( si.inlrg );
	    storedsubsel_.setCrlRange( si.crlrg );
	}

	storedsubsel_.setZRange( si.zrg );
	desiredsubsel_ = storedsubsel_;
    }
    else
    {
	const StringPair strpair(
			desc_.getValParam(keyStr())->getStringValue(0) );
	const DBKey dbky( strpair.first() );
	if ( !isOK() )
	    return false;

	mscprov_ = new Seis::MSCProvider( dbky );
	if ( !initMSCProvider() )
	    mErrRet( uirv_ )

	const bool is2d = mscprov_->is2D();
	desc_.setIs2D( is2d );
	mscprov_->provider().getFullSubSel( storedsubsel_ );
	status_ = StorageOpened;
    }

    return true;
}


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    if ( status_==None && isondisk_ )
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
	if ( isondisk_ )
	{
	    auto res = mscprov_ ? mscprov_->advance()
				: Seis::MSCProvider::EndReached;
	    switch ( res )
	    {
		case Seis::MSCProvider::Error:
		    { uirv_ = mscprov_->errMsg();	return -1; }
		case Seis::MSCProvider::EndReached:	return 0;
		case Seis::MSCProvider::Buffering:	continue;

		case Seis::MSCProvider::NewPosition:
		{
		    if ( useshortcuts_ )
			{ advancefurther = false; continue; }

		    SeisTrc* trc = mscprov_->curTrc();
		    if ( !trc )
			{ pErrMsg("should not happen"); continue; }

		    registerNewPosInfo( trc, startpos, firstcheck,
					advancefurther );
		}
	    }
	}
	else
	{
	    SeisTrc* trc = getTrcFromPack( BinID(0,0), 1 );
	    if ( !trc )
		return 0;

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
    currentbid_ = trcinfobid_ = newti.binID();
    if ( firstcheck || startpos == BinID(-1,-1) || currentbid_ == startpos
	    || newti.binID() == startpos )
    {
	advancefurther = false;
	curtrcinfo_ = &trc->info();
    }
}


bool StorageProvider::calcPossibleSubSel( int outp, const FullSubSel& )
{
    possiblesubsel_ = storedsubsel_;
    return status_ != None;
}


bool StorageProvider::initMSCProvider()
{
    if ( !mscprov_ || !mscprov_->errMsg().isEmpty() )
    {
	if ( !mscprov_ )
	    uirv_ = mINTERNAL( "mscprov_ null" );
	else
	    uirv_ = mscprov_->errMsg();
	return false;
    }

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


Seis::MSCProvider* StorageProvider::getMSCProvider( bool& needmscprov) const
{
    needmscprov = isondisk_;
    return mscprov_;
}


bool StorageProvider::setMSCProvSelData()
{
    if ( !mscprov_ )
	return false;

    Seis::Provider& prov = mscprov_->provider();
    if ( prov.isPS() )
	return false;

    const bool haveseldata = seldata_ && !seldata_->isAll();
    if ( haveseldata )
    {
	if ( seldata_->type() == Seis::Table )
	    return setTableSelData();
	prov.setSelData( seldata_->clone() );
    }
    else if ( !desiredSubSelOK() )
	return false;
    else
    {

	FullSubSel fss( desiredsubsel_ );
	fss.limitTo( storedsubsel_ );
	prov.setSelData( new Seis::RangeSelData(fss) );
    }

    TypeSet<int> selcomps;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
	if ( outputinterest_[idx] )
	    selcomps += idx;
    prov.selectComponents( selcomps );

    return true;
}


bool StorageProvider::setTableSelData()
{
    if ( !isondisk_ )
	return false;	//in this case we might not use a table
    if ( !mscprov_ )
	return false;

    Seis::SelData* seldata = seldata_->clone();
    Seis::Provider& prov = mscprov_->provider();
    if ( prov.is2D() && seldata->isRange() )
	seldata->asRange()->setGeomID( geomID() );

    prov.setSelData( seldata );
    mscprov_->provider().setZExtension( extraz_ );

    TypeSet<int> selcomps;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
	if ( outputinterest_[idx] )
	    selcomps += idx;
    prov.selectComponents( selcomps );

    return true;
}


void StorageProvider::checkZDisjunct( uiRetVal& uirv, const ZSubSel& posss,
					const ZSubSel& desss ) const
{
    const auto desrg = desss.outputZRange();
    const auto possrg = posss.outputZRange();
    static const auto zeps = 1.0e-6f;
    if ( desrg.start > possrg.stop+zeps || desrg.stop < possrg.start-zeps )
	uirv.add( tr("Requested position is outside the Z range (%1 to %2)")
		.arg( possrg.start ).arg( possrg.stop ) );
}


void StorageProvider::checkDisjunct( uiRetVal& uirv, const IdxSubSelData& posss,
		const IdxSubSelData& desss, const uiString& what ) const
{
    const auto desrg = desss.outputPosRange();
    const auto possrg = posss.outputPosRange();
    if ( desrg.start > possrg.stop || desrg.stop < possrg.start )
    {
	uirv.add( tr("Requested position is outside the data range: "
			"%1 range is %2 to %3" ).arg( what )
		.arg( possrg.start ).arg( possrg.stop ) );
    }
    else
    {
	const auto nrpos = desrg.nrSteps() + 1;
	bool foundone = false;
	for ( auto idx=0; idx<nrpos; idx++ )
	    if ( possrg.isPresent(desrg.atIndex(idx)) )
		{ foundone = true; break; }
	if ( !foundone )
	    uirv.add( tr("Requested %1 are not in the data grid setup ("
			"%2 to %3 step %4)" ).arg( what )
		.arg( possrg.start ).arg( possrg.stop ).arg( possrg.step ) );
    }
}


bool StorageProvider::desiredSubSelOK() const
{
#   define mCheckZDisjunct( uirv, pobj, dobj ) \
    checkZDisjunct( uirv, pobj.zSubSel(), dobj.zSubSel() )
    if ( !is2D() )
    {
	const auto& pcss = storedsubsel_.subSel3D();
	const auto& dcss = desiredsubsel_.subSel3D();
#	undef mCheckDisjunct
#	define mCheckDisjunct( subsel, uistr ) \
	checkDisjunct( uirv_, pcss.subsel(), dcss.subsel(), uiStrings::uistr );
	mCheckDisjunct( inlSubSel, sInline(mPlural) );
	mCheckDisjunct( crlSubSel, sCrossline(mPlural) );
	mCheckZDisjunct( uirv_, pcss, dcss );
    }
    else
    {
	const auto& plsss = storedsubsel_.subSel2D();
	const auto& dlsss = desiredsubsel_.subSel2D();
#	undef mCheckDisjunct
#	define mCheckDisjunct() \
	checkDisjunct( uirv, plss.trcNrSubSel(), dlss.trcNrSubSel(), \
			uiStrings::sTraceNumber(mPlural) )
	bool havecommonline = false;
	bool haveoksel = false;
	for ( auto dlssptr : dlsss )
	{
	    const auto& dlss = *dlssptr;
	    const auto* plssptr = plsss.find( dlss.geomID() );
	    if ( !plssptr )
		continue;
	    else
		havecommonline = true;
	    const auto& plss = *plssptr;

	    uiRetVal uirv;
	    mCheckDisjunct();
	    mCheckZDisjunct( uirv, plss, dlss );
	    if ( uirv.isOK() )
		haveoksel = true;
	    else
		uirv_.add( uirv );
	}
	if ( !havecommonline )
	    uirv_.set( tr("Attribute not available for requested line(s)") );
	else if ( haveoksel )
	    uirv_.setOK();
    }

    if ( !uirv_.isOK() )
	{ setDataUnavailableFlag( true ); return false; }

    return true;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples, int threadid ) const
{
    const BinID bidstep = getStepoutStep();
    const SeisTrc* trc = 0;

    if ( !isondisk_ )
	trc = getTrcFromPack( relpos, 0 );
    else if ( desc_.is2D() )
	trc = mscprov_->getAt( relpos.crl()/bidstep.crl() );
    else
	trc = mscprov_->getAt( relpos.inl()/bidstep.inl(),
			       relpos.crl()/bidstep.crl() );

    if ( !trc || !trc->size() )
	return false;

    if ( desc_.is2D() && seldata_ && seldata_->type() == Seis::Table )
    {
	const auto deszrg = desiredsubsel_.zRange();
	const auto poszrg = storedsubsel_.zRange();
	const float desonlyzrgstart = deszrg.start - poszrg.start;
	const float desonlyzrgstop = deszrg.stop - poszrg.stop;
	Interval<float> trcrange = trc->info().sampling_.interval(trc->size());
	const float diffstart = z0*refzstep_ - trcrange.start;
	const float diffstop = (z0+nrsamples-1)*refzstep_ - trcrange.stop;
	bool isdiffacceptable =
	    ( (mIsEqual(diffstart,0,refzstep_/100) || diffstart>0)
	      || diffstart >= desonlyzrgstart )
	 && ( (mIsEqual(diffstop,0,refzstep_/100) || diffstop<0 )
	      || diffstop<=desonlyzrgstop );
	if ( !isdiffacceptable )
	    return false;
    }

    return fillDataHolderWithTrc( trc, output );
}


DataPack::FullID StorageProvider::getDPID() const
{
    const StringPair strpair( desc_.getValParam(keyStr())->getStringValue(0) );

    const BufferString storstr = strpair.first();
    if ( storstr.firstChar() != '#' )
	return DataPack::FullID::getInvalid();

    DataPack::FullID fid = DataPack::FullID::getFromString( storstr.buf()+1 );
    return fid;
}


SeisTrc* StorageProvider::getTrcFromPack( const BinID& relpos, int relidx) const
{
    DataPack::FullID fid( getDPID() );
    auto stbdtp = DPM( fid ).get<SeisTrcBufDataPack>( fid.packID() );

    if ( !stbdtp )
	return 0;

    int trcidx;
    if ( desc_.is2D() )
	trcidx = stbdtp->trcBuf().find( currentbid_+relpos );
    else
	trcidx = stbdtp->trcBuf().findTrcNr(
					currentbid_.trcNr()+relpos.trcNr() );

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

    Interval<float> trcrange = trc->info().sampling_.interval(trc->size());
    trcrange.widen( 0.001f * trc->info().sampling_.step );
    int compidx = -1;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] )
	    continue;

	ValueSeries<float>* series = const_cast<DataHolder&>(data).series(idx);
	const bool isclss = isclass[idx];
	compidx++;

	const int compnr = desc_.is2D() ? idx : compidx;
	for ( int sampidx=0; sampidx<data.nrsamples_; sampidx++ )
	{
	    const float curt = (float)(z0+sampidx)*refzstep_ +extrazfromsamppos;
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
    if ( !mscprov_ )
	sos.inl() = sos.crl() = 1;
    else
    {
	const Seis::Provider& prov = mscprov_->provider();
	if ( !prov.is2D() )
	    sos = prov.as3D()->binIDStep();
	else
	{
	    const auto& prov2d = *prov.as2D();
	    const int lnr = geomID().isValid() ? prov2d.lineIdx(geomID()) : 0;
	    sos.crl() = prov2d.trcNrStep( lnr );
	}
    }

    return stepoutstep_;
}


void StorageProvider::fillDataPackWithTrc( RegularSeisDataPack* dp ) const
{
    if ( !dp || !mscprov_ )
	return;
    const SeisTrc* trc = mscprov_->curTrc();
    const auto& hss = dp->horSubSel();
    if ( !trc || !hss.includes(trc->info().trcKey()) )
	return;

    const auto inlidx = hss.idx4LineNr( trc->info().lineNr() );
    const auto crlidx = hss.idx4TrcNr( trc->info().trcNr() );
    Interval<float> trczrg = trc->info().sampling_.interval( trc->size() );
    trczrg.widen( 0.001f * trc->info().sampling_.step );

    int cubeidx = -1;
    const auto& zss = dp->zSubSel();
    const auto nrz = zss.size();
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] )
	    continue;

	cubeidx++;
	if ( cubeidx>=dp->nrComponents() &&
		!dp->addComponent(OD::EmptyString(),true) )
	    continue;

	const int compnr = desc_.is2D() ? idx : cubeidx;
	for ( int zidx=0; zidx<nrz; zidx++ )
	{
	    const float curt = zss.z4Idx( zidx );
	    if ( !trczrg.includes(curt,false) )
		continue;

	    //the component index inthe trace is depending on outputinterest_,
	    //thus is the same as cubeidx
	    const float val = trc->getValue( curt, compnr );
	    dp->data(cubeidx).set( inlidx, crlidx, zidx, val );
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
    if ( !mscprov_ )
	return false;
    if ( ls2ddata_ && ls2ddata_->areStatsComputed() )
	return true;

    const Seis::Provider& prov = mscprov_->provider();
    if ( !prov.is2D() )
	return false;
    const auto& prov2d = *prov.as2D();

    if ( ls2ddata_ )
	delete ls2ddata_;
    ls2ddata_ = new PosInfo::LineSet2DData();
    for ( int idx=0; idx<prov2d.nrLines(); idx++ )
    {
	const BufferString linenm( prov2d.lineName(idx) );
	PosInfo::Line2DData& linegeom = ls2ddata_->addLine( linenm );
	const auto& geom2d = SurvGeom::get2D( prov2d.geomID(idx) );
	if ( geom2d.isEmpty() )
	    continue;

	linegeom = geom2d.data();
	if ( linegeom.positions().isEmpty() )
	{
	    ls2ddata_->removeLine( linenm );
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
	    const_cast<StorageProvider*>(this)->useintertrcdist_ = true;
	    return useintertrcdist_;
	}
    }

    return false;
}


float StorageProvider::getDistBetwTrcs( bool ismax ) const
{
    if ( !ls2ddata_ )
	const_cast<StorageProvider*>(this)->compDistBetwTrcsStats( true );

    return ls2ddata_ ? ls2ddata_->getDistBetwTrcs( ismax, geomID().name() )
		     : mUdf(float);
}

}; // namespace Attrib
