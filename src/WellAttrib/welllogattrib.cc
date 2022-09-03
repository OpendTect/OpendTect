/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welllogattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "wellman.h"
#include "wellreader.h"

#include <math.h>


namespace Attrib
{

mAttrDefCreateInstance(WellLog)

void WellLog::initClass()
{
    mAttrStartInitClass

    desc->addParam( new StringParam(keyStr()) );
    desc->addParam( new StringParam(logName()) );
    EnumParam* scaletype = new EnumParam( upscaleType() );
    scaletype->addEnums( Stats::UpscaleTypeNames() );
    scaletype->setDefaultValue( (int)Stats::Average );
    desc->addParam( scaletype );

    desc->setNrOutputs( Seis::UnknowData, 1 );
    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


WellLog::WellLog( Desc& ds )
    : Provider(ds)
    , logvals_(0)
{
    if ( !isOK() ) return;

    logname_ = desc_.getValParam(logName())->getStringValue( 0 );
    wellid_ = MultiID( desc_.getValParam(keyStr())->getStringValue(0) );
    mGetEnum( upscaletype_, upscaleType() );
}


WellLog::~WellLog()
{
    delete logvals_;
}


bool WellLog::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool WellLog::getInputData( const BinID& relpos, int zintv )
{
    return true;
}


bool WellLog::allowParallelComputation() const
{ return true; }


void WellLog::prepareForComputeData()
{
    deleteAndZeroPtr( logvals_ );

    RefMan<Well::Data> wd = new Well::Data;
    if ( Well::MGR().isLoaded(wellid_) )
    {
	Well::LoadReqs lreqs( Well::Trck, Well::D2T, Well::LogInfos );
	lreqs.add( Well::Mrkrs );
	wd = Well::MGR().get( wellid_, lreqs );
	if ( !wd )
	{
	    errmsg_ = mToUiStringTodo( Well::MGR().errMsg() );
	    return;
	}
    }
    else
    {
	Well::Reader wrdr( wellid_, *wd );
	const bool hastrack = wrdr.getTrack();
	const bool hasd2t = wrdr.getD2T();
	const bool haslog = wrdr.getLog( logname_ );
	if ( !hastrack || ( SI().zIsTime() && !hasd2t ) || !haslog )
	{
	    errmsg_ = uiStrings::phrCannotRead( !hastrack
				    ? uiStrings::sTrack()
				    : !haslog ? uiStrings::sWellLog()
					      : tr("time-depth model") );
	    if ( !wd->name().isEmpty() )
		errmsg_.append( " for well ").append( wd->name() );

	    return;
	}
    }

    Well::ExtractParams pars;
    pars.zstep_ = SI().zStep();
    pars.extractzintime_ = SI().zIsTime();
    pars.snapZRangeToSurvey( !is2D() );
    pars.samppol_ = (Stats::UpscaleType)upscaletype_;

    BufferStringSet lognms; lognms.add( logname_ );
    Well::LogSampler logsamp( *wd, pars, lognms );
    if ( !logsamp.executeParallel(false) )
    {
	errmsg_ = logsamp.errMsg();
	return;
    }

    arrzrg_ = logsamp.zRange();
    arrzrg_.step = pars.zstep_;
    const int arrsz = arrzrg_.nrSteps()+1;
    logvals_ = new Array1DImpl<float>( arrsz );

    for ( int idx=0; idx<arrsz; idx++ )
	logvals_->set( idx, logsamp.getLogVal(0,idx) );
}


bool WellLog::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    if ( output.isEmpty() || !logvals_ )
	return false;

    const float step = !mIsZero(refstep_,mDefEps) ? refstep_ : SI().zStep();
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float realz = (z0+idx)*step;
	float val = mUdf(float);
	if ( arrzrg_.includes(realz,false) )
	{
	    const int arridx = arrzrg_.getIndex( realz );
	    val = logvals_->get( arridx );
	}

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

} // namespace Attrib
