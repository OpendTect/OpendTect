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
#include "emobjectio.h"
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
				 const TaskRunnerProvider& trprov )
{
    errmsg_ = uiString::empty();
    if ( newids.isEmpty() )
	return false;

    EM::ObjectManager& horman = is2d ? EM::Hor2DMan() : EM::Hor3DMan();
    uiString msg1( tr("The selected horizons:") );
    uiString msg2;
    int nralreadyloaded = 0;
    DBKeySet tobeloadedkeys;
    for ( int idx=0; idx<newids.size(); idx++ )
    {
	const int selidx = loadedids_.indexOf( newids[idx] );
	if ( selidx < 0 )
	    tobeloadedkeys += newids[idx];
	else
	{
	    msg1.appendPlainText( " '%1'", uiString::NoSep,
		    uiString::AfterEmptyLine ).arg( loadednms_.get(selidx) );
	    nralreadyloaded++;
	}
    }

    if ( nralreadyloaded > 0 )
    {
	msg1.appendAfterList( tr("are already pre-loaded") );
	errmsg_.appendPhrase( msg1 );
    }

    PtrMan<EM::ObjectLoader> loader = horman.objectLoader( tobeloadedkeys );
    if ( !loader->load(trprov) )
    {
	const DBKeySet& notloadedkeys = loader->notLoadedKeys();
	if ( notloadedkeys.size() == newids.size() )
	{
	    if ( newids.size() == 1 )
		msg2 = tr("Cannot find the horizon for pre-load");
	    else
		msg2 = tr("Cannot find any horizons for pre-load");
	}
	else
	    msg2 = tr("Some of the horizons could not be preloaded");

	errmsg_.appendPhrase( msg2 );
    }

    RefObjectSet<EM::Object> loadedobjs = loader->getLoadedEMObjects();
    for ( int idx=0; idx<loadedobjs.size(); idx++ )
    {
	loadedids_ += loadedobjs[idx]->dbKey();
	loadednms_.add( loadedobjs[idx]->name() );
	loadedobjs[idx]->ref();
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

    errmsg_ = uiString::empty();
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
    errmsg_ = uiString::empty();
}
