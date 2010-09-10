/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: emhorizonpreload.cc,v 1.5 2010-09-10 07:26:56 cvsnageswara Exp $";

#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "ioman.h"
#include "multiid.h"
#include "ptrman.h"

static const MultiID udfmid( "-1" );

namespace EM
{

HorizonPreLoad& HPreL()
{
    static PtrMan<HorizonPreLoad> hpl = 0;
    if ( !hpl )
	hpl = new HorizonPreLoad;

    return *hpl;
}


HorizonPreLoad::HorizonPreLoad()
{
    IOM().surveyToBeChanged.notify( mCB(this,HorizonPreLoad,surveyChgCB) );
}


bool HorizonPreLoad::load( const TypeSet<MultiID>& midset, TaskRunner* tr )
{
    errmsg_ = "";
    if ( midset.isEmpty() )
	return false;

    BufferString msg1( "The selected horizon(s):\n" );
    BufferString msg2( "Could not pre-load:\n" );
    int nralreadyloaded = 0;
    int nrproblems = 0;
    PtrMan<ExecutorGroup> execgrp = new ExecutorGroup( "Pre-loading horizons" );
    for ( int idx=0; idx<midset.size(); idx++ )
    {
	const int selidx = midset_.indexOf( midset[idx] );
	if ( selidx > -1 )
	{
	    msg1.add( " '" ).add( nameset_.get(selidx) ).add( "' " );
	    nralreadyloaded++;
	    continue;
	}

	EM::ObjectID emid = EM::EMM().getObjectID( midset[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj || !emobj->isFullyLoaded() )
	{
	    Executor* exec = EM::EMM().objectLoader( midset[idx] );
	    if ( !exec )
	    {
		BufferString name( EM::EMM().objectName(midset[idx]) );
		msg2.add( " '" ).add( name ).add( "' " );
		nrproblems++;
		continue;
	    }

	    execgrp->add( exec );
	    emid = EM::EMM().getObjectID( midset[idx] );
	    emobj = EM::EMM().getObject( emid );
	}

	midset_ += midset[idx];
	nameset_.add( emobj->name() );
	emobj->ref();
    }

    if ( nralreadyloaded > 0 )
    {
	msg1.add( " already pre-loaded" );
	errmsg_.add( msg1 );
    }

    if ( nrproblems > 0 )
	errmsg_.add( "\n" ).add( msg2 );

    if ( execgrp->nrExecutors() != 0 && !tr->execute( *execgrp ) )
	return false;

    return true;
}


const MultiID& HorizonPreLoad::getMultiID( const char* horname ) const
{
    if ( !nameset_.isPresent(horname) )
	return udfmid;

    const int mididx = nameset_.indexOf( horname );

    return midset_[mididx];
}


bool HorizonPreLoad::unload( const BufferStringSet& hornames )
{
    if ( hornames.isEmpty() )
	return false;

    errmsg_ = "";
    int invalidids = 0;
    for ( int hidx=0; hidx<hornames.size(); hidx++ )
    {
	const int selidx = nameset_.indexOf( hornames.get(hidx) );
	if ( selidx < 0 )
	    continue;

	const MultiID mid = midset_[selidx];
	EM::ObjectID emid = EM::EMM().getObjectID( mid );
	EM::EMObject* emobj = EM::EMM().getObject( emid );
	if ( !emobj )
	{
	    invalidids++;
	    errmsg_ = "Found Invalid ID(s)";
	    continue;
	}

	emobj->unRef();
	midset_.remove( selidx );
	nameset_.remove( selidx );
    }

    if ( invalidids > 0 )
	return false;

    return true;
}


void HorizonPreLoad::surveyChgCB( CallBacker* )
{
    unload( nameset_ );
    midset_.erase();
    nameset_.erase();
    errmsg_ = "";
}

} //namespace
