/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.27 2004-04-27 12:09:19 kristofer Exp $";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

const char* visBase::VisualObjectImpl::materialidstr = "Material ID";
const char* visBase::VisualObjectImpl::isonstr = "Is on";

visBase::VisualObject::VisualObject( bool selectable_ )
    : isselectable(selectable_)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
{}


visBase::VisualObject::~VisualObject()
{ }


visBase::VisualObjectImpl::VisualObjectImpl( bool selectable_ )
    : VisualObject(selectable_)
    , root(new SoSeparator)
    , onoff(new SoSwitch)
    , material(0)
{
    setMaterial( visBase::Material::create() );
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;
}


visBase::VisualObjectImpl::~VisualObjectImpl()
{
    onoff->unref();
    if ( material ) material->unRef();
}


void visBase::VisualObjectImpl::turnOn( bool yn )
{
    onoff->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::VisualObjectImpl::isOn() const
{
    return !onoff->whichChild.getValue();
}


void visBase::VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material )
    {
	root->removeChild( material->getInventorNode() );
	material->unRef();
    }

    material = nm;

    if ( material )
    {
	material->ref();
	root->insertChild( material->getInventorNode(), 0 );
    }
}


SoNode* visBase::VisualObjectImpl::getInventorNode() 
{ return onoff; }


void visBase::VisualObjectImpl::addChild( SoNode* nn )
{ root->addChild( nn ); }


void visBase::VisualObjectImpl::insertChild( int pos, SoNode* nn )
{ root->insertChild( nn, pos ); }


void visBase::VisualObjectImpl::removeChild( SoNode* nn )
{ root->removeChild( nn ); }


bool visBase::VisualObjectImpl::isChild( const SoNode* nn ) const
{ return root->findChild(nn)!=-1; }


int visBase::VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar( iopar );
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get(materialidstr,matid) )
    {
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = visBase::DM().getObj( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(Material) ) return -1;

	    setMaterial( (Material*)mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN(isonstr,isonsw) )
	VisualObjectImpl::turnOn( isonsw );

    return 1;
}


void visBase::VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( materialidstr, material ? material->id() : -1 );

    if ( material && saveids.indexOf(material->id()) == -1 )
	saveids += material->id();

    iopar.setYN( isonstr, isOn() );
}
