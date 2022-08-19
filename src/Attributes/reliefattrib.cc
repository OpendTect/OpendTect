/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reliefattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"


namespace Attrib
{

mAttrDefCreateInstance(Relief)

void Relief::initClass()
{
    mAttrStartInitClass

    desc->addInput( InputSpec("Input Data",true) );
    desc->setNrOutputs( Seis::UnknowData, 1 );
    desc->setLocality( Desc::SingleTrace );

    mAttrEndInitClass
}


Relief::Relief( Desc& ds )
    : Provider(ds)
{
    if ( !isOK() ) return;
}


bool Relief::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Relief::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs_[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Relief::allowParallelComputation() const
{ return true; }


bool Relief::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() || output.isEmpty() )
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float val = getInputValue( *inputdata_, dataidx_, idx, z0 );
	setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

} // namespace Attrib
