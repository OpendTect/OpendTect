
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.13 2002-03-18 14:45:41 kristofer Exp $";

#include "visobject.h"
#include "vismaterial.h"
#include "visdataman.h"
#include "iopar.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoDrawStyle.h"

visBase::VisualObjectImpl::VisualObjectImpl()
    : root( new SoSeparator )
    , onoff( new SoSwitch )
    , drawstyle( new SoDrawStyle )
    , material( 0 )
{
    setMaterial( visBase::Material::create() );
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;
}


visBase::VisualObjectImpl::~VisualObjectImpl()
{
    onoff->unref();
    material->unRef();
}


void visBase::VisualObjectImpl::turnOn(bool n)
{
    onoff->whichChild = n ? 0 : SO_SWITCH_NONE;
}


bool visBase::VisualObjectImpl::isOn() const
{
    return !onoff->whichChild.getValue();
}


void visBase::VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material )
    {
	root->removeChild( material->getData() );
	material->unRef();
    }

    material = nm;

    if ( material )
    {
	material->ref();
	root->insertChild( material->getData(), 0 );
    }
}


SoNode* visBase::VisualObjectImpl::getData() 
{ return onoff; }


void visBase::VisualObjectImpl::addChild( SoNode* nn )
{ root->addChild( nn ); }


void visBase::VisualObjectImpl::insertChild( int pos, SoNode* nn )
{ root->insertChild( nn, pos ); }


void visBase::VisualObjectImpl::removeChild( SoNode* n )
{ root->removeChild( n ); }


int visBase::VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar(iopar);
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get( materialidstr, matid ) )
    {
	DataObject* mat = visBase::DM().getObj( matid );
	if ( !mat ) return 0;
	if ( typeid(*mat)!=typeid(Material) ) return -1;

	setMaterial( (Material*) mat );
    }

    bool isonsw;
    if ( iopar.getYN( isonstr, isonsw ) )
	turnOn( isonsw );

    return 1;
}
