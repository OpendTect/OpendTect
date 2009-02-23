/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribfactory.cc,v 1.8 2009-02-23 05:59:11 cvsranojay Exp $";

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
    for ( int idx=0; idx<descs.size(); idx++ )
	descs[idx]->unRef();

    descs.erase();
    creaters.erase();
}


void ProviderFactory::addDesc( Desc* nps, ProviderCreater pc )
{
    const int idx = indexOf(nps->attribName());
    if ( idx!=-1 )
	return;

    nps->ref();
    descs += nps;
    creaters += pc;
};


Provider* ProviderFactory::create( Desc& desc ) const
{
    if ( desc.isSatisfied()>=2 )
	return 0;

    const int idx = indexOf(desc.attribName());
    if ( idx==-1 ) return 0;

    return creaters[idx]( desc );
}


Desc* ProviderFactory::createDescCopy( const char* nm ) const
{
    const int idx = indexOf(nm);
    if ( idx==-1 ) return 0;

    return new Desc( *descs[idx] );
}


int ProviderFactory::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<descs.size(); idx++ )
    {
	if ( !strcmp( descs[idx]->attribName(), nm ) )
	    return idx;
    }

    return -1;
}


ProviderFactory& PF()
{
    static ProviderFactory& factory = *new ProviderFactory();
    return factory;
}

}; //namespace

