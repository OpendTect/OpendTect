/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: emhorizonpreload.cc,v 1.3 2010-08-27 04:53:00 cvsnageswara Exp $";

#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "multiid.h"
#include "task.h"

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


bool HorizonPreLoad::load( const MultiID& mid, TaskRunner* tr )
{
    if ( midset_.isPresent(mid) )
    {
	const int idx = midset_.indexOf( mid );
	errmsg_ = "The selected horizon '";
	errmsg_.add( nameset_.get(idx) )
	       .add( "' is already pre-loaded" );

	return false;
    }

    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid, tr );
    if ( !emobj )
    {
	errmsg_ = "Problem while loading horizon";
	return false;
    }

    midset_ += mid;
    nameset_.add( emobj->name() );
    emobj->ref();

    return true;
}


const MultiID& HorizonPreLoad::getMultiID( const char* horname ) const
{
    if ( !nameset_.isPresent(horname) )
	return udfmid;

    const int mididx = nameset_.indexOf( horname );

    return midset_[mididx];
}


bool HorizonPreLoad::unload( const char* horname )
{
    if ( !nameset_.isPresent(horname) )
    {
	errmsg_ = "";
	return false;
    }

    const int selidx = nameset_.indexOf( horname );
    const MultiID mid = midset_[selidx];
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj )
    {
	errmsg_ = "Invalid ID";
	return false;
    }

    emobj->unRef();
    midset_.remove( selidx );
    nameset_.remove( selidx );

    return true;
}


void HorizonPreLoad::surveyChgCB( CallBacker* )
{
    for ( int idx=0; idx<nameset_.size(); idx++ )
	unload( nameset_.get(idx) );

    midset_.erase();
    nameset_.erase();
    errmsg_ = "";
}

} //namespace
