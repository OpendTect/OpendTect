/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: madagcattrib.cc,v 1.2 2010-09-14 03:25:11 cvsnageswara Exp $";

#include "madagcattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"


namespace Attrib
{
    
mAttrDefCreateInstance(MadAGC)
    
void MadAGC::initClass()
{
    mAttrStartInitClass

    BinIDParam* stepout = new BinIDParam( smoothradiusStr() );
    stepout->setDefaultValue( BinID(0,0) );
    desc->addParam( stepout );

    desc->addParam( new IntParam( nrrepeatStr(), 1, false ) );
    desc->addParam( new IntParam( smoothzradiusStr(), 125 ) );

    desc->addInput( InputSpec("Input cube",true) );
    desc->setNrOutputs( Seis::UnknowData, 1 );

    mAttrEndInitClass
}


MadAGC::MadAGC( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    mGetInt( nrrepeat_, nrrepeatStr() );

    int zradius = 0;
    mGetInt( zradius, smoothzradiusStr() );
    dessamps_ = Interval<int>( - (zradius-1)/2, (zradius-1)/2 );

    mGetBinID( reqstepout_, smoothradiusStr() );
}


bool MadAGC::getInputData( const BinID& relpos, int zintv )
{
    const BinID bidstep = inputs_[0]->getStepoutStep();
    for ( int idi=-reqstepout_.inl; idi<=reqstepout_.inl; idi++ )
    {
	for ( int idc=-reqstepout_.crl; idc<=reqstepout_.crl; idc++ )
	{
	    const DataHolder* inputn =
		    inputs_[0]->getData( relpos+BinID(idi,idc)*bidstep, zintv );
	    if ( !inputn ) return false;
	    inputdata_ += inputn;
	}
    }
    return true;
}


const Interval<int>* MadAGC::desZSampMargin(int,int) const
{
    return &dessamps_;
}


bool MadAGC::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples, int threadid ) const
{
    //TODO: connect with Madagascar sfagc
    //setOutputValue( output, 1, idx-z0, z0, val );
    return true;
}

}; //namespace
