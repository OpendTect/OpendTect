/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2002
 RCS:           $Id: visdatagroup.cc,v 1.5 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

#include <Inventor/nodes/SoSeparator.h>

mCreateFactoryEntry( visBase::DataObjectGroup );

namespace visBase
{

const char* DataObjectGroup::nokidsstr = "Number of Children";
const char* DataObjectGroup::kidprefix = "Child ";

DataObjectGroup::DataObjectGroup()
    : group ( new SoGroup )
    , separator( new SoSeparator )
    , separate( true )
{
    separator->ref();
    separator->addChild( group );
}


DataObjectGroup::~DataObjectGroup()
{
    const int sz = size();

    for ( int idx=0; idx<sz; idx++ )
	objects[idx]->unRef();

    separator->unref();
}


int DataObjectGroup::size() const
{ return objects.size(); }


void DataObjectGroup::addObject( DataObject* no )
{
    objects += no;
    group->addChild( no->getInventorNode() );
    nodes += no->getInventorNode();

    no->ref();
}


void DataObjectGroup::setDisplayTransformation( Transformation* nt )
{
    for ( int idx=0; idx<objects.size(); idx++ )
	objects[idx]->setDisplayTransformation(nt);
}


Transformation* DataObjectGroup::getDisplayTransformation()
{
    for ( int idx=0; idx<objects.size(); idx++ )
	if ( objects[idx]->getDisplayTransformation() )
	    return objects[idx]->getDisplayTransformation();

    return 0;
}


void DataObjectGroup::addObject(int nid)
{
    DataObject* no =
	dynamic_cast<DataObject*>( DM().getObject(nid) );

    if ( !no ) return;
    addObject( no );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() ) return addObject( no );

    objects.insertAt( no, insertpos );
    nodes.insertAt(no->getInventorNode(), insertpos );
    group->insertChild( no->getInventorNode(), insertpos );
    no->ref();
}


int DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj ) return -1;

    return objects.indexOf(sceneobj);
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects.indexOf(sceneobj); }


void DataObjectGroup::removeObject( int idx )
{
    DataObject* sceneobject = objects[idx];
    SoNode* node = nodes[idx];
    group->removeChild( node );

    nodes.remove( idx );
    objects.remove( idx );

    sceneobject->unRef();
}


void DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


SoNode*  DataObjectGroup::getInventorNode()
{ return separate ? (SoNode*) separator : (SoNode*) group; }


void DataObjectGroup::fillPar( IOPar& par, TypeSet<int>& saveids)const
{
    DataObject::fillPar( par, saveids );
    
    par.set( nokidsstr, objects.size() );
    
    BufferString key;
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	key = kidprefix;
	key += idx;

	int saveid = objects[idx]->id();
	if ( saveids.indexOf( saveid )==-1 ) saveids += saveid;

	par.set( key, saveid );
    }
}


int DataObjectGroup::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int nrkids;
    if ( !par.get( nokidsstr, nrkids ) )
	return -1;

    BufferString key;
    TypeSet<int> ids;
    for ( int idx=0; idx<nrkids; idx++ )
    {
	key = kidprefix;
	key += idx;

	int newid;
	if ( !par.get( key, newid ) )
	    return -1;

	if ( !DM().getObject( newid ) )
	    return 0;

	ids += newid;
    }

    for ( int idx=0; idx<nrkids; idx++ )
	addObject( ids[idx] );

    return 1;
}

}; // namespace visBase
