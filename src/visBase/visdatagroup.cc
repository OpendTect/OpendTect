/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

#include <Inventor/nodes/SoSeparator.h>

#include <osg/Group>

mCreateFactoryEntry( visBase::DataObjectGroup );

namespace visBase
{

const char* DataObjectGroup::nokidsstr()	{ return "Number of Children"; }
const char* DataObjectGroup::kidprefix()	{ return "Child "; }

DataObjectGroup::DataObjectGroup()
    : group_ ( 0 )
    , osggroup_( 0 )
    , separate_( true )
    , change( this )
    , righthandsystem_( true )
{ }


DataObjectGroup::~DataObjectGroup()
{
    deepUnRef( objects_ );

    if ( group_ ) group_->unref();
    if ( osggroup_ ) osggroup_->unref();
}


void DataObjectGroup::ensureGroup()
{
    if ( doOsg() && !osggroup_ )
    {
	osggroup_ = new osg::Group;
	osggroup_->ref();
	updateOsgNodeData();
    }

    if ( group_ ) return;
    group_ = createGroup();
    group_->ref();
}


SoGroup* DataObjectGroup::createGroup()
{
    return separate_ ? new SoSeparator : new SoGroup;
}


int DataObjectGroup::size() const
{ return objects_.size(); }


void DataObjectGroup::addObject( DataObject* no )
{
    ensureGroup();
    objects_ += no;
    group_->addChild( no->getInventorNode() );
    nodes_ += no->getInventorNode();

    if ( osggroup_ && no->osgNode() ) osggroup_->addChild( no->osgNode() );

    no->ref();
    no->setRightHandSystem( isRightHandSystem() );
    change.trigger();
}


void DataObjectGroup::setDisplayTransformation( const mVisTrans* nt )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setDisplayTransformation(nt);
}


const mVisTrans* DataObjectGroup::getDisplayTransformation() const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	if ( objects_[idx]->getDisplayTransformation() )
	    return objects_[idx]->getDisplayTransformation();

    return 0;
}


void DataObjectGroup::setRightHandSystem( bool yn )
{
    righthandsystem_ = yn;

    yn = isRightHandSystem();

    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setRightHandSystem( yn );
}


bool DataObjectGroup::isRightHandSystem() const
{ return righthandsystem_; }


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

    objects_.insertAt( no, insertpos );
    nodes_.insertAt(no->getInventorNode(), insertpos );
    ensureGroup();
    group_->insertChild( no->getInventorNode(), insertpos );
    if ( osggroup_ && no->osgNode() ) osggroup_->insertChild( insertpos, no->osgNode() );
    no->ref();
    no->setRightHandSystem( isRightHandSystem() );
    change.trigger();
}


int DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj ) return -1;

    return getFirstIdx( sceneobj );
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects_.indexOf(sceneobj); }


void DataObjectGroup::removeObject( int idx )
{
    DataObject* sceneobject = objects_[idx];
    SoNode* node = nodes_[idx];
    group_->removeChild( node );
    if ( osggroup_ ) osggroup_->removeChild( sceneobject->osgNode() );

    nodes_.remove( idx );
    objects_.remove( idx );

    sceneobject->unRef();
    change.trigger();
}


void DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


SoNode*  DataObjectGroup::gtInvntrNode()
{
    ensureGroup();
    return group_;
}


osg::Node* DataObjectGroup::gtOsgNode()
{
    ensureGroup();
    return osggroup_;
}


void DataObjectGroup::fillPar( IOPar& par, TypeSet<int>& saveids)const
{
    DataObject::fillPar( par, saveids );
    
    par.set( nokidsstr(), objects_.size() );
    
    BufferString key;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	key = kidprefix();
	key += idx;

	int saveid = objects_[idx]->id();
	if ( saveids.indexOf( saveid )==-1 ) saveids += saveid;

	par.set( key, saveid );
    }
}


int DataObjectGroup::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int nrkids;
    if ( !par.get( nokidsstr(), nrkids ) )
	return -1;

    BufferString key;
    TypeSet<int> ids;
    for ( int idx=0; idx<nrkids; idx++ )
    {
	key = kidprefix();
	key += idx;

	int newid;
	if ( !par.get( key, newid ) )
	    return -1;

	if ( !DM().getObject( newid ) )
	{
	    res = 0;
	    continue;
	}

	ids += newid;
    }

    for ( int idx=0; idx<ids.size(); idx++ )
	addObject( ids[idx] );

    return res;
}

}; // namespace visBase
