/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: visemobjdisplay.cc,v 1.1 2005-01-06 10:53:24 kristofer Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: visemobjdisplay.cc,v 1.1 2005-01-06 10:53:24 kristofer Exp $";


#include "vissurvemobj.h"

#include "attribsel.h"
#include "cubicbeziersurface.h"
#include "emmanager.h"
#include "emobject.h"
#include "viscubicbeziersurface.h"
#include "vismaterial.h"


namespace visSurvey
{


mCreateFactoryEntry( EMObjectDisplay );


EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , mid(-1)
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , usestexture(false)
{}


EMObjectDisplay::~EMObjectDisplay()
{
    removeAll();

    delete &as;
    delete &colas;
}


void EMObjectDisplay::removeAll()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
    }

    sections.erase();
    sectionids.erase();

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(mid));
    if ( emobject ) emobject->unRef();
}


bool EMObjectDisplay::setEMObject(const MultiID& newmid)
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(newmid));
    if ( !emobject ) return false;

    if ( sections.size() ) removeAll();

    mid = newmid;
    emobject->ref();
    return updateFromEM();
}


bool EMObjectDisplay::updateFromEM()
{ 
    if ( sections.size() ) removeAll();

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobject = em.getObject(em.multiID2ObjectID(mid));
    if ( !emobject ) return false;

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emobject->sectionID(idx);
	Geometry::Element* ge = const_cast<Geometry::Element*>(
	    const_cast<const EM::EMObject*>(emobject)->getElement(sectionid));
	if ( !ge ) continue;

	visBase::VisualObject* vo = createSection( ge );
	if ( !vo ) continue;

	vo->ref();
	vo->setMaterial( 0 );
	addChild( vo->getInventorNode() );

	sections += vo;
	sectionids += sectionid;
    }

    return true;
}


void EMObjectDisplay::useTexture(bool yn)
{
    usestexture = yn;
    if ( usesTexture()==yn )
	return;

    Color newcolor = yn ? Color::White : nontexturecol;
    if ( yn ) nontexturecol = getMaterial()->getColor();

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	//TODO: DynamicCast and set texture
    }
}


bool EMObjectDisplay::usesTexture() const { return usestexture; }



const AttribSelSpec* EMObjectDisplay::getSelSpec() const
{ return &as; }


void EMObjectDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    removeAttribCache();
    as = as_;
}


const ColorAttribSel* EMObjectDisplay::getColorSelSpec() const
{ return &colas; }


void EMObjectDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void EMObjectDisplay::setDepthAsAttrib()
{
    pErrMsg("Not impl");
}


MPEEditor* EMObjectDisplay::getEditor()
{
    pErrMsg("Not impl");
    return 0;
}


EM::SectionID EMObjectDisplay::getSectionID(int visid) const
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	if ( sections[idx]->id()==visid )
	    return sectionids[idx];
    }

    return -1;
}



visBase::VisualObject*
EMObjectDisplay::createSection( Geometry::Element* ge )
{
    mDynamicCastGet( Geometry::CubicBezierSurface*, cbs, ge );
    if ( cbs )
    {
	visBase::CubicBezierSurface* surf =
	    		visBase::CubicBezierSurface::create();
	surf->setSurface( *cbs, false, true );
	return surf;
    }

    return 0;
}


void EMObjectDisplay::removeAttribCache()
{
    deepEraseArr( attribcache );
    attribcachesz.erase();
}


};
