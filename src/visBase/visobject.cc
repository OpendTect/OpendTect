/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visobject.h"

#include "iopar.h"
#include "keystrs.h"
#include "visevent.h"
#include "vismaterial.h"

#include <osg/Group>
#include <osg/BlendFunc>

namespace visBase
{

const char* VisualObjectImpl::sKeyMaterialID()	{ return "Material ID"; }
const char* VisualObjectImpl::sKeyMaterial()    { return "Material"; }
const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


VisualObject::VisualObject( bool issel )
    : isselectable(issel)
    , selnotifier(this)
    , deselnotifier(this)
    , rightClick(this)
    , rcevinfo(0)
{}


VisualObject::~VisualObject()
{}


VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject( issel )
    , material_( 0 )
    , righthandsystem_( true )
    , osgroot_( new osg::Group )
{
    setOsgNode( osgroot_ );
}


VisualObjectImpl::~VisualObjectImpl()
{
    detachAllNotifiers();
    if ( material_ )
	material_->unRef();
}


void VisualObjectImpl::setLockable()
{
}


void VisualObjectImpl::readLock()
{
}


void VisualObjectImpl::readUnLock()
{
}


bool VisualObjectImpl::tryReadLock()
{
    return false;
}


void VisualObjectImpl::writeLock()
{
}


void VisualObjectImpl::writeUnLock()
{
}


bool VisualObjectImpl::tryWriteLock()
{
    return false;
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    osg::StateSet* ss = osgroot_->getOrCreateStateSet();

    if ( material_ )
    {
	removeNodeState( material_ );
	mDetachCB( material_->change, VisualObjectImpl::materialChangeCB );
	material_->unRef();
    }

    material_ = nm;

    if ( material_ )
    {
	material_->ref();
	mAttachCB( material_->change, VisualObjectImpl::materialChangeCB );
	ss->setDataVariance( osg::Object::DYNAMIC );
	addNodeState( material_ );
    }
}


void VisualObjectImpl::materialChangeCB( CallBacker* )
{
    osg::StateSet* ss = osgroot_->getOrCreateStateSet();

    const bool transparent = getMaterial()->getTransparency() > 0.0;

    if ( transparent && ss->getRenderingHint()!=osg::StateSet::TRANSPARENT_BIN )
    {
	osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
	blendFunc->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	ss->setAttributeAndModes( blendFunc );
	ss->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    }

    if ( !transparent && ss->getRenderingHint()!=osg::StateSet::OPAQUE_BIN )
    {
	ss->removeAttribute( osg::StateAttribute::BLENDFUNC );
	ss->setRenderingHint( osg::StateSet::OPAQUE_BIN );
    }
}


Material* VisualObjectImpl::getMaterial()
{
    if ( !material_ )
	setMaterial( new visBase::Material );

    return material_;
}



NotifierAccess* VisualObjectImpl::materialChange()
{
    return getMaterial() ? &getMaterial()->change : nullptr;
}


void VisualObjectImpl::setGroupNode( osg::Group* grpnode )
{
    if ( !grpnode || osgroot_ == grpnode )
	return;

    osg::ref_ptr<osg::Group> newgroup = grpnode;

    for ( int idx=0; idx<osgroot_->getNumChildren(); idx++ )
	grpnode->addChild( osgroot_->getChild(idx) );

    if ( osgroot_->getStateSet() )
    grpnode->setStateSet( osgroot_->getStateSet() );

    setOsgNode( grpnode );
    osgroot_ = grpnode;
}


void VisualObjectImpl::setGroupNode( DataObject* dobj )
{
    if ( !dobj )
	return;

    osg::Node* node = dobj->osgNode();
    mDynamicCastGet(osg::Group*,grp,node)
    setGroupNode( grp );
}


int VisualObjectImpl::addChild( osg::Node* nn )
{
    if ( !nn )
	return -1;

    const int res = osgroot_->addChild( nn );
    requestSingleRedraw();
    return res;
}


void VisualObjectImpl::insertChild( int pos, osg::Node* nn )
{
    osgroot_->insertChild( pos, nn );
    requestSingleRedraw();
}


void VisualObjectImpl::removeChild( osg::Node* nn )
{
    osgroot_->removeChild( nn );
    requestSingleRedraw();
}


int VisualObjectImpl::childIndex( const osg::Node* nn ) const
{
    const int idx = osgroot_->getChildIndex(nn);
    if ( idx==osgroot_->getNumChildren() )
	return -1;
    return idx;
}


bool VisualObjectImpl::usePar( const IOPar& iopar )
{
    if ( material_ )
    {
	PtrMan<IOPar> matpar = iopar.subselect( sKeyMaterial() );
	if ( matpar )
	    material_->usePar( *matpar );
    }

    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    const BufferString nm = iopar.find( sKey::Name() );

    if ( !nm.isEmpty() )
	setName( nm );

    return true;
}


void VisualObjectImpl::fillPar( IOPar& iopar ) const
{
    if ( material_ )
    {
	IOPar materialpar;
	material_->fillPar( materialpar );
	iopar.mergeComp( materialpar, sKeyMaterial() );

    }

    iopar.setYN( sKeyIsOn(), isOn() );

  /*  const StringView nm = name();
    if ( !nm.isEmpty() )
	iopar.set( sKey::Name(),nm );*/

}


void VisualObject::triggerRightClick( const EventInfo* eventinfo )
{
    rcevinfo = eventinfo;
    rightClick.trigger();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    pErrMsg( "Not impl. Not sure if needed." );
    return false;
    /*
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
     */
}


const TypeSet<VisID>* VisualObject::rightClickedPath() const
{
    return rcevinfo ? &rcevinfo->pickedobjids : 0;
}

} // namespace visBase
