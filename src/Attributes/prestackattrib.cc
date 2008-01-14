/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : B.Bril & H.Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: prestackattrib.cc,v 1.1 2008-01-14 15:59:44 cvshelene Exp $";

#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"

namespace Attrib
{

mAttrDefCreateInstance(PreStack)
    
void PreStack::initClass()
{
    mAttrStartInitClass

    desc->addParam( new StringParam(subtypeStr()) );

    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


PreStack::PreStack( Desc& ds )
    : Provider(ds)
{
    if ( !isOK() ) return;

    mGetString( subtypestr_, subtypeStr() );
}


bool PreStack::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


//TODO: getPSData
bool PreStack::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool PreStack::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inputdata_ || inputdata_->isEmpty() || output.isEmpty() )
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float val;
	//TODO: how to get val from prestack data?
	//val = getInputValue( *inputdata_, dataidx_, idx, z0 );
	if ( isOutputEnabled(0) )
	    setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

} // namespace Attrib
