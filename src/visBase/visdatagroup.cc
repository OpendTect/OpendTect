
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visdatagroup.cc,v 1.1 2004-01-05 09:43:23 kristofer Exp $";

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

const char* visBase::DataObjectGroup::nokidsstr = "Number of Children";
const char* visBase::DataObjectGroup::kidprefix = "Child ";

#include "Inventor/nodes/SoSeparator.h"

mCreateFactoryEntry( visBase::DataObjectGroup );

visBase::DataObjectGroup::DataObjectGroup()
    : group ( new SoGroup )
    , separator( new SoSeparator )
    , separate( true )
{
    separator->ref();
    separator->addChild( group );
}


visBase::DataObjectGroup::~DataObjectGroup()
{
    const int sz = size();

    for ( int idx=0; idx<sz; idx++ )
	objects[idx]->unRef();

    separator->unref();
}


int visBase::DataObjectGroup::size() const
{ return objects.size(); }


void visBase::DataObjectGroup::addObject( DataObject* no )
{
    objects += no;
    group->addChild( no->getInventorNode() );
    nodes += no->getInventorNode();

    no->ref();
}


void visBase::DataObjectGroup::setTransformation( Transformation* nt )
{
    for ( int idx=0; idx<objects.size(); idx++ )
	objects[idx]->setTransformation(nt);
}


visBase::Transformation* visBase::DataObjectGroup::getTransformation()
{
    for ( int idx=0; idx<objects.size(); idx++ )
	if ( objects[idx]->getTransformation() )
	    return objects[idx]->getTransformation();

    return 0;
}


void visBase::DataObjectGroup::addObject(int nid)
{
    DataObject* no =
	dynamic_cast<DataObject*>( visBase::DM().getObj(nid) );

    if ( !no ) return;
    addObject( no );
}


void visBase::DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() ) return addObject( no );

    objects.insertAt( no, insertpos );
    nodes.insertAt(no->getInventorNode(), insertpos );
    group->insertChild( no->getInventorNode(), insertpos );
    no->ref();
}


int visBase::DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) visBase::DM().getObj(nid);

    if ( !sceneobj ) return -1;

    return objects.indexOf(sceneobj);
}


int visBase::DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects.indexOf(sceneobj); }


void visBase::DataObjectGroup::removeObject( int idx )
{
    DataObject* sceneobject = objects[idx];
    SoNode* node = nodes[idx];
    group->removeChild( node );

    nodes.remove( idx );
    objects.remove( idx );

    sceneobject->unRef();
}


void visBase::DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


SoNode*  visBase::DataObjectGroup::getInventorNode()
{ return separate ? (SoNode*) separator : (SoNode*) group; }


void visBase::DataObjectGroup::fillPar( IOPar& par, TypeSet<int>& saveids)const
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


int visBase::DataObjectGroup::usePar( const IOPar& par )
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

	int id;
	if ( !par.get( key, id ) )
	    return -1;

	if ( !DM().getObj( id ) )
	    return 0;

	ids += id;
    }

    for ( int idx=0; idx<nrkids; idx++ )
	addObject( ids[idx] );

    return 1;
}
