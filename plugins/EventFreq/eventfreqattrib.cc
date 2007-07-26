/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jul 2007
 RCS:           $Id: eventfreqattrib.cc,v 1.1 2007-07-26 16:35:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "eventfreqattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "seistrc.h"
#include "seistrcprop.h"


namespace Attrib
{
    
mAttrDefCreateInstance(EventFreq)
    
void EventFreq::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    desc->setNrOutputs( Seis::Frequency, 1 );
    mAttrEndInitClass
}


EventFreq::EventFreq( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;
}


bool EventFreq::getInputData( const BinID& relpos, int zintv )
{
    inpdata_ = inputs[0]->getData( relpos, zintv );
    if ( !inpdata_ ) return false;
    inpseries_ = inpdata_->series( getDataIndex(0) );
    return inpseries_;
}


bool EventFreq::computeData( const DataHolder& output, const BinID& relpos, 
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inpseries_ ) return false;

    SeisTrc trc( inpdata_->nrsamples_ );
    for ( int idx=0; idx<inpdata_->nrsamples_; idx++ )
	trc.set( idx, inpseries_->value( idx ), 0 );
    SeisTrcPropCalc stpc( trc );

    Interval<int> samps( z0-inpdata_->z0_, 0 );
    samps.stop = samps.start + nrsamples - 1;
    for ( int idx=samps.start; idx<=samps.stop; idx++ )
    {
	const float freq = stpc.getFreq( idx );
	setOutputValue( output, 0, idx-samps.start, z0, freq );
    }
    return true;
}

}; //namespace
