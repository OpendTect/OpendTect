/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert & H.Huck
 * DATE     : Jan 2008
-*/


#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "cubedata.h"
#include "cubesubsel.h"
#include "datapackbase.h"
#include "prestackanglecomputer.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "prestackgather.h"
#include "prestackprop.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "survinfo.h"
#include "raytrace1d.h"
#include "windowfunction.h"

#include "ioobj.h"


namespace Attrib
{



mAttrDefCreateInstance(PSAttrib)

void PSAttrib::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new SeisStorageRefParam("id") );

#define mDefEnumPar(var,typ,defval) \
    epar = new EnumParam( var##Str() ); \
    epar->setEnums( typ##Def() ); \
    epar->setDefaultValue( defval ); \
    desc->addParam( epar )

    EnumParam*
    mDefEnumPar(calctype,PreStack::PropCalc::CalcType,1);
    mDefEnumPar(stattype,Stats::Type,Stats::Average);
    mDefEnumPar(lsqtype,PreStack::PropCalc::LSQType,0);
    mDefEnumPar(valaxis,PreStack::PropCalc::AxisType,0);
    mDefEnumPar(offsaxis,PreStack::PropCalc::AxisType,0);
    mDefEnumPar(gathertype,Gather::Type,0);
    mDefEnumPar(xaxisunit,Gather::Unit,0);

    IntParam* ipar = new IntParam( componentStr(), 0 , false );
    ipar->setLimits( Interval<int>(0,mUdf(int)) );
    desc->addParam( ipar );
    ipar = ipar->clone(); ipar->setKey( apertureStr() );
    desc->addParam( ipar );

    desc->addParam( new FloatParam( offStartStr(), 0, false ) );
    desc->addParam( new FloatParam( offStopStr(), mUdf(float), false ) );

    desc->addParam( new StringParam( preProcessStr(), "", false ) );

    desc->addParam( new StringParam( velocityIDStr(), "", false ) );

    desc->addParam( new IntParam( angleStartStr(), 0, false ) );
    desc->addParam( new IntParam( angleStopStr(), mUdf(int), false ) );
    desc->addParam( new IntParam(angleDPIDStr(),-1,true) );

    EnumParam* smoothtype = new EnumParam(angleSmoothType() );
    smoothtype->setEnums( PreStack::AngleComputer::smoothingTypeDef() );
    smoothtype->setDefaultValue( PreStack::AngleComputer::FFTFilter );
    desc->addParam( smoothtype );

    desc->addParam( new StringParam( angleFiltFunction(), "", false ) );
    desc->addParam( new FloatParam( angleFiltValue(), mUdf(float), false ) );
    desc->addParam( new FloatParam( angleFiltLength(), mUdf(float), false ) );
    desc->addParam( new FloatParam( angleFFTF3Freq(), mUdf(float), false ) );
    desc->addParam( new FloatParam( angleFFTF4Freq(), mUdf(float), false ) );
    desc->addParam( new BoolParam( useangleStr(), true, false ) );
    desc->addParam( new StringParam( rayTracerParamStr(), "", false ) );

    desc->addOutputDataType( Seis::UnknownData );

    desc->setIsSingleTrace( true );
    desc->setUsesTrcPos( true );
    desc->setIsPS( true );
    mAttrEndInitClass
}


void PSAttrib::updateDesc( Desc& desc )
{
    const BufferString procidstr
		    = desc.getValParam(preProcessStr())->getStringValue();
    const DBKey procid( procidstr );
    const bool dopreproc = !procid.isInvalid();
    desc.setParamEnabled( preProcessStr(), dopreproc );

    const int calctype = desc.getValParam( calctypeStr() )->getIntValue();
    const bool dostats = calctype == PreStack::PropCalc::Stats;
    desc.setParamEnabled( stattypeStr(), dostats );
    desc.setParamEnabled( lsqtypeStr(), !dostats );
    desc.setParamEnabled( offsaxisStr(), !dostats );

    const bool useangle = desc.getValParam(useangleStr())->getBoolValue();
    desc.setParamEnabled( gathertypeStr(), !useangle );
    desc.setParamEnabled( xaxisunitStr(), !useangle );
    desc.setParamEnabled( offStartStr(), !useangle );
    desc.setParamEnabled( offStopStr(), !useangle );

    desc.setParamEnabled( velocityIDStr(), useangle );
    desc.setParamEnabled( angleStartStr(), useangle );
    desc.setParamEnabled( angleStopStr(), useangle );
    desc.setParamEnabled( rayTracerParamStr(), useangle );
    desc.setParamEnabled( angleSmoothType(), useangle );

    const int smoothtype = desc.getValParam( angleSmoothType() )->getIntValue();
    const bool fftfilter = smoothtype == PreStack::AngleComputer::FFTFilter;
    const bool movingavg = smoothtype == PreStack::AngleComputer::MovingAverage;
    desc.setParamEnabled( angleFFTF3Freq(), useangle && fftfilter );
    desc.setParamEnabled( angleFFTF4Freq(), useangle && fftfilter );
    desc.setParamEnabled( angleFiltFunction(), useangle && movingavg );
    desc.setParamEnabled( angleFiltValue(), useangle && movingavg );
    desc.setParamEnabled( angleFiltLength(), useangle && movingavg );
}


PSAttrib::PSAttrib( Desc& ds )
    : Provider(ds)
    , psioobj_(0)
    , psrdr_(0)
    , component_(0)
    , preprocessor_(0)
    , propcalc_(0)
{
    if ( !isOK() ) return;

    const char* res;
    mGetString(res,"id")
    psid_.fromString( res );

    BufferString preprocessstr;
    mGetString( preprocessstr, preProcessStr() );
    preprocid_.fromString( preprocessstr );
    PtrMan<IOObj> preprociopar = preprocid_.getIOObj();
    if ( !preprociopar )
	preprocid_.setInvalid();
    else
    {
	preprocessor_ = new PreStack::ProcessManager;
	uiString errmsg;
	if ( !PreStackProcTranslator::retrieve( *preprocessor_,preprociopar,
					       errmsg ) )
	    { uirv_ = errmsg; delete preprocessor_; preprocessor_ = 0; }
    }

    mGetInt( component_, componentStr() );
    mGetInt( setup_.aperture_, apertureStr() );

#define mGetSetupEnumPar(var,typ) \
    int tmp_##var = (int)setup_.var##_; \
    mGetEnum(tmp_##var,var##Str()); \
    setup_.var##_ = (typ)tmp_##var

    mGetSetupEnumPar(calctype,PreStack::PropCalc::CalcType);
    if ( setup_.calctype_ == PreStack::PropCalc::Stats )
    {
	mGetSetupEnumPar(stattype,Stats::Type);
    }
    else
    {
	mGetSetupEnumPar(lsqtype,PreStack::PropCalc::LSQType);
	mGetSetupEnumPar(offsaxis,PreStack::PropCalc::AxisType);
    }

    mGetSetupEnumPar(valaxis,PreStack::PropCalc::AxisType);

    BufferString velocityidstr;
    mGetString( velocityidstr, velocityIDStr() );
    velocityid_.fromString( velocityidstr );
    mGetInt( anglegsdpid_.getI(), angleDPIDStr() );
    const bool useangle = anglegsdpid_.isValid() || velocityid_.isValid();
    if ( useangle )
    {
	int anglestart, anglestop;
	mGetInt( anglestart, angleStartStr() );
	mGetInt( anglestop, angleStopStr() );
	setup_.anglerg_.set( mCast(float,anglestart),
			     mCast(float,anglestop) );
	if ( anglegsdpid_.isValid() )
	    return;

	RefMan<PreStack::VelocityBasedAngleComputer> velangcomp =
				new PreStack::VelocityBasedAngleComputer;
	velangcomp->setDBKey( velocityid_ );
	anglecomp_ = velangcomp;
	BufferString raytracerparam;
	mGetString( raytracerparam, rayTracerParamStr() );
	IOPar raypar;
	raypar.getParsFrom( raytracerparam );
	anglecomp_->setRayTracerPars( raypar );
	setSmootheningPar();
    }
    else
    {
	float offstart, offstop;
	mGetFloat( offstart, offStartStr() );
	mGetFloat( offstop, offStopStr() );
	int gathertype = 0;
	mGetEnum( gathertype, gathertypeStr() );
	if ( gathertype == Gather::Off )
	{
	    setup_.offsrg_.set( offstart, offstop );
	    return;
	}

	if ( mIsUdf(offstop) ) offstop = 90.f;
	setup_.anglerg_ = setup_.offsrg_;
    }
}


PSAttrib::~PSAttrib()
{
    anglecomp_ = 0;
    delete propcalc_;
    delete preprocessor_;

    delete psrdr_;
    delete psioobj_;
}


void PSAttrib::setSmootheningPar()
{
    int smoothtype = 0;
    mGetEnum( smoothtype , angleSmoothType() );
    if ( smoothtype == PreStack::AngleComputer::None )
    {
	anglecomp_->setNoSmoother();
    }
    else if ( smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	BufferString smwindow;
	float windowparam, windowlength;
	mGetString( smwindow, angleFiltFunction() );
	mGetFloat( windowparam, angleFiltValue() );
	mGetFloat( windowlength, angleFiltLength() );

	if ( smwindow == CosTaperWindow::sName() )
	    anglecomp_->setMovingAverageSmoother( windowlength, smwindow,
						  windowparam );
	else
	    anglecomp_->setMovingAverageSmoother( windowlength, smwindow );
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq, f4freq;
	mGetFloat( f3freq, angleFFTF3Freq() );
	mGetFloat( f4freq, angleFFTF4Freq() );

	anglecomp_->setFFTSmoother( f3freq, f4freq );
    }
}


void PSAttrib::setAngleData( DataPack::ID angledpid )
{
    anglegsdpid_ = angledpid;
}


void PSAttrib::setAngleComp( PreStack::AngleComputer* ac )
{
    anglecomp_ = ac;
    setSmootheningPar();
}


bool PSAttrib::getInputOutput( int input, TypeSet<int>& res ) const
{
    Interval<float>& rg = const_cast<Interval<float>&>(setup_.offsrg_);
    if ( rg.start > 1e28 ) rg.start = 0;
    if ( rg.stop > 1e28 ) rg.stop = mUdf(float);

    return Provider::getInputOutput( input, res );
}


bool PSAttrib::getAngleInputData()
{
    if ( propcalc_->hasAngleData() )
	return true;
    const Gather* gather = propcalc_->getGather();
    if ( !gather || !anglecomp_ )
	return false;

    const FlatPosData& fp = gather->posData();
    anglecomp_->setOutputSampling( fp );
    if ( anglecomp_->needsTrcKey() )
	anglecomp_->setTrcKey( TrcKey(gather->getBinID()) );

    RefMan<Gather> angledata = anglecomp_->computeAngles();
    if ( !angledata )
	return false;

    propcalc_->setAngleData( *angledata.ptr() );

    return true;
}


bool PSAttrib::getGatherData( const BinID& bid,
			      RefMan<Gather>& resgather,
			      RefMan<Gather>& resanglegather )
{
    if ( gatherset_.size() )
    {
	ConstRefMan<GatherSetDataPack> anglegsdp;
	if ( anglegsdpid_.isValid() )
	    anglegsdp = DPM( DataPackMgr::SeisID() )
			    .get<GatherSetDataPack>( anglegsdpid_ );

	ConstRefMan<Gather> curgather = 0;
	ConstRefMan<Gather> curanglegather = 0;
	for ( int idx=0; idx<gatherset_.size(); idx++ )
	{
	    const bool hasanglegather =
		anglegsdp && anglegsdp->getGathers().validIdx( idx );
	    const int trcnr = idx+1;
	    //TODO full support for 2d : idx is not really my nymber of traces
	    if ( (is2D() && trcnr == bid.crl()) ||
		 (gatherset_[idx]->getBinID() == bid) )
	    {
	       curgather = gatherset_[idx];
	       if ( hasanglegather )
		   curanglegather = anglegsdp->getGathers()[idx];
	       break;
	    }
	}

	if ( !curgather ) return false;

	mDeclareAndTryAlloc( RefMan<Gather>, gather,
			     Gather(*curgather ) );

	if ( !gather )
	    return false;

	DPM(DataPackMgr::FlatID()).add( gather );
	resgather = gather;

	if ( curanglegather )
	{
	    mDeclareAndTryAlloc( RefMan<Gather>, anglegather,
				 Gather(*curanglegather ) );
	    DPM(DataPackMgr::FlatID()).add( anglegather );
	    resanglegather = anglegather;
	}
    }
    else
    {
	mDeclareAndTryAlloc( Gather*, gather, Gather );
	if ( !gather )
	    return false;

	TrcKey tk( bid );
	if ( !gather->readFrom( *psioobj_, *psrdr_, tk, component_ ) )
	    return false;

	DPM(DataPackMgr::FlatID()).add( gather );
	resgather = gather;
        resanglegather = 0;
    }

    return true;
}


RefMan<Gather> PSAttrib::getPreProcessed( const BinID& relpos )
{
    if ( !preprocessor_->reset() || !preprocessor_->prepareWork() )
	return 0;

    const BinID stepout = preprocessor_->getInputStepout();
    BinID relbid;
    RefObjectSet<Gather> tempgathers;
    const BinID sistep( SI().inlStep(), SI().crlStep() );
    for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
	  relbid.inl()++ )
    {
	for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
	      relbid.crl()++ )
	{
	    if ( !preprocessor_->wantsInput(relbid) )
		continue;

	    const BinID bid = currentbid_+relpos+relbid*sistep;
	    RefMan<Gather> gather = 0;
	    if ( gatherset_.isEmpty() )
	    {
		gather = new Gather;
		TrcKey tk( bid );
		if (!gather->readFrom(*psioobj_,*psrdr_,tk,component_) )
		    continue;

		DPM(DataPackMgr::FlatID()).add( gather );

		tempgathers += gather;
	    }
	    else
	    {
		for ( int gidx=0; gidx<gatherset_.size(); gidx++ )
		{
		    if ( bid == gatherset_[gidx]->getBinID() )
		    {
			gather = gatherset_[gidx];
			break;
		    }
		}

		if ( !gather )
		    continue;
	    }

	    preprocessor_->setInput( relbid, gather->id() );
	    gather = 0;
	}
    }

    if ( !preprocessor_->process() )
	{ uirv_ = preprocessor_->errMsg(); return 0; }

    return DPM(DataPackMgr::FlatID()).get<Gather>(preprocessor_->getOutput());
}


bool PSAttrib::getInputData( const BinID& relpos, int zintv )
{
    if ( !psrdr_ && gatherset_.isEmpty() )
	return false;

    const BinID bid = currentbid_+relpos;
    RefMan<Gather> curgather = 0;
    RefMan<Gather> curanglegather = 0;
    if ( !getGatherData(bid,curgather,curanglegather) )
	return false;

    if ( preprocessor_ && preprocessor_->nrProcessors() )
    {
	curgather = getPreProcessed( relpos );
    }

    int gathertype = 0;
    mGetEnum( gathertype, gathertypeStr() );
    int xaxisunit = 0;
    mGetEnum( xaxisunit, xaxisunitStr() );
    curgather->setIsOffsetAngle( gathertype == Gather::Ang,
				 xaxisunit == 0 ? Gather::Deg : Gather::Rad );
    propcalc_->setGather( *curgather.ptr() );
    if ( !propcalc_->hasAngleData() && anglecomp_ && !getAngleInputData() )
	return false;
    else if ( curanglegather )
	propcalc_->setAngleData( *curanglegather.ptr() );

    return true;
}

#define mErrRet(s1) { uirv_ = s1; return; }

void PSAttrib::prepPriorToBoundsCalc()
{
    delete psioobj_;

    bool isondisc = true;
    const BufferString fullidstr = psid_.toString();
    if ( fullidstr.firstChar() == '#' )
    {
	const DataPack::FullID fid = DataPack::FullID::getFromString(
							    fullidstr.str()+1 );
	auto psgdtp = DPM( fid ).get<GatherSetDataPack>( fid.packID() );

	isondisc =  !psgdtp;
	if ( isondisc )
	    mErrRet(tr("Cannot obtain gathers kept in memory"))

	gatherset_ = psgdtp->getGathers();
    }
    else
    {
	psioobj_ = psid_.getIOObj();
	if ( !psioobj_ && isondisc )
	    mErrRet( uiStrings::phrCannotFindDBEntry(psid_) )

	if ( is2D() )
	    psrdr_ = SPSIOPF().get2DReader( *psioobj_, geomID().name() );
	else
	    psrdr_ = SPSIOPF().get3DReader( *psioobj_ );

	if ( !psrdr_ )
	    mErrRet( uiStrings::phrCannotRead( psioobj_
		    ? ::toUiString(psioobj_->name())
		    : uiStrings::sSeisObjName(true,true,true) ) )

	const uiString emsg = psrdr_->errMsg();
	if ( !emsg.isEmpty() )
	    mErrRet( tr("PS Reader: %1").arg(emsg) );
    }

    propcalc_ = new PreStack::PropCalc( setup_ );
    Provider::prepPriorToBoundsCalc();
}


void PSAttrib::updateSSIfNeeded( FullSubSel& fss ) const
{
    if ( !psrdr_ )
	return;

    mDynamicCastGet( SeisPS3DReader*, reader3d, psrdr_ )
    if ( reader3d )
    {
	const PosInfo::CubeData& cd = reader3d->posData();
	StepInterval<int> rg;
	cd.getInlRange( rg );
	fss.cubeSubSel().setInlRange( rg );
	cd.getCrlRange( rg );
	fss.cubeSubSel().setCrlRange( rg );
    }

    //TODO: anything we would need to do in 2D?
}


bool PSAttrib::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !propcalc_ )
	return false;

    float extrazfromsamppos = 0;
    if ( needinterp_ )
	extrazfromsamppos = getExtraZFromSampInterval( z0, nrsamples );

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float z = (z0 + idx) * refzstep_ + extrazfromsamppos;
	setOutputValue( output, 0, idx, z0, propcalc_->getVal(z) );
    }

    return true;
}

} // namespace Attrib
