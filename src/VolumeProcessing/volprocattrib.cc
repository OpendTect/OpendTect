/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID = "$Id: volprocattrib.cc,v 1.10 2010-05-10 16:10:45 cvskris Exp $";

#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "volprocchain.h"
#include "volproctrans.h"

namespace VolProc
{
/*
mAttrDefCreateInstance( AttributeAdapter );

void AttributeAdapter::initClass()
{
    mAttrStartInitClass;

    Attrib::StringParam* renderparam = 
	new Attrib::StringParam( sKeyRenderSetup() );
    desc->addParam( renderparam );

    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass;
}


AttributeAdapter::AttributeAdapter( Attrib::Desc& d )
    : Attrib::Provider( d )
    , chain_( 0 )
    , firstlocation_( true )
    , executor_( 0 )
{
    BufferString rendersetup;
    mGetString( rendersetup, sKeyRenderSetup() );
    rendermid_ = rendersetup;

    PtrMan<IOObj> ioobj = IOM().get( rendermid_ );
    chain_ = new Chain();
    chain_->ref();
    if ( !VolProcessingTranslator::retrieve( *chain_, ioobj, errmsg ) )
    {
	chain_->unRef();
	chain_ = 0;
    }
} 


AttributeAdapter::~AttributeAdapter()
{
    delete executor_;
}


bool AttributeAdapter::isOK() const
{
    return chain_ && Attrib::Provider::isOK();
}


bool AttributeAdapter::computeData( const Attrib::DataHolder& output,
				    const BinID& relpos, int t0,
				    int nrsamples, int threadid ) const
{
    if ( !executor_->setCalculationScope( getCurrentPosition() +
			     relpos * getStepoutStep(),
			     StepInterval<int>( t0, t0+nrsamples-1, 1 ),
	   		     *output.series(0) ) )
	return false;


    return executor_->execute();
}


void AttributeAdapter::prepareForComputeData()
{
    Provider::prepareForComputeData();
    if ( firstlocation_ )
    {
	firstlocation_ = false;
	prepareChain();
    }
}


bool AttributeAdapter::prepareChain()
{
    chain_->setZSampling( SamplingData<float>( 0, getRefStep() ),zIsTime() );
    if ( !executor_ ) executor_ = new ChainExecutor( *chain_ );
    return true;
}

*/


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
	 strcmp(attribname.buf(), sAttribName()) )
	return false;
    
    BufferString midstring;
    if ( !Attrib::Desc::getParamString( definition, sKeySetup(), midstring ) )
	return false;

    MultiID mid = midstring.buf();
    PtrMan<IOObj>  ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	errmsg_ = "Cannot find the processing setup.";
	return false;
    }

    chain_ = new Chain();
    chain_->ref();
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_, ioobj, errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	errmsg_ = "Cannot read processing setup.";
	if ( !errmsg.isEmpty() )
	{
	    errmsg_ += " Reason given: ";
	    errmsg_ += errmsg;
	}   

	return false;
    }

    chain_->setStorageID( mid );

    return true;
}


DataPack::ID ExternalAttribCalculator::createAttrib( const CubeSampling& cs,
						     DataPack::ID dpid,
       						     TaskRunner* tr )
{
    if ( !chain_ || !chain_->nrSteps() )
    {
	errmsg_ = "There are no steps in the processing chain.";
	return DataPack::cNoID();
    }

    ChainExecutor executor( *chain_ );
    if ( !executor.setCalculationScope(cs) ) 
    {
	errmsg_ = "Cannot calculate at this location";
	return DataPack::cNoID();
    }

    if ( (tr && !tr->execute(executor)) || (!tr && !executor.execute() ) )
    {
	if ( executor.errMsg() )
	    errmsg_ = executor.errMsg();
	else
	    errmsg_ = "Error while calculating.";
    }

    RefMan<const Attrib::DataCubes> datacubes = executor.getOutput();
    if ( !datacubes->nrCubes() )
    {
	errmsg_ = "No output produced";
	return DataPack::cNoID();
    }

    const Attrib::DescID did = Attrib::SelSpec::cOtherAttrib();
    Attrib::Flat3DDataPack* ndp =
	new Attrib::Flat3DDataPack( did, *datacubes, 0 );
    DPM( DataPackMgr::FlatID() ).add( ndp );

    PtrMan<IOObj> ioobj = IOM().get( chain_->storageID() );
    if ( ioobj ) ndp->setName( ioobj->name() );

    return ndp->id();
}


}; //namespace
