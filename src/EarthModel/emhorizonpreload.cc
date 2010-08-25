/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: emhorizonpreload.cc,v 1.2 2010-08-25 08:33:39 cvsnageswara Exp $";

#include "emhorizonpreload.h"

#include "bufstring.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "multiid.h"
#include "task.h"


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


bool HorizonPreLoad::loadHorizon( const MultiID& mid, TaskRunner* tr )
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


const BufferString HorizonPreLoad::name( const MultiID& mid ) const
{
    BufferString horname;
    if ( !midset_.isPresent(mid) )
	return horname;

    const int horidx = midset_.indexOf( mid );

    return horname = nameset_.get( horidx );
}


const MultiID HorizonPreLoad::getMultiID( const BufferString& horname ) const
{
    MultiID mid( -1 );
    if ( !nameset_.isPresent(horname) )
	return mid;

    const int mididx = nameset_.indexOf( horname );
    return mid = midset_[mididx];
}


bool HorizonPreLoad::unloadHorizon( const BufferString& horname )
{
    if ( !nameset_.isPresent(horname) )
	return false;

    const int selidx = nameset_.indexOf( horname );
    const MultiID mid = midset_[selidx];
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj )
	return false;

    emobj->unRef();
    midset_.remove( selidx );
    nameset_.remove( selidx );

    return true;
}


void HorizonPreLoad::surveyChgCB( CallBacker* )
{
    for ( int idx=0; idx<nameset_.size(); idx++ )
	unloadHorizon( nameset_.get(idx) );

    midset_.erase();
    nameset_.erase();
    errmsg_ = "";
}

} //namespace
