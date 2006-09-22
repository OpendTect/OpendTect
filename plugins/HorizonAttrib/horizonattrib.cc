/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horizonattrib.cc,v 1.1 2006-09-22 09:21:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "horizonattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "ptrman.h"


namespace Attrib
{

mAttrDefCreateInstance(Horizon)
    
void Horizon::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new StringParam(sKeyHorID()) );
    desc->addParam( new StringParam(sKeyFileName()) );

    desc->addInput( InputSpec("Input data for Horizon",true) );
    desc->setNrOutputs( Seis::UnknowData, 2 );

    mAttrEndInitClass
}


void Horizon::updateDesc( Desc& desc )
{
}


Horizon::Horizon( Desc& desc_ )
    : Provider( desc_ )
    , inputdata_( 0 )
{ 
    if ( !isOK() ) return;
}


bool Horizon::getInputData( const BinID& relpos, int zintv )
{
    inputdata_ = inputs[0]->getData( relpos, zintv );
    dataidx_ = getDataIndex( 0 );
    return inputdata_;
}


bool Horizon::computeData( const DataHolder& output, const BinID& relpos,
			   int z0, int nrsamples ) const
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
    }
}


} // namespace Attrib
