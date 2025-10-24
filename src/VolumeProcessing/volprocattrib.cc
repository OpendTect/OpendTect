/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "volprocchainexec.h"
#include "volproctrans.h"

// Attrib::VolProcAttrib

using namespace Attrib;

mAttrDefCreateInstance(VolProcAttrib)

void VolProcAttrib::initClass()
{
    mAttrStartInitClass
    auto* setup = new StringParam( sKeySetup() );
    desc->addParam( setup );

    desc->addOutputDataType( Seis::UnknowData );
    mAttrEndInitClass
}


VolProcAttrib::VolProcAttrib( Desc& ds )
    : Provider(ds)
{
    setupmid_ = getDesc().getValParam( sKeySetup() )->getMultiID();
}


VolProcAttrib::~VolProcAttrib()
{
    delete executor_;
}


bool VolProcAttrib::allowParallelComputation() const
{
    return false;
}


void VolProcAttrib::prepareForComputeData()
{
    PtrMan<IOObj>  ioobj = IOM().get( setupmid_ );
    if ( !ioobj )
	return;

    deleteAndNullPtr( executor_ );
    chain_ = new VolProc::Chain();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj.ptr(),errmsg) )
    {
	chain_ = nullptr;
	errmsg_ = uiStrings::phrCannotRead( tr("processing setup") );
	if ( !errmsg.isEmpty() )
	    errmsg_.appendPhraseSameLine( tr("Reason given: %1").arg( errmsg ));

	return;
    }

    chain_->setStorageID( setupmid_ );

    executor_ = new VolProc::ChainExecutor( *chain_ );
    const TrcKeyZSampling tkzs( *getDesiredVolume() );
    od_uint64 memusage;
    if ( !executor_->setCalculationScope(tkzs.hsamp_,tkzs.zsamp_,memusage) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return;
    }

    if ( !executor_->execute() )
    {
	errmsg_ = executor_->errMsg().isEmpty()
		? tr("Error while calculating.")
		: executor_->errMsg();
    }
}


bool VolProcAttrib::computeData( const Attrib::DataHolder& output,
				 const BinID& relpos, int z0, int nrsamples,
				 int threadid ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float outval = mUdf(float);
	setOutputValue( output, 0, idx, z0, outval );
    }

    return true;
}


// VolProc::ExternalAttribCalculator

namespace VolProc
{

ExternalAttribCalculator::ExternalAttribCalculator()
{}


ExternalAttribCalculator::~ExternalAttribCalculator()
{
}


bool ExternalAttribCalculator::setTargetSelSpec( const Attrib::SelSpec& as )
{
    if ( !checkSelSpec(as) )
	return false;

    const char* definition = as.defString();
    BufferString midstring;
    if ( !Attrib::Desc::getParamString(definition,sKeySetup(),midstring) )
	return false;

    MultiID mid;
    mid.fromString( midstring.buf() );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	errmsg_ = tr("Cannot find the processing setup.");
	return false;
    }

    chain_ = new Chain();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj.ptr(),errmsg) )
    {
	chain_ = nullptr;
	errmsg_ = uiStrings::phrCannotRead( tr("processing setup") );
	if ( !errmsg.isEmpty() )
	    errmsg_.appendPhraseSameLine( tr("Reason given: %1").arg( errmsg ));

	return false;
    }

    chain_->setStorageID( mid );

    return true;
}


ConstRefMan<RegularSeisDataPack>
ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& tkzs,
					const RegularSeisDataPack* /*cachedp*/,
					TaskRunner* taskrunner )
{
    if ( !chain_ || !chain_->nrSteps() )
    {
	errmsg_ = tr("There are no steps in the processing chain.");
	return nullptr;
    }

    ChainExecutor executor( *chain_ );
    od_uint64 memusage;
    if ( !executor.setCalculationScope(tkzs.hsamp_,tkzs.zsamp_,memusage) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return nullptr;
    }

    if ( !TaskRunner::execute(taskrunner,executor) )
    {
	if ( executor.errMsg().isEmpty() )
	    errmsg_ = tr("Error while calculating.");

	return nullptr;
    }

    ConstRefMan<RegularSeisDataPack> output = executor.getOutput();
    if ( !output || output->isEmpty() )
    {
	errmsg_ = tr("No output produced");
	return nullptr;
    }

    return output;
}


ConstRefMan<RegularSeisDataPack>
ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& tkzs,
					TaskRunner* taskrun )
{
    return createAttrib( tkzs, nullptr, taskrun );
}


bool ExternalAttribCalculator::createAttrib( ObjectSet<BinIDValueSet>& o,
					     TaskRunner* taskrun )
{
    return Attrib::ExtAttribCalc::createAttrib( o, taskrun );
}


bool ExternalAttribCalculator::createAttrib( const BinIDValueSet& b,
					     SeisTrcBuf& tb,
					     TaskRunner* taskrun )
{
    return Attrib::ExtAttribCalc::createAttrib( b, tb, taskrun );
}


ConstRefMan<RandomSeisDataPack>
 ExternalAttribCalculator::createRdmTrcAttrib( const ZGate& zrg,
				const RandomLineID& rdlid, TaskRunner* taskrun )
{
    //TODO impl?
    return Attrib::ExtAttribCalc::createRdmTrcAttrib( zrg, rdlid, taskrun );
}


BufferString ExternalAttribCalculator::createDefinition( const MultiID& setup )
{
    BufferString res = sFactoryKeyword();
    res.addSpace().add( sKeySetup() ).add( "=" ).add( setup );

    return res;
}


uiString ExternalAttribCalculator::createDisplayName( const MultiID& setup )
{
    PtrMan<IOObj> ioobj = IOM().get( setup );
    if ( !ioobj )
	return uiString::empty();

    return ioobj->uiName();
}


bool ExternalAttribCalculator::sCheckSelSpec( const Attrib::SelSpec& spec )
{
    return Attrib::ExtAttribCalc::sCheckSelSpec( spec, sFactoryKeyword() );
}

} // namespace VolProc
