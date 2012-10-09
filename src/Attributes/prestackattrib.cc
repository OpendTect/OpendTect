/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : B.Bril & H.Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id$";

#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "posinfo.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "prestackgather.h"

#include "prestackprop.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "survinfo.h"

#include "ioman.h"
#include "ioobj.h"


namespace Attrib
{

mAttrDefCreateInstance(PSAttrib)
    
void PSAttrib::initClass()
{
    mAttrStartInitClass

    desc->addParam( new SeisStorageRefParam("id") );

#define mDefEnumPar(var,typ) \
    epar = new EnumParam( var##Str() ); \
    epar->addEnums( typ##Names() ); \
    desc->addParam( epar )

    EnumParam*
    mDefEnumPar(calctype,::PreStack::PropCalc::CalcType);
    mDefEnumPar(stattype,Stats::Type);
    mDefEnumPar(lsqtype,::PreStack::PropCalc::LSQType);
    mDefEnumPar(valaxis,::PreStack::PropCalc::AxisType);
    mDefEnumPar(offsaxis,::PreStack::PropCalc::AxisType);

    desc->addParam( new BoolParam( useazimStr(), false, false ) );
    IntParam* ipar = new IntParam( componentStr(), 0 , false );
    ipar->setLimits( Interval<int>(0,mUdf(int)) );
    desc->addParam( ipar );
    ipar = ipar->clone(); ipar->setKey( apertureStr() );
    desc->addParam( ipar );

    desc->addParam( new FloatParam( offStartStr(), 0, false ) );
    desc->addParam( new FloatParam( offStopStr(), mUdf(float), false ) );

    desc->addParam( new StringParam( preProcessStr(), "", false ) );

    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    desc->setUsesTrcPos( true );
    desc->setPS( true );
    mAttrEndInitClass
}


PSAttrib::PSAttrib( Desc& ds )
    : Provider(ds)
    , psrdr_(0)
    , propcalc_(0)
    , preprocessor_( 0 )
    , psioobj_( 0 )
    , component_( 0 )
{
    if ( !isOK() ) return;

    const char* res; mGetString(res,"id") psid_ = res;
    float offstart, offstop;
    mGetFloat( offstart, offStartStr() );
    mGetFloat( offstop, offStopStr() );

    setup_.offsrg_ = Interval<float>( offstart, offstop );

#define mGetSetupEnumPar(var,typ) \
    int tmp_##var = (int)setup_.var##_; \
    mGetEnum(tmp_##var,var##Str()); \
    setup_.var##_ = (typ)tmp_##var

    mGetSetupEnumPar(calctype,::PreStack::PropCalc::CalcType);
    mGetSetupEnumPar(stattype,Stats::Type);
    mGetSetupEnumPar(lsqtype,::PreStack::PropCalc::LSQType);
    mGetSetupEnumPar(valaxis,::PreStack::PropCalc::AxisType);
    mGetSetupEnumPar(offsaxis,::PreStack::PropCalc::AxisType);

    bool useazim = setup_.useazim_;
    mGetBool( useazim, useazimStr() ); setup_.useazim_ = useazim;
    mGetInt( component_, componentStr() );
    mGetInt( setup_.aperture_, apertureStr() );

    BufferString preprocessstr;
    mGetString( preprocessstr, preProcessStr() );
    preprocid_ = preprocessstr;
    PtrMan<IOObj> preprociopar = IOM().get( preprocid_ );
    if ( preprociopar )
    {
	preprocessor_ = new ::PreStack::ProcessManager;
	if ( !PreStackProcTranslator::retrieve( *preprocessor_,preprociopar,
					       errmsg_ ) )
	{
	    delete preprocessor_;
	    preprocessor_ = 0;
	}
    }

    setMyMainHackingClass( new MyChildHackingClass(this) );
}


PSAttrib::~PSAttrib()
{
    delete propcalc_;
    delete preprocessor_;

    if ( psrdr_ ) delete psrdr_;
    if ( psioobj_ ) delete psioobj_;
}


bool PSAttrib::getInputOutput( int input, TypeSet<int>& res ) const
{
    Interval<float>& rg = const_cast<Interval<float>&>(setup_.offsrg_);
    if ( rg.start > 1e28 ) rg.start = 0;
    if ( rg.stop > 1e28 ) rg.stop = mUdf(float);

    return Provider::getInputOutput( input, res );
}


bool PSAttrib::getInputData( const BinID& relpos, int zintv )
{
    if ( !psrdr_ && gatherset_.isEmpty() )
	return false;

    if ( preprocessor_ && preprocessor_->nrProcessors() )
    {
	if ( !preprocessor_->reset() || !preprocessor_->prepareWork() )
	    return false;

	if ( gatherset_.size() )
	{
	    for ( int idx=0; idx<gatherset_.size(); idx++ )
	    {
		const BinID relbid = gatherset_[idx]->getBinID();
		if ( !preprocessor_->wantsInput(relbid) )                       
		    continue;

		preprocessor_->setInput( relbid, gatherset_[idx]->id() );
	    }
	}
	else
	{
	    const BinID stepout = preprocessor_->getInputStepout();
	    const BinID stepoutstep( SI().inlRange(true).step,
				     SI().crlRange(true).step );
	    ::PreStack::Gather* gather = 0;
	    for ( int inlidx=-stepout.inl; inlidx<=stepout.inl; inlidx++ )
	    {
		for ( int crlidx=-stepout.crl; crlidx<=stepout.crl; crlidx++ )
		{
		    const BinID relbid( inlidx, crlidx );
		    if ( !preprocessor_->wantsInput(relbid) )
			continue;

		    const BinID bid = currentbid_+relpos+relbid*stepoutstep;

		    mTryAlloc( gather, ::PreStack::Gather );
		    if ( !gather )
			return false;

		    if ( !gather->readFrom(*psioobj_, *psrdr_, bid, component_))
			continue;

		    DPM(DataPackMgr::FlatID()).add( gather );
		    preprocessor_->setInput( relbid, gather->id() );
		    gather = 0;
		}
	    }
	}

	if ( !preprocessor_->process() )
	    return false;

	propcalc_->setGather( preprocessor_->getOutput() );
	return true;
    }

    const BinID bid = currentbid_+relpos;

    DataPack::ID curgatherid = -1;
    if ( gatherset_.size() )
    {
	PreStack::Gather* curgather = 0;
	for ( int idx=0; idx<gatherset_.size(); idx++ )  
	{                                       
	    //TODO full support for 2d : idx is not really my nymber of traces
	    if ( desc_.is2D() )
	    {
		if ( idx == bid.crl )
		   curgather = const_cast<PreStack::Gather*> (gatherset_[idx]);
	    }
            else if ( gatherset_[idx]->getBinID() == bid )
	       curgather = const_cast<PreStack::Gather*> (gatherset_[idx]);
	}
	if (!curgather ) return false;

	mDeclareAndTryAlloc( ::PreStack::Gather*, gather, 
				::PreStack::Gather(*curgather ) );
	if ( !gather )
	    return false;
	DPM(DataPackMgr::FlatID()).add( gather );
	curgatherid = gather->id();
    }
    else
    {
	mDeclareAndTryAlloc( ::PreStack::Gather*, gather, ::PreStack::Gather );
	if ( !gather )
	    return false;

	if ( !gather->readFrom( *psioobj_, *psrdr_, bid, component_ ) )
	    return false;

	DPM(DataPackMgr::FlatID()).add( gather );
	curgatherid = gather->id();
    }

    propcalc_->setGather( curgatherid );
    return true;
}


#define mErrRet(s1,s2,s3) { errmsg_ = BufferString(s1,s2,s3); return; }

void PSAttrib::prepPriorToBoundsCalc()
{
    delete psioobj_;

    bool isondisc = true;
    const char* fullidstr = psid_.buf();
    if ( fullidstr && *fullidstr == '#' )
    {
	DataPack::FullID fid( fullidstr+1 );
	DataPack* dtp = DPM( fid ).obtain( DataPack::getID(fid), false );
	mDynamicCastGet(PreStack::GatherSetDataPack*,psgdtp, dtp)
	isondisc =  !psgdtp;
	if ( isondisc )
	    mErrRet("Cannot obtain gathers kept in memory","" , "")

	gatherset_ = psgdtp->getGathers();
    }
    else
    {
	psioobj_ = IOM().get( psid_ );
	if ( !psioobj_ && isondisc )
	    mErrRet("Cannot find pre-stack data store ",psid_,
		    " in object manager")

	if ( desc_.is2D() )
	    psrdr_ = SPSIOPF().get2DReader( *psioobj_,
		    			    curlinekey_.lineName().buf() );
	else
	    psrdr_ = SPSIOPF().get3DReader( *psioobj_ );

	if ( !psrdr_ )
	    mErrRet("Cannot create reader for ",psid_," pre-stack data store")
	
	const char* emsg = psrdr_->errMsg();
	if ( emsg ) mErrRet("PS Reader: ",emsg,"");
    }

    mTryAlloc( propcalc_, ::PreStack::PropCalc( setup_ ) );
}


void PSAttrib::updateCSIfNeeded( CubeSampling& cs ) const
{
    if ( !psrdr_ )
	return;

    mDynamicCastGet( SeisPS3DReader*, reader3d, psrdr_ )

    if ( reader3d )
    {
	const PosInfo::CubeData& cd = reader3d->posData();
	StepInterval<int> rg;
	cd.getInlRange( rg );
	cs.hrg.setInlRange( rg );
	cd.getCrlRange( rg );
	cs.hrg.setCrlRange( rg );
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
	const float z = (z0 + idx) * refstep_ + extrazfromsamppos;
	setOutputValue( output, 0, idx, z0, propcalc_->getVal(z) );
    }

    return true;
}


void MyChildHackingClass::updateCSIfNeeded( CubeSampling& cs ) const
{
    mDynamicCastGet( PSAttrib*, psattrprov, prov_ )
    if ( psattrprov )
	psattrprov->updateCSIfNeeded( cs );
}

} // namespace Attrib
