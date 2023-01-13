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
#include "seisdatapack.h"
#include "survinfo.h"
#include "volprocchainexec.h"
#include "volproctrans.h"

using namespace Attrib;

mAttrDefCreateInstance(VolProcAttrib)

void VolProcAttrib::initClass()
{
    mAttrStartInitClass
    StringParam* setup = new StringParam( sKeySetup() );
    desc->addParam( setup );

    desc->addOutputDataType( Seis::UnknowData );
    mAttrEndInitClass
}


VolProcAttrib::VolProcAttrib( Desc& ds )
    : Provider(ds)
    , chain_(0)
    , executor_(0)
{
    const char* idstr = desc_.getValParam( sKeySetup() )->getStringValue();
    setupmid_ = MultiID( idstr );
}


VolProcAttrib::~VolProcAttrib()
{
    if ( chain_ ) chain_->unRef();
    delete executor_;
}


bool VolProcAttrib::allowParallelComputation() const
{ return false; }


void VolProcAttrib::prepareForComputeData()
{
    PtrMan<IOObj>  ioobj = IOM().get( setupmid_ );
    if ( !ioobj )
	return;

    if ( chain_ ) chain_->unRef();
    if ( executor_ ) deleteAndNullPtr( executor_ );
    chain_ = new VolProc::Chain();
    chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = uiStrings::phrCannotRead(tr("processing setup."));
	if ( !errmsg.isEmpty() )
	    errmsg_.append(tr(" Reason given: %1").arg( errmsg ) );

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



namespace VolProc
{

// ExternalAttribCalculator
void ExternalAttribCalculator::initClass()
{ Attrib::ExtAttrFact().addCreator( create, 0 ); }


Attrib::ExtAttribCalc* ExternalAttribCalculator::create(
					const Attrib::SelSpec& as )
{
    ExternalAttribCalculator* res = new ExternalAttribCalculator;
    if ( res->setTargetSelSpec( as ) )
	return res;

    delete res;
    return 0;
}


ExternalAttribCalculator::ExternalAttribCalculator()
    : chain_( 0 )
{}


ExternalAttribCalculator::~ExternalAttribCalculator()
{
    if ( chain_ ) chain_->unRef();
}


BufferString ExternalAttribCalculator::createDefinition( const MultiID& setup )
{
    BufferString res = sAttribName();
    res += " ";
    res += sKeySetup();
    res += "=";
    res += setup;

    return res;
}


bool ExternalAttribCalculator::setTargetSelSpec( const Attrib::SelSpec& ss )
{
    const char* definition = ss.defString();

    BufferString attribname;
    if ( !Attrib::Desc::getAttribName( definition, attribname ) ||
	 attribname != sAttribName() )
	return false;

    BufferString midstring;
    if ( !Attrib::Desc::getParamString( definition, sKeySetup(), midstring ) )
	return false;

    MultiID mid = midstring.buf();
    PtrMan<IOObj>  ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	errmsg_ = tr("Cannot find the processing setup.");
	return false;
    }

    chain_ = new Chain();
    chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = uiStrings::phrCannotRead(tr("processing setup.") );
	if ( !errmsg.isEmpty() )
	{
	    errmsg_.append( tr( " Reason given: %1").arg( errmsg ) );
	}

	return false;
    }

    chain_->setStorageID( mid );

    return true;
}


DataPackID
ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& tkzs,
					DataPackID dpid,
					TaskRunner* taskrunner )
{
    if ( !chain_ || !chain_->nrSteps() )
    {
	errmsg_ = tr("There are no steps in the processing chain.");
	return DataPack::cNoID();
    }

    ChainExecutor executor( *chain_ );
    od_uint64 memusage;
    if ( !executor.setCalculationScope(tkzs.hsamp_,tkzs.zsamp_,memusage) )
    {
	errmsg_ = tr("Cannot calculate at this location");
	return DataPack::cNoID();
    }

    if ( !TaskRunner::execute(taskrunner,executor) )
    {
	if ( executor.errMsg().isEmpty() )
	    errmsg_ = tr("Error while calculating.");

	return DataPack::cNoID();
    }

    ConstRefMan<RegularSeisDataPack> output = executor.getOutput();
    if ( !output || output->isEmpty() )
    {
	errmsg_ = tr("No output produced");
	return DataPack::cNoID();
    }

    //Ensure it survives the chain executor destruction
    DPM( DataPackMgr::SeisID() ).add( output );
    DPM( DataPackMgr::SeisID() ).ref( output->id() );

    return output->id();
}


DataPackID
ExternalAttribCalculator::createAttrib( const TrcKeyZSampling& tkzs,
					const LineKey& lk, TaskRunner* taskr )
{
    return createAttrib( tkzs, DataPackID(-1), taskr );
}

} // namespace VolProc
