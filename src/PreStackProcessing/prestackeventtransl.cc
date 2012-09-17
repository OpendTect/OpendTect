/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: prestackeventtransl.cc,v 1.4 2010/08/11 14:50:45 cvsbert Exp $";

#include "prestackeventtransl.h"

#include "prestackeventio.h"
#include "prestackevents.h"
#include "ioman.h"

defineTranslatorGroup(PSEvent, PSEventTranslatorGroup::sKeyword());
defineTranslator(dgb,PSEvent,mDGBKey);

mDefSimpleTranslatorSelector(PSEvent,sKeyword());

const IOObjContext& PSEventTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    if ( !ctxt )
    {
	ctxt = new IOObjContext( 0 );
	ctxt->stdseltype = IOObjContext::Surf;
	ctxt->toselect.allowtransls_ = mDGBKey;
    }

    ctxt->trgroup = &theInst();
    return *ctxt;
}



Executor* PSEventTranslator::reader( PreStack::EventManager& pse,
       const BinIDValueSet* bvs, const HorSampling* hs, IOObj* ioobj,
       bool trigger )
{
    mDynamicCastGet( PSEventTranslator*, t, ioobj->getTranslator() );
    if ( !t ) { return 0; }

    PtrMan<PSEventTranslator> trans = t;
    return t->createReader( pse, bvs, hs, ioobj, trigger );
}


Executor* PSEventTranslator::writer( PreStack::EventManager& pse, IOObj* ioobj )
{
    mDynamicCastGet( PSEventTranslator*, t, ioobj->getTranslator() );
    if ( !t ) { return 0; }

    PtrMan<PSEventTranslator> trans = t;
    return t->createWriter( pse, ioobj );
}


Executor* PSEventTranslator::writeAs( PreStack::EventManager& pse, IOObj* ioobj )
{
    mDynamicCastGet( PSEventTranslator*, t, ioobj->getTranslator() );
    if ( !t ) { return 0; }

    PtrMan<PSEventTranslator> trans = t;
    return t->createSaveAs( pse, ioobj );
}


Executor* dgbPSEventTranslator::createReader( PreStack::EventManager& pse,
	const BinIDValueSet* bvs, const HorSampling* hs, IOObj* ioobj,
        bool trigger )
{
    mDynamicCastGet( PSEventTranslator*, t, ioobj->getTranslator() );
    if ( !t ) { return 0; }
    PtrMan<PSEventTranslator> trans = t;

    PreStack::EventReader* res = new PreStack::EventReader(ioobj,&pse,trigger);
    res->setSelection( bvs );
    res->setSelection( hs );
    return res;
}


Executor* dgbPSEventTranslator::createWriter( PreStack::EventManager& pse,
					      IOObj* ioobj)
{
    mDynamicCastGet( PSEventTranslator*, t, ioobj->getTranslator() );
    if ( !t ) { return 0; }
    PtrMan<PSEventTranslator> trans = t;

    return new PreStack::EventWriter( ioobj, pse );
}


Executor* dgbPSEventTranslator::createSaveAs( PreStack::EventManager& pse,
					      IOObj* newstorage )
{
    mDynamicCastGet( PSEventTranslator*, t, newstorage->getTranslator() );
    if ( !t ) { return 0; }
    PtrMan<PSEventTranslator> trans = t;

    PtrMan<IOObj> oldstorage = IOM().get( pse.getStorageID() );

    if ( !newstorage ) return 0;

    ExecutorGroup* grp = new ExecutorGroup( "Save as", false );

    if ( oldstorage )
    {
	grp->add( new PreStack::EventDuplicator( oldstorage->clone(),
						 newstorage->clone() ) );
    }

    Executor* setstorexec = pse.setStorageID( newstorage->key(), false );
    if ( setstorexec ) grp->add( setstorexec );

    grp->add( new PreStack::EventWriter( newstorage, pse ) );
    return grp;
}
