/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    desc->addOutputDataType( Seis::UnknowData );

    desc->setLocality( Desc::SingleTrace );
    mAttrEndInitClass
}


SampleValue::SampleValue( Desc& desc )
    : Provider(desc)
{
    if ( !isOK() ) return;
}


bool SampleValue::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool SampleValue::computeData( const DataHolder& output, const BinID& relpos,
			 int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() )               
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {

	const float val = getInputValue( *inputdata_, dataidx_, idx, z0 );

	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

}; // namespace Attrib
