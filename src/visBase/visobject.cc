/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include "SoLockableSeparator.h"

#include <osg/Switch>

namespace visBase
{

const char* VisualObjectImpl::sKeyMaterialID()	{ return "Material ID"; }
const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


VisualObject::VisualObject( bool issel )
    : isselectable(issel)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
    , rcevinfo(0)
{}


VisualObject::~VisualObject()
{}


VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject( issel )
    , osgroot_( doOsg() ? new osg::Switch : 0 )
    , root_( new SoSeparator )
    , lockableroot_( 0 )
    , onoff_( new SoSwitch )
    , material_( 0 )
    , righthandsystem_( true )
{
    if ( osgroot_ ) osgroot_->ref();

    setMaterial( Material::create() );
    onoff_->ref();
    onoff_->addChild( root_ );
    onoff_->whichChild = SO_SWITCH_ALL;
}


VisualObjectImpl::~VisualObjectImpl()
{
    if ( osgroot_ ) osgroot_->unref();

    getInventorNode()->unref();
    if ( material_ ) material_->unRef();
}


void VisualObjectImpl::setLockable()
{
    if ( lockableroot_ )
	return;

    lockableroot_ = new SoLockableSeparator;
    lockableroot_->ref();

    for ( int idx=0; idx<root_->getNumChildren(); idx++ )
	lockableroot_->addChild( root_->getChild(idx) );

    if ( onoff_ )
    {
	onoff_->removeChild( root_ );
	onoff_->addChild( lockableroot_ );
	root_ = lockableroot_;
	lockableroot_->unref();
    }
    else
    {
	root_->unref();
	root_ = lockableroot_;
	root_->ref();
    }
}


void VisualObjectImpl::readLock()
{
    if ( lockableroot_ ) lockableroot_->lock.readLock();
}
	

void VisualObjectImpl::readUnLock()
{
    if ( lockableroot_ ) lockableroot_->lock.readUnlock();
}


bool VisualObjectImpl::tryReadLock()
{
    if ( !lockableroot_ )
	return false;

    return lockableroot_->lock.tryReadLock();
}


void VisualObjectImpl::writeLock()
{
    if ( lockableroot_ ) lockableroot_->lock.writeLock();
}
	

void VisualObjectImpl::writeUnLock()
{
    if ( lockableroot_ ) lockableroot_->lock.writeUnlock();
}


bool VisualObjectImpl::tryWriteLock()
{
    if ( !lockableroot_ )
	return false;

    return lockableroot_->lock.tryWriteLock();
}



void VisualObjectImpl::turnOn( bool yn )
{
    if ( onoff_ ) 
    {
	const int newval = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
 	if ( newval!=onoff_->whichChild.getValue() )
 	    onoff_->whichChild = newval;
    }
    else if ( !yn )
    {
	pErrMsg( "Turning off object without switch");
    }
    if ( osgroot_ )
    {
	if ( yn )
	    osgroot_->setAllChildrenOn();
	else
	    osgroot_->setAllChildrenOff();
    }
}


bool VisualObjectImpl::isOn() const
{
    if ( osgroot_ && osgroot_->getNumChildren() )
    {
	return osgroot_->getValue( 0 );
    }

    return !onoff_ || onoff_->whichChild.getValue()==SO_SWITCH_ALL;
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material_ )
    {
	root_->removeChild( material_->getInventorNode() );
	material_->unRef();
    }

    material_ = nm;

    if ( material_ )
    {
	material_->ref();
	root_->insertChild( material_->getInventorNode(), 0 );
    }
}


void VisualObjectImpl::removeSwitch()
{
    root_->ref();
    onoff_->unref();
    onoff_ = 0;
}


SoNode* VisualObjectImpl::gtInvntrNode() 
{ return onoff_ ? (SoNode*) onoff_ : (SoNode*) root_; }


osg::Node* VisualObjectImpl::gtOsgNode()
{
    return osgroot_;
}


void VisualObjectImpl::addChild( SoNode* nn )
{ root_->addChild( nn ); }


void VisualObjectImpl::insertChild( int pos, SoNode* nn )
{ root_->insertChild( nn, pos ); }


void VisualObjectImpl::removeChild( SoNode* nn )
{ root_->removeChild( nn ); }


int VisualObjectImpl::childIndex( const SoNode* nn ) const
{ return root_->findChild(nn); }


void VisualObjectImpl::addChild( osg::Node* nn )
{
    osgroot_->addChild( nn );
}


void VisualObjectImpl::insertChild( int pos, osg::Node* nn )
{
    osgroot_->insertChild( pos, nn );
}


void VisualObjectImpl::removeChild( osg::Node* nn )
{
    osgroot_->removeChild( nn );
}


int VisualObjectImpl::childIndex( const osg::Node* nn ) const
{
    const int idx = osgroot_->getChildIndex(nn);
    if ( idx==osgroot_->getNumChildren() )
	return -1;
    return idx;
}


SoNode* VisualObjectImpl::getChild(int idx)
{ return root_->getChild(idx); }


int VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar( iopar );
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get(sKeyMaterialID(),matid) )
    {
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = DM().getObject( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(Material) ) return -1;

	    setMaterial( (Material*)mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    return 1;
}


void VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( sKeyMaterialID(), material_ ? material_->id() : -1 );

    if ( material_ && !saveids.isPresent(material_->id()) )
	saveids += material_->id();

    iopar.setYN( sKeyIsOn(), isOn() );
}


void VisualObject::triggerRightClick( const EventInfo* eventinfo )
{
    rcevinfo = eventinfo;
    rightClick.trigger();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( const_cast<SoNode*>(getInventorNode()) );
    const SbBox3f bbox = action.getBoundingBox();

    if ( bbox.isEmpty() )
	return false;

    const SbVec3f min = bbox.getMin();
    const SbVec3f max = bbox.getMax();

    minpos.x = min[0]; minpos.y = min[1]; minpos.z = min[2];
    maxpos.x = max[0]; maxpos.y = max[1]; maxpos.z = max[2];

    const Transformation* trans =
	const_cast<VisualObject*>(this)->getDisplayTransformation();
    if ( trans )
    {
	minpos = trans->transformBack( minpos );
	maxpos = trans->transformBack( maxpos );
    }

    return true;
}


const TypeSet<int>* VisualObject::rightClickedPath() const
{
    return rcevinfo ? &rcevinfo->pickedobjids : 0;
}

}; //namespace
