
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.9 2002-03-01 10:12:12 kristofer Exp $";

#include "visobject.h"
#include "colortab.h"
#include "visscene.h"
#include "visselman.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoDrawStyle.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoGroup.h"

visBase::VisualObjectImpl::VisualObjectImpl( Scene& scene_ )
    : root( new SoSeparator )
    , onoff( new SoSwitch )
    , material( new SoSwitch )
    , drawstyle( new SoDrawStyle )
    , defaultmaterial( new SoMaterial )
    , colortablematerial( new SoMaterial )
    , usecolortable( false )
    , ambience( 0.2 )
    , diffuseintencity( 0.8 )
    , specularintensity( 0 )
    , emmissiveintensity( 0 )
    , shininess( 0 )
    , transparency( 0 )
    , colortable( 0 )
    , scene( scene_ )
{
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;

    SoGroup* defmaterialgroup = new SoGroup;
    defmaterialgroup->addChild( defaultmaterial );
    SoMaterialBinding* defaultbinidng = new SoMaterialBinding;
    defaultbinidng->value = SoMaterialBinding::OVERALL;
    defmaterialgroup->addChild( defaultbinidng );

    material->addChild( defmaterialgroup );
    material->addChild( colortablematerial );
    material->whichChild = 0;
    root->addChild( material );

    root->addChild( drawstyle );
    updateMaterial();
}


visBase::VisualObjectImpl::~VisualObjectImpl()
{
    scene.selMan().unRegSelObject( *this );
    onoff->unref();
    delete colortable;
}


void visBase::VisualObjectImpl::turnOn(bool n)
{
    onoff->whichChild = n ? 0 : SO_SWITCH_NONE;
}


bool visBase::VisualObjectImpl::isOn() const
{
    return !onoff->whichChild.getValue();
}


void visBase::VisualObjectImpl::setColor( const Color& nc )
{
    color = nc;
    updateMaterial();
}
    


void visBase::VisualObjectImpl::switchColorMode( bool totable )
{
    material->whichChild = totable ? 1 : 0;
    if ( totable && !colortable ) colortable = new ColorTable;
}


bool  visBase::VisualObjectImpl::isColorTable() const
{
    return material->whichChild.getValue();
}


const ColorTable& visBase::VisualObjectImpl::colorTable() const
{
    if ( !colortable )
	const_cast<VisualObjectImpl*>(this)->colortable = new ColorTable;
    return *colortable;
}


ColorTable& visBase::VisualObjectImpl::colorTable()
{
    if ( !colortable ) colortable = new ColorTable;
    return *colortable;
}


SoNode* visBase::VisualObjectImpl::getData() { return onoff; }


void visBase::VisualObjectImpl::updateMaterial()
{
    defaultmaterial->ambientColor.setValue( color.r() * ambience/255,
					    color.g() * ambience/255,
					    color.b() * ambience/255 );

    defaultmaterial->diffuseColor.setValue( color.r() * diffuseintencity/255,
					    color.g() * diffuseintencity/255,
					    color.b() * diffuseintencity/255 );

    defaultmaterial->specularColor.setValue( color.r() * specularintensity/255,
					    color.g() * specularintensity/255,
					    color.b() * specularintensity/255 );

    defaultmaterial->emissiveColor.setValue( color.r() * emmissiveintensity/255,
					    color.g() * emmissiveintensity/255,
					    color.b() * emmissiveintensity/255);

    defaultmaterial->shininess = shininess;
    defaultmaterial->transparency = transparency;
}

