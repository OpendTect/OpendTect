
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.18 2002-05-02 11:30:46 kristofer Exp $";

#include "visobject.h"
#include "vismaterial.h"
#include "visdataman.h"
#include "iopar.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"

const char* visBase::VisualObjectImpl::materialidstr = "Material ID";
const char* visBase::VisualObjectImpl::isonstr = "Is on";

visBase::VisualObject::VisualObject( bool selectable_ )
    : isselectable( selectable_ )
    , deselnotifier( this )
    , selnotifier( this )
{}


visBase::VisualObjectImpl::VisualObjectImpl( bool selectable_ )
    : root( new SoSeparator )
    , onoff( new SoSwitch )
    , material( 0 )
    , VisualObject( selectable_ )
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
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = visBase::DM().getObj( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat)!=typeid(Material) ) return -1;

	    setMaterial( (Material*) mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN( isonstr, isonsw ) )
	turnOn( isonsw );

    return 1;
}


void visBase::VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( materialidstr, material ? material->id(): -1 );

    if ( material && saveids.indexOf( material->id() )==-1 )
	saveids += material->id();

    iopar.setYN( isonstr, isOn() );
}
    
