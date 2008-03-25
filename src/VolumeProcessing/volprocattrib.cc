/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID = "$Id: volprocattrib.cc,v 1.2 2008-03-25 16:50:12 cvsnanne Exp $";

#include "volprocattrib.h"

#include "attribdesc.h"
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
	return false;

    chain_ = new Chain();
    chain_->ref();
    BufferString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_, ioobj, errmsg) )
    {
	chain_->unRef();
	chain_ = 0;
	return false;
    }

    return true;
}


DataPack::ID ExternalAttribCalculator::createAttrib(
			const CubeSampling& cs, DataPack::ID dpid )
{
    return DataPack::cNoID;
}


const Attrib::DataCubes*
ExternalAttribCalculator::createAttrib( const CubeSampling& cs,
					const Attrib::DataCubes* dc )
{
    if ( !chain_ || !chain_->nrSteps() )
	return 0;

    chain_->setZSampling( SamplingData<float>( cs.zrg ), SI().zIsTime() );
    Attrib::DataCubes* datacubes = new Attrib::DataCubes::DataCubes();
    if ( !datacubes->setSizeAndPos(cs) )
    {
	datacubes->unRef();
	return 0;
    }

    ChainExecutor executor( *chain_ );
    if ( !executor.setCalculationScope(datacubes) || !executor.execute() )
    {
	datacubes->unRef();
	return 0;
    }	

    return datacubes;
}


bool ExternalAttribCalculator::createAttrib(ObjectSet<BinIDValueSet>&)
{
    return false;
}


bool ExternalAttribCalculator::createAttrib(const BinIDValueSet&, SeisTrcBuf&)
{
    return false;
}


DataPack::ID ExternalAttribCalculator::createAttrib( const CubeSampling&,
						     const LineKey&)
{ return DataPack::cNoID; }


bool ExternalAttribCalculator::isIndexes() const
{ return false; }


}; //namespace
