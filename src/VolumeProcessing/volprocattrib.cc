/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

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

    if ( !TaskRunner::execute( tr, executor ) )
    {
	if ( executor.errMsg() )
	    errmsg_ = executor.errMsg();
	else
	    errmsg_ = "Error while calculating.";
    }

    ConstRefMan<Attrib::DataCubes> datacubes = executor.getOutput();
    if ( !datacubes->nrCubes() )
    {
	errmsg_ = "No output produced";
	return DataPack::cNoID();
    }

    const Attrib::DescID did = Attrib::SelSpec::cOtherAttrib();
    Attrib::Flat3DDataPack* ndp =
	new Attrib::Flat3DDataPack( did, *datacubes, 0 );


    CubeSampling::Dir dir;
    int slice;

    if ( cs.nrInl()<2 )
    {
	dir = CubeSampling::Inl;
	slice = datacubes->inlsampling_.nearestIndex( cs.hrg.start.inl );
    }
    else if ( cs.nrCrl()<2 )
    {
	dir = CubeSampling::Crl;
	slice = datacubes->crlsampling_.nearestIndex( cs.hrg.start.crl );
    }
    else
    {
	dir = CubeSampling::Z;
	slice = mNINT32( cs.zrg.start/datacubes->zstep_ )-datacubes->z0_;
    }

    if ( !ndp->setDataDir( dir ) || !ndp->setDataSlice( slice ) )
    {
	delete ndp;
	return DataPack::cNoID();
    }

    DPM( DataPackMgr::FlatID() ).add( ndp );

    PtrMan<IOObj> ioobj = IOM().get( chain_->storageID() );
    if ( ioobj ) ndp->setName( ioobj->name() );

    return ndp->id();
}


}; //namespace
