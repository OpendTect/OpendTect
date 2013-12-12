/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "fkfilterattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"


namespace Attrib
{

mAttrDefCreateInstance(FKFilter)

void FKFilter::initClass()
{
    mAttrStartInitClass
    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );
    mAttrEndInitClass
}


FKFilter::FKFilter( Desc& desc )
    : Provider( desc )
{
    if ( !isOK() ) return;
}


bool FKFilter::getInputData( const BinID& relpos, int zintv )
{
    inpdata_ = inputs_[0]->getData( relpos, zintv );
    return inpdata_;
}


bool FKFilter::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !inpdata_ ) return false;

    return true;
}

} // namespace Attrib
