/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seispsioprov.cc,v 1.1 2004-12-30 17:29:35 bert Exp $";

#include "seispsioprov.h"
#include "filegen.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"

SeisPSIOProviderFactory& SPSIOPF()
{
    static SeisPSIOProviderFactory* theinst = 0;
    if ( !theinst ) theinst = new SeisPSIOProviderFactory;
    return *theinst;
}


const SeisPSIOProvider* SeisPSIOProviderFactory::provider( const char* t ) const
{
    if ( !provs_.size() )	return 0;
    else if ( !t )		return provs_[0];

    for ( int idx=0; idx<provs_.size(); idx++ )
	if ( !strcmp(t,provs_[idx]->type()) )
	    return provs_[idx];

    return 0;
}


SeisPSReader* SeisPSIOProviderFactory::getReader( const IOObj& ioobj ) const
{
    if ( !provs_.size() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.pars().find( sKey::Type ) );
    return prov ? prov->makeReader( ioobj.fullUserExpr(true) ) : 0;
}


SeisPSWriter* SeisPSIOProviderFactory::getWriter( const IOObj& ioobj ) const
{
    if ( !provs_.size() ) return 0;
    const SeisPSIOProvider* prov = provider( ioobj.pars().find( sKey::Type ) );
    return prov ? prov->makeWriter( ioobj.fullUserExpr(false) ) : 0;
}


int SeisPSTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


const IOObjContext& SeisPSTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Seis;
    }
     
    return *ctxt;
}


bool ODSeisPSTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    BufferString fnm( ioobj->fullUserExpr(true) );
    File_remove( fnm, File_isDirectory(fnm) );
    return File_exists(fnm);
}
