
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: tutorialattrib.cc,v 1.1 2007-06-01 06:31:22 cvsraman Exp $
________________________________________________________________________

-*/

#include "tutorialattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"


namespace Attrib
{

mAttrDefCreateInstance(Tutorial)    
    
void Tutorial::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* action = new EnumParam( actionStr() );
    action->addEnum( "Scale" );
    action->addEnum( "Square" );
    action->addEnum( "Smooth" );
    desc->addParam( action );
    
    FloatParam* factor = new FloatParam( factorStr() );
    factor->setLimits( Interval<float>(0,mUdf(float)) );
    factor->setDefaultValue( 1 );
    desc->addParam( factor );

    FloatParam* shift = new FloatParam( shiftStr() );
    desc->addParam( shift );
    shift->setDefaultValue( 0 );

    BoolParam* smooth = new BoolParam( smoothStr() );
    desc->addParam( smooth );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );

    mAttrEndInitClass
}

    
void Tutorial::updateDesc( Desc& desc )
{
    BufferString action = desc.getValParam( actionStr() )->getStringValue();

    desc.setParamEnabled( factorStr(), action=="Scale" );
    desc.setParamEnabled( shiftStr(), action=="Scale" );
    desc.setParamEnabled( smoothStr(), action=="Smooth" );
}


Tutorial::Tutorial( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    mGetEnum( action_, actionStr() );
    mGetFloat( factor_, factorStr() );
    mGetFloat( shift_, shiftStr() );
    mGetBool ( weaksmooth_, smoothStr() );
}


bool Tutorial::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Tutorial::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Tutorial::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples, int threadid ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float outval = 0;
	if ( action_==0 || action_==1 )
	{
	    const float trcval = getInputValue( *inputdata_, dataidx_,
		    				idx, z0 );
	    outval = action_==0 ? trcval * factor_ + shift_ :
					trcval * trcval;
	}
	else if (action_==2 )
	{
	    int sgate = weaksmooth_ ? 3 : 5;
	    int sgate2 = sgate/2;
	    float sum = 0;
	    int count = 0;
	    for ( int isamp=-sgate2; isamp<=sgate2; isamp++ )
	    {
		const float curval = getInputValue( *inputdata_, dataidx_,
			                idx + isamp, z0 );
		if ( !mIsUdf(curval) )
		{
		    sum += curval;
		    count ++;
		}
	    }
	    outval = sum / count;
	}

	setOutputValue( output, 0, idx, z0, outval );
    }

    return true;
}


} // namespace Attrib
