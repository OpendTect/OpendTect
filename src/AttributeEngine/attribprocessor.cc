/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribprocessor.cc,v 1.1 2005-02-03 15:35:02 kristofer Exp $";

#include "attribprocessor.h"

#include "attribdesc.h"
#include "attribprovider.h"
#include "cubesampling.h"


namespace Attrib
{

AttribProcessor::AttribProcessor( Desc& desc_ )
    : Executor("Attribute Processor")
    , desc( desc_ )
    , geometry( 0 )
    , provider( Provider::create(desc_) )
    , nriter( 0 )
{
    if ( !provider ) return;
    provider->ref();
    desc.ref();


    if ( desc.selectedOutput()!=-1 )
	enableOutput(desc.selectedOutput());
}


AttribProcessor::~AttribProcessor()
{
    desc.unRef();
    if ( provider ) provider->unRef();
}


bool AttribProcessor::isOK() const { return provider; }


void AttribProcessor::enableOutput(int outp)
{
    provider->enableOutput(outp, true );
}


bool AttribProcessor::getPossibleOutput(CubeSampling& cs) const
{ return provider->getPossibleVolume(-1,cs); }


int AttribProcessor::nextStep()
{
    if ( !nriter )
    {
	provider->setDesiredVolume( getDesiredVolume() );
    }

    const int res = provider->moveToNextTrace();
    if ( !res ) collectData();

    return res;
}


BinID AttribProcessor::currentPosition() const
{ return provider->getCurrentPosition(); }


const DataHolder* AttribProcessor::getData( const Interval<int>& zrg )
{
    //TODO: Reset range?
    provider->addLocalCompZInterval( zrg );
    return provider->getData(BinID(0,0));
}


}; //namespace
