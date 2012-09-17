/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribfactory.cc,v 1.13 2011/01/06 15:25:01 cvsbert Exp $";

#include "attribfactory.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "msgh.h"

namespace Attrib
{

ProviderFactory::ProviderFactory()
{}


ProviderFactory::~ProviderFactory()
{
    for ( int idx=0; idx<descs_.size(); idx++ )
	descs_[idx]->unRef();

    descs_.erase();
    creaters_.erase();
}


void ProviderFactory::addDesc( Desc* nps, ProviderCreater pc )
{
    const int idx = indexOf(nps->attribName());
    if ( idx!=-1 )
	return;

    nps->ref();
    descs_ += nps;
    creaters_ += pc;
};


Provider* ProviderFactory::create( Desc& desc ) const
{
    if ( desc.isSatisfied()>=2 )
	return 0;

    const int idx = indexOf(desc.attribName());
    if ( idx==-1 ) return 0;

    return creaters_[idx]( desc );
}


const Desc* ProviderFactory::getDesc( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : descs_[idx];
}


Desc* ProviderFactory::createDescCopy( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : new Desc( *descs_[idx] );
}


int ProviderFactory::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<descs_.size(); idx++ )
    {
	if ( !strcmp( descs_[idx]->attribName(), nm ) )
	    return idx;
    }

    return -1;
}


void ProviderFactory::updateAllDescsDefaults()
{
    for ( int idx=0; idx<descs_.size(); idx++ )
	descs_[idx]->updateDefaultParams();
}


ProviderFactory& PF()
{
    static PtrMan<ProviderFactory> factory = new ProviderFactory();
    return *factory;
}

}; //namespace

