/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/


#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "dbman.h"
#include "dbkey.h"
#include "ptrman.h"


EM::HorizonPreLoader& EM::HPreL()
{
    mDefineStaticLocalObject( PtrMan<HorizonPreLoader>, hpl,
                              (new HorizonPreLoader) );

    return *hpl;
}


EM::HorizonPreLoader::HorizonPreLoader()
{
    DBM().surveyToBeChanged.notify( mCB(this,HorizonPreLoader,surveyChgCB) );
}


EM::HorizonPreLoader::~HorizonPreLoader()
{
    DBM().surveyToBeChanged.remove( mCB(this,HorizonPreLoader,surveyChgCB) );
}


bool EM::HorizonPreLoader::load( const DBKeySet& newids, bool is2d,
				 TaskRunner* tskr )
{
    errmsg_ = uiString::emptyString();
    if ( newids.isEmpty() )
	return false;

    EM::ObjectManager& horman = is2d ? EM::Hor2DMan() : EM::Hor3DMan();
    uiString msg1( tr("The selected horizons:") );
    uiString msg2;
    int nralreadyloaded = 0;
    int nrproblems = 0;
    PtrMan<ExecutorGroup> execgrp = new ExecutorGroup("Pre-loading horizons");
    ObjectSet<EM::Object> emobjects;
    for ( int idx=0; idx<newids.size(); idx++ )
    {
	const int selidx = loadedids_.indexOf( newids[idx] );
	if ( selidx > -1 )
	{
	    msg1.appendPlainText( " '%1'", uiString::Empty,
			uiString::LeaveALine ).arg( loadednms_.get(selidx) );
	    nralreadyloaded++;
	    continue;
	}

	EM::Object* emobj = horman.getObject( newids[idx] );
	if ( !emobj || !emobj->isFullyLoaded() )
	{
	    Executor* exec = horman.objectLoader( newids[idx] );
	    if ( !exec )
	    {
		nrproblems++;
		continue;
	    }

	    execgrp->add( exec );
	}

	emobj = horman.getObject( newids[idx] );
	emobjects += emobj;
    }

    if ( nrproblems == newids.size() )
    {
	if ( newids.size() == 1 )
	    msg2 = tr( "Cannot find the horizon for pre-load" );
	else
	    msg2 = tr( "Cannot find any horizons for pre-load" );
    }
    else
	msg2 = tr("Cannot pre-load some horizons");


    if ( nralreadyloaded > 0 )
    {
	msg1.appendAfterList( tr("are already pre-loaded") );
	errmsg_.appendPhrase( msg1 );
    }

    if ( nrproblems > 0 )
	errmsg_.appendPhrase( msg2 );

    if ( execgrp->nrExecutors()!=0 &&  !TaskRunner::execute( tskr, *execgrp) )
	return false;

    for ( int idx=0; idx<emobjects.size(); idx++ )
    {
	loadedids_ += emobjects[idx]->dbKey();
	loadednms_.add( emobjects[idx]->name() );
	emobjects[idx]->ref();
    }

    return true;
}


const DBKeySet& EM::HorizonPreLoader::getPreloadedIDs() const
{
    return loadedids_;
}

const BufferStringSet& EM::HorizonPreLoader::getPreloadedNames() const
{
    return loadednms_;
}


DBKey EM::HorizonPreLoader::getDBKey( const char* horname ) const
{
    const int ididx = loadednms_.indexOf( horname );
    return ididx < 0 ? DBKey::getInvalid() : loadedids_[ididx];
}


void EM::HorizonPreLoader::unload( const BufferStringSet& hornames )
{
    if ( hornames.isEmpty() )
	return;

    errmsg_ = uiString::emptyString();
    for ( int hidx=0; hidx<hornames.size(); hidx++ )
    {
	const int selidx = loadednms_.indexOf( hornames.get(hidx) );
	if ( selidx < 0 )
	    continue;

	const DBKey id = loadedids_[selidx];
	EM::Object* emobj = EM::Hor3DMan().getObject( loadedids_[selidx] );
	if ( !emobj )
	    emobj = EM::Hor2DMan().getObject( loadedids_[selidx] );

	if ( emobj )
	    emobj->unRef();

	loadedids_.removeSingle( selidx );
	loadednms_.removeSingle( selidx );
    }
}


void EM::HorizonPreLoader::surveyChgCB( CallBacker* )
{
    unload( loadednms_ );
    loadedids_.erase();
    loadednms_.erase();
    errmsg_ = uiString::emptyString();
}
