/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2006
 RCS:           $Id: gapdeconattrib.cc,v 1.3 2006-08-15 07:54:56 cvshelene Exp $
________________________________________________________________________

-*/

#include "gapdeconattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"

namespace Attrib
{
    
mAttrDefCreateInstance(GapDecon)
    
void GapDecon::initClass()
{
    mAttrStartInitClass
	
    IntParam* lagsize = new IntParam( lagsizeStr() );
    desc->addParam( lagsize );
		
    IntParam* gapsize = new IntParam( gapsizeStr() );
    desc->addParam( gapsize );
    
    IntParam* noiselevel = new IntParam( noiselevelStr() );
    desc->addParam( noiselevel );

    IntParam* nrtrcs = new IntParam( nrtrcsStr() );
    desc->addParam( nrtrcs );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-mLargestZGate,mLargestZGate) );
    desc->addParam( gate );
    
    BoolParam* isinputzerophase = new BoolParam( isinp0phaseStr() );
    isinputzerophase->setDefaultValue( true );
    desc->addParam( isinputzerophase );

    BoolParam* isoutputzerophase = new BoolParam( isout0phaseStr() );
    isoutputzerophase->setDefaultValue( true );
    desc->addParam( isoutputzerophase );

    desc->addInput( InputSpec("Input data",true) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    mAttrEndInitClass
}


GapDecon::GapDecon( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetBool( isinpzerophase_, isinp0phaseStr() );
    mGetBool( isoutzerophase_, isout0phaseStr() );
    mGetInt( lagsize_, lagsizeStr() );
    mGetInt( gapsize_, gapsizeStr() );
    int nrtrcs;
    mGetInt( nrtrcs, nrtrcsStr() );
    stepout_ = BinID( nrtrcs/2, nrtrcs/2 );

    mGetInt( noiselevel_, noiselevelStr() );
}


bool GapDecon::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool GapDecon::getInputData( const BinID& relpos, int zintv )
{
    //rework
    /*
    while ( inputdata_.size() < trcpos_.size() )
	inputdata_ += 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos_.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs[0]->getData( relpos+trcpos_[idx]*bidstep, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
    }
    
    dataidx_ = getDataIndex( 0 );

    steeringdata_ = dosteer_ ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;
*/
    return true;
}


bool GapDecon::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples ) const
{
    if ( !inputdata_.size() ) return false;

    //implement
    return true;
}


const BinID* GapDecon::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }

}; //namespace
