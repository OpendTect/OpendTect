/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.1 2005-02-01 16:00:43 kristofer Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "seisreq.h"
#include "seistrc.h"

namespace Attrib
{

void StorageProvider::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc, isDescOK );
    desc->ref();

    desc->addParam( new Param(keyStr(), new StringInpSpec ) );

    if ( !desc->init() )
    { desc->unRef(); return; }

    PF().addDesc( desc, createFunc );
    desc->unRef();
}


Provider* StorageProvider::createFunc( Desc& ds )
{
    StorageProvider* res = new StorageProvider( ds );
    res->ref();

    if ( !res->isOK() ) { res->unRef(); return 0; }

    res->unRefNoDelete();
    return res; 
}


bool StorageProvider::isDescOK( const Desc& ds )
{
    //TODO: Check that id exists;

    return true;
}


void StorageProvider::updateDesc( Desc& )
{
    //TODO Set nrInputs & types depending on what volume that is selecte
}


StorageProvider::StorageProvider( Desc& desc_ )
    : Provider( desc_ )
    , status( Nada )
{ }


int StorageProvider::moveToNextTrace()
{
    if ( status==Nada )
	return false;

    if ( status==StorageOpened )
    {
    }

    while ( true ) 
    {
	const int res = rg[currentreq]->next();
	if ( res==-1 ) return -1;
	if ( !res ) return 0;
	if ( res<3 )
	{
	    SeisTrc* trc = rg[currentreq]->get(0,0);
	    if ( trc )
	    {
		currentbid = trc->info().binid;
		break;
	    }
	}
    }

    return 1;
}




	

}; //namespace
