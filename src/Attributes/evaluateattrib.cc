/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : H. Payraudeau
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: evaluateattrib.cc,v 1.1 2005-10-19 10:41:21 cvshelene Exp $";


#include "evaluateattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "datainpspec.h"


namespace Attrib
{

void Evaluate::initClass()
{
    Desc* desc = new Desc( attribName() );
    desc->ref();

    desc->init();

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Evaluate::createInstance( Desc& desc )
{
    Evaluate* res = new Evaluate( desc );
    res->ref();

    if ( !res->isOK() )
    {
        res->unRef();
        return 0;
    }

    res->unRefNoDelete();
    return res;
}


Evaluate::Evaluate( Desc& desc )
        : Provider( desc )
{
    if ( !isOK() ) return;

    inputdata.allowNull( true );
}


bool Evaluate::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool Evaluate::getInputData( const BinID& relpos, int idx )
{
    if ( !inputdata.size() )
	inputdata += 0;

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DataHolder* data = inputs[idx]->getData( relpos, idx );

	if ( !data ) return false;
	inputdata.replace( idx, data );
	dataidx_ += getDataIndex( idx );
    }
    
    return true;
}


bool Evaluate::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples ) const
{
    if ( !inputdata.size() || !output.nrSeries() ) return false;

    for ( int idx=0; idx<output.nrSeries(); idx++ )
    {
	float* outp = output.series(idx)->arr();
	outp = inputdata[idx]->series(dataidx_[idx])->arr();
    }

    return true;
}
			
} // namespace Attrib
