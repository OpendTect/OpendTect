/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "ioman.h"
#include "multiid.h"
#include "ptrman.h"

namespace EM
{

HorizonPreLoader& HPreL()
{
    mDefineStaticLocalObject( PtrMan<HorizonPreLoader>, hpl,
                              (new HorizonPreLoader) );

    return *hpl;
}


HorizonPreLoader::HorizonPreLoader()
{
    mAttachCB( IOM().surveyToBeChanged, HorizonPreLoader::surveyChgCB );
}


HorizonPreLoader::~HorizonPreLoader()
{
    detachAllNotifiers();
}


bool HorizonPreLoader::load( const TypeSet<MultiID>& newmids, TaskRunner* tr )
{
    errmsg_ = "";
    if ( newmids.isEmpty() )
	return false;

    BufferString msg1( "The selected horizon(s):\n" );
    BufferString msg2;
    int nralreadyloaded = 0;
    int nrproblems = 0;
    PtrMan<ExecutorGroup> execgrp = new ExecutorGroup("Pre-loading horizons");
    ObjectSet<EM::EMObject> emobjects;
    for ( int idx=0; idx<newmids.size(); idx++ )
    {
	const int selidx = loadedmids_.indexOf( newmids[idx] );
	if ( selidx > -1 )
	{
	    msg1.add( " '" ).add( loadednms_.get(selidx) ).add( "' " );
	    nralreadyloaded++;
	    continue;
	}

	EM::ObjectID emid = EM::EMM().getObjectID( newmids[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj || !emobj->isFullyLoaded() )
	{
	    Executor* exec = EM::EMM().objectLoader( newmids[idx] );
	    if ( !exec )
	    {
		nrproblems++;
		continue;
	    }

	    execgrp->add( exec );
	}

	emid = EM::EMM().getObjectID( newmids[idx] );
	emobj = EM::EMM().getObject( emid );
	emobjects += emobj;
    }

    if ( nrproblems == newmids.size() )
    {
	if ( newmids.size() == 1 )
	    msg2 = "Could not find the horizon for pre-load\n";
	else
	    msg2 = "Could not find any horizons for pre-load\n";
    }
    else
	msg2 = "Could not pre-load some horizons\n";


    if ( nralreadyloaded > 0 )
    {
	msg1.add( " already pre-loaded" );
	errmsg_.add( msg1 );
    }

    if ( nrproblems > 0 )
	errmsg_.add( "\n" ).add( msg2 );

    if ( execgrp->nrExecutors()!=0 &&  !TaskRunner::execute( tr, *execgrp) )
	return false;

    for ( int idx=0; idx<emobjects.size(); idx++ )
    {
	loadedmids_ += emobjects[idx]->multiID();
	loadednms_.add( emobjects[idx]->name() );
	emobjects[idx]->ref();
    }

    return true;
}


const TypeSet<MultiID>& HorizonPreLoader::getPreloadedIDs() const
{ return loadedmids_; }

const BufferStringSet& HorizonPreLoader::getPreloadedNames() const
{ return loadednms_; }


const MultiID& HorizonPreLoader::getMultiID( const char* horname ) const
{
    const int loadedhorid = loadednms_.indexOf( horname );
    return loadedhorid < 0 ? MultiID::udf() : loadedmids_[loadedhorid];
}


void HorizonPreLoader::unload( const BufferStringSet& hornames )
{
    if ( hornames.isEmpty() )
	return;

    errmsg_ = "";
    for ( int hidx=0; hidx<hornames.size(); hidx++ )
    {
	const int selidx = loadednms_.indexOf( hornames.get(hidx) );
	if ( selidx < 0 )
	    continue;

	const MultiID mid = loadedmids_[selidx];
	EM::ObjectID emid = EM::EMM().getObjectID( mid );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( emobj )
	    emobj->unRef();

	loadedmids_.removeSingle( selidx );
	loadednms_.removeSingle( selidx );
    }
}


void HorizonPreLoader::surveyChgCB( CallBacker* )
{
    unload( loadednms_ );
    loadedmids_.erase();
    loadednms_.erase();
    errmsg_ = "";
}

} // namespace EM
