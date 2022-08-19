/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackeventtransl.h"

#include "prestackeventio.h"
#include "prestackevents.h"
#include "ioman.h"
#include "uistrings.h"

defineTranslatorGroup(PSEvent, "PreStack Event" );
defineTranslator(dgb,PSEvent,mDGBKey);
mDefSimpleTranslatorSelector(PSEvent);

uiString PSEventTranslatorGroup::sTypeName(int)
{ return uiStrings::sPreStackEvents(); }

const IOObjContext& PSEventTranslatorGroup::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, ctxt, = 0 );
    if ( !ctxt )
    {
	IOObjContext* newctxt = new IOObjContext( 0 );
	newctxt->stdseltype_ = IOObjContext::Surf;

	ctxt.setIfNull(newctxt,true);
    }

    ctxt->trgroup_ = &theInst();
    return *ctxt;
}



Executor* PSEventTranslator::reader( PreStack::EventManager& pse,
       const BinIDValueSet* bvs, const TrcKeySampling* hs, IOObj* ioobj,
       bool trigger )
{
    mDynamicCast( PSEventTranslator*, PtrMan<PSEventTranslator> trans,
		  ioobj->createTranslator() );
    if ( !trans ) { return 0; }

    return trans->createReader( pse, bvs, hs, ioobj, trigger );
}


Executor* PSEventTranslator::writer( PreStack::EventManager& pse, IOObj* ioobj )
{
    mDynamicCast( PSEventTranslator*, PtrMan<PSEventTranslator> trans,
		 ioobj->createTranslator() );
    if ( !trans ) { return 0; }

    return trans->createWriter( pse, ioobj );
}


Executor* PSEventTranslator::writeAs( PreStack::EventManager& pse, IOObj* ioobj)
{
    mDynamicCast( PSEventTranslator*, PtrMan<PSEventTranslator> trans,
		 ioobj->createTranslator() );
    if ( !trans ) { return 0; }

    return trans->createSaveAs( pse, ioobj );
}


Executor* dgbPSEventTranslator::createReader( PreStack::EventManager& pse,
	const BinIDValueSet* bvs, const TrcKeySampling* hs, IOObj* ioobj,
        bool trigger )
{
    mDynamicCast( PSEventTranslator*, PtrMan<PSEventTranslator> trans,
		     ioobj->createTranslator() );
    if ( !trans ) { return 0; }

    PreStack::EventReader* res = new PreStack::EventReader(ioobj,&pse,trigger);
    res->setSelection( bvs );
    res->setSelection( hs );
    return res;
}


Executor* dgbPSEventTranslator::createWriter( PreStack::EventManager& pse,
					      IOObj* ioobj)
{
    mDynamicCast( PSEventTranslator*, PtrMan<PSEventTranslator> trans,
		 ioobj->createTranslator() );
    if ( !trans ) { return 0; }
    return new PreStack::EventWriter( ioobj, pse );
}


Executor* dgbPSEventTranslator::createSaveAs( PreStack::EventManager& pse,
					      IOObj* newstorage )
{
    if ( !newstorage ) return 0;

    PtrMan<IOObj> oldstorage = IOM().get( pse.getStorageID() );

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
