/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: samplevalueattrib.cc,v 1.1 2012-08-28 13:24:21 cvsbert Exp $";

#include "samplevalueattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"


namespace Attrib
{

mAttrDefCreateInstance(SampleValue)
    
void SampleValue::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    mAttrEndInitClass
}


SampleValue::SampleValue( Desc& desc )
    : Provider(desc)
{
    desc_.setLocality( Attrib::Desc::SingleTrace );
}


bool SampleValue::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    if ( !inputdata_ )
	return false;

    dataidx_ = getDataIndex( 0 );
    return true;
}


bool SampleValue::computeData( const DataHolder& output, const BinID& relpos,
			 int z0, int nrsamples, int threadid ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {

	const float val = getInputValue( *inputdata_, dataidx_, idx, z0 );

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

}; // namespace Attrib
