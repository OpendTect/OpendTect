/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: visemobjdisplay.cc,v 1.42 2005-08-15 19:05:17 cvskris Exp $
________________________________________________________________________

-*/

#include "visemobjdisplay.h"

#include "attribsel.h"
#include "binidvalset.h"
#include "cubicbeziersurface.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "mpeengine.h"
#include "emeditor.h"
#include "viscubicbeziersurface.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vishingeline.h"
#include "vismarker.h"
#include "visparametricsurface.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "survinfo.h"
#include "keystrs.h"
#include "iopar.h"

#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emsurfaceedgeline.h"



mCreateFactoryEntry( visSurvey::EMObjectDisplay );

visBase::FactoryEntry visSurvey::EMObjectDisplay::oldnameentry(
	(FactPtr) visSurvey::EMObjectDisplay::create,
	"visSurvey::SurfaceDisplay");

namespace visSurvey
{

const char* EMObjectDisplay::sKeyEarthModelID = "EarthModel ID";
const char* EMObjectDisplay::sKeyTexture = "Use texture";
const char* EMObjectDisplay::sKeyColorTableID = "ColorTable ID";
const char* EMObjectDisplay::sKeyShift = "Shift";
const char* EMObjectDisplay::sKeyEdit = "Edit";
const char* EMObjectDisplay::sKeyWireFrame = "WireFrame on";
const char* EMObjectDisplay::sKeyResolution = "Resolution";
const char* EMObjectDisplay::sKeyOnlyAtSections = "Display only on sections";
const char* EMObjectDisplay::sKeyLineStyle = "Linestyle";
const char* EMObjectDisplay::sKeyEdgeLineRadius = "Edgeline radius";

EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , em(EM::EMM())
    , mid(-1)
    , as(*new Attrib::SelSpec)
    , colas(*new Attrib::ColorSelSpec)
    , curtextureidx(0)
    , usestexture(true)
    , useswireframe(false)
    , editor(0)
    , eventcatcher(0)
    , transformation(0)
    , translation(0)
    , displayonlyatsections( false )
    , coltab_(visBase::VisColorTab::create())
    , hasmoved( this )
    , drawstyle(visBase::DrawStyle::create())
    , edgelineradius( 3.5 )
{
    coltab_->ref();
    drawstyle->ref();
    addChild( drawstyle->getInventorNode() );
}


EMObjectDisplay::~EMObjectDisplay()
{
    removeChild( drawstyle->getInventorNode() );
    drawstyle->unRef();
    drawstyle = 0;

    delete &as;
    delete &colas;
    if ( transformation ) transformation->unRef();

    setSceneEventCatcher( 0 );
    if ( coltab_ ) coltab_->unRef();

    if ( translation )
    {
	removeChild( translation->getInventorNode() );
	translation->unRef();
	translation = 0;
    }

    removeEMStuff();
}


mVisTrans* EMObjectDisplay::getDisplayTransformation()
{ return transformation; }


void EMObjectDisplay::setDisplayTransformation( mVisTrans* nt )
{
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();

    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->setDisplayTransformation(transformation);

    for ( int idx=0; idx<intersectionlines.size(); idx++ )
	intersectionlines[idx]->setDisplayTransformation(transformation);

    for ( int idx=0; idx<posattribmarkers.size(); idx++ )
	posattribmarkers[idx]->setDisplayTransformation(transformation);

    if ( editor ) editor->setDisplayTransformation(transformation);
}


void EMObjectDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove( mCB(this,EMObjectDisplay,clickCB) );
	eventcatcher->unRef();
    }

    eventcatcher = ec;
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify( mCB(this,EMObjectDisplay,clickCB) );
	eventcatcher->ref();
    }

    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->setSceneEventCatcher( ec );

    if ( editor ) editor->setSceneEventCatcher( ec );
}


void EMObjectDisplay::clickCB( CallBacker* cb )
{
    if ( !isOn() || eventcatcher->isEventHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    bool onobject = false;
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	if ( eventinfo.pickedobjids.indexOf(sections[idx]->id())!=-1 )
	{
	    onobject = true;
	    break;
	}
    }

    if  ( !onobject )
	return;

    bool keycb = false;
    bool mousecb = false;
    if ( eventinfo.type == visBase::Keyboard )
	keycb = eventinfo.key=='n' && eventinfo.pressed;
    else if ( eventinfo.type == visBase::MouseClick )
	mousecb =
	    eventinfo.mousebutton==visBase::EventInfo::leftMouseButton() &&
	    eventinfo.pressed;

    if ( !keycb && !mousecb ) return;

    const EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    mDynamicCastGet(const EM::Surface*,emsurface,emobject)
    if ( !emsurface ) return;

    Coord3 newpos =
      visSurvey::SPM().getZScaleTransform()->transformBack(eventinfo.pickedpos);
    if ( transformation )
	newpos = transformation->transformBack(newpos);

    const float tracedist = SI().transform(BinID(0,0)).distance(
	    SI().transform(BinID(SI().inlStep(true),SI().crlStep(true))));

    const BinID pickedbid = SI().transform( newpos );
    TypeSet<EM::PosID> closestnodes;
    emsurface->geometry.findPos( pickedbid, closestnodes );
    if ( !closestnodes.size() ) return;

    EM::PosID closestnode = closestnodes[0];
    float mindist=mUndefValue;
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emsurface->getPos( closestnodes[idx] );
	const Coord3 displaypos =
	    visSurvey::SPM().getZScaleTransform()->transform(transformation
		    ? transformation->transform(coord) : coord );

	const float dist = displaypos.distance( eventinfo.pickedpos );
	if ( !idx || dist<mindist )
	{
	    closestnode = closestnodes[idx];
	    mindist = dist;
	}
    }

    if ( mousecb && editor )
    {
	editor->mouseClick( closestnode, eventinfo.shift, eventinfo.alt,
			    eventinfo.ctrl );
	eventcatcher->eventIsHandled();
    }
    else if ( keycb )
    {
	const RowCol closestrc =
	    emsurface->geometry.subID2RowCol( closestnode.subID() );
	BufferString str = "Section: "; str += closestnode.sectionID();
	str += " ("; str += closestrc.row;
	str += ","; str += closestrc.col; str+=")";
	pErrMsg(str);
    }
}


void EMObjectDisplay::edgeLineRightClickCB(CallBacker*)
{}


void EMObjectDisplay::removeEMStuff()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
    }

    while ( posattribs.size() )
	showPosAttrib( posattribs[0], false, Color(0,0,0) );

    while ( intersectionlines.size() )
    {
	intersectionlines[0]->unRef();
	intersectionlines.remove(0);
	intersectionlineids.remove(0);
    }


    sections.erase();
    sectionids.erase();

    if ( editor ) editor->unRef();

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( emobject )
    {
	emobject->notifier.remove( mCB(this,EMObjectDisplay,emChangeCB));
	emobject->unRef();
	const int trackeridx = MPE::engine().getTrackerByObject(emobject->id());
	if ( trackeridx >= 0 )
	    MPE::engine().removeTracker( trackeridx );
	MPE::engine().removeEditor(emobject->id());

	mDynamicCastGet( EM::Surface*, emsurface,
			 em.getObject( em.multiID2ObjectID(mid) ));
	if ( emsurface )
	    emsurface->edgelinesets.addremovenotify.remove(
			    mCB(this,EMObjectDisplay,emEdgeLineChangeCB ));
    }

}


bool EMObjectDisplay::setEMObject( const MultiID& newmid )
{
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(newmid) );
    if ( !emobject ) return false;

    if ( sections.size() ) removeEMStuff();

    mid = newmid;
    emobject->ref();
    emobject->notifier.notify( mCB(this,EMObjectDisplay,emChangeCB) );
    mDynamicCastGet( EM::Surface*, emsurface,
		     em.getObject( em.multiID2ObjectID(mid) ));
    if ( emsurface ) emsurface->edgelinesets.addremovenotify.notify(
			mCB(this,EMObjectDisplay,emEdgeLineChangeCB ));

    return updateFromEM();
}


bool EMObjectDisplay::updateFromEM()
{ 
    if ( sections.size() ) removeEMStuff();

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( !emobject ) return false;

    setName( emobject->name() );

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
	addSection( emobject->sectionID(idx) );

    nontexturecol = emobject->preferredColor();
    getMaterial()->setColor( nontexturecol );
    updateFromMPE();

    return true;
}


void EMObjectDisplay::updateFromMPE()
{
    const EM::ObjectID objid = em.multiID2ObjectID(mid);
    const bool hastracker = MPE::engine().getTrackerByObject(objid) >= 0;
    if ( hastracker )
    {
	useWireframe( true );
	useTexture( false );
	setResolution( nrResolutions()-1 );
	showPosAttrib( EM::EMObject::sSeedNode, true, Color(255,255,255) );
    }

    if ( MPE::engine().getEditor(objid,hastracker) )
	enableEditing(true);
}


void EMObjectDisplay::showPosAttrib( int attr, bool yn, const Color& color )
{
    int attribindex = posattribs.indexOf(attr);
    if ( yn )
    {
	if ( attribindex==-1 )
	{
	    posattribs += attr;
	    visBase::DataObjectGroup* group= visBase::DataObjectGroup::create();
	    group->ref();
	    addChild( group->getInventorNode() );
	    posattribmarkers += group;

	    group->addObject( visBase::Material::create() );
	    attribindex = posattribs.size()-1;
	}

	mDynamicCastGet(visBase::Material*, material,
			posattribmarkers[attribindex]->getObject(0) );
	material->setColor( color );

	updatePosAttrib(attr);
    }
    else if ( attribindex!=-1 && !yn )
    {
	posattribs -= attr;
	removeChild(posattribmarkers[attribindex]->getInventorNode());
	posattribmarkers[attribindex]->unRef();
	posattribmarkers.remove(attribindex);
    }
}


bool EMObjectDisplay::showsPosAttrib(int attr) const
{ return posattribs.indexOf(attr)!=-1; }


bool EMObjectDisplay::addSection( EM::SectionID sid )
{
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    Geometry::Element* ge = const_cast<Geometry::Element*>(
	const_cast<const EM::EMObject*>(emobject)->getElement(sid));
    if ( !ge ) return false;

    visBase::VisualObject* vo = createSection( ge );
    if ( !vo ) return false;

    vo->ref();
    vo->setMaterial( 0 );
    vo->setDisplayTransformation( transformation );
    const int index = childIndex(drawstyle->getInventorNode());
    insertChild( index, vo->getInventorNode() );
    vo->turnOn( !displayonlyatsections );

    sections += vo;
    sectionids += sid;

    mDynamicCastGet(visBase::CubicBezierSurface*,cbs,vo);
    if ( cbs ) cbs->useWireframe( useswireframe );

    mDynamicCastGet(visBase::ParametricSurface*,psurf,vo)
    if ( psurf )
    {
	psurf->setColorTab( *coltab_ );
	psurf->useWireframe(useswireframe);
	psurf->setResolution( getResolution()-1 );
    }

    return addEdgeLineDisplay(sid);
}


bool EMObjectDisplay::addEdgeLineDisplay(EM::SectionID sid)
{
    mDynamicCastGet( EM::Surface*, emsurface,
		     em.getObject( em.multiID2ObjectID(mid) ));
    EM::EdgeLineSet* els = emsurface
	? emsurface->edgelinesets.getEdgeLineSet(sid,false) : 0;

    if ( els )
    {
	bool found = false;
	for ( int idx=0; idx<edgelinedisplays.size(); idx++ )
	{
	    if ( edgelinedisplays[idx]->getEdgeLineSet()==els )
	    {
		found = true;
		break;
	    }
	}
	
	if ( !found )
	{
	    visSurvey::EdgeLineSetDisplay* elsd =
		visSurvey::EdgeLineSetDisplay::create();
	    elsd->ref();
	    elsd->setConnect(true);
	    elsd->setEdgeLineSet(els);
	    elsd->setRadius(edgelineradius);
	    addChild( elsd->getInventorNode() );
	    elsd->setDisplayTransformation(transformation);
	    elsd->rightClicked()->notify(
		    mCB(this,EMObjectDisplay,edgeLineRightClickCB));
	    edgelinedisplays += elsd;
	}
    }

    return true;
}


void EMObjectDisplay::useTexture( bool yn )
{
    usestexture = yn;
//  if ( yn ) nontexturecol = getMaterial()->getColor();
    getMaterial()->setColor( yn ? Color::White : nontexturecol );

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	{
	    if ( psurf->nrTextures() )
		psurf->selectActiveTexture( yn ? 0 : -1 );
	}
    }
}


bool EMObjectDisplay::usesTexture() const
{ return usestexture; }


const LineStyle* EMObjectDisplay::lineStyle() const
{
    return usesWireframe() || getOnlyAtSectionsDisplay()
	? &drawstyle->lineStyle()
	: 0;
}


void EMObjectDisplay::setLineStyle( const LineStyle& ls )
{ drawstyle->setLineStyle(ls); }


bool EMObjectDisplay::hasColor() const
{
    return !usesTexture();
}


void EMObjectDisplay::setOnlyAtSectionsDisplay(bool yn)
{
    displayonlyatsections = yn;

    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->turnOn(!displayonlyatsections);

    hasmoved.trigger();
}


bool EMObjectDisplay::getOnlyAtSectionsDisplay() const
{ return displayonlyatsections; }


void EMObjectDisplay::setColor( Color col )
{
    nontexturecol = col;
    getMaterial()->setColor( col );

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( emobject )
    {
	const bool wasenabled = emobject->notifier.disable();
	emobject->setPreferredColor( nontexturecol );
	emobject->notifier.enable( wasenabled );
    }
}


Color EMObjectDisplay::getColor() const
{
    return nontexturecol;
}


void EMObjectDisplay::readAuxData()
{
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    mDynamicCastGet(EM::Surface*,emsurface,emobject)
    if ( !emsurface ) return;

    ObjectSet<BinIDValueSet> auxdata;
    for ( int idx=0; idx<emsurface->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emsurface->sectionID(idx);
	const Geometry::ParametricSurface* psurf = 
	    		emsurface->geometry.getSurface( sectionid );
	if ( !psurf ) continue;

	EM::PosID posid( emsurface->id(), sectionid );
	const int nrattribs = emsurface->auxdata.nrAuxData()+1;
	BinIDValueSet* res = new BinIDValueSet( nrattribs, false );
	auxdata += res;
	float auxvalues[nrattribs];

	const int nrnodes = psurf->nrKnots();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const RowCol rc = psurf->getKnotRowCol( idy );
	    posid.setSubID( rc.getSerialized() );
	    auxvalues[0] = 0;
	    for ( int ida=1; ida<nrattribs; ida++ )
		auxvalues[ida] = emsurface->auxdata.getAuxDataVal(ida-1,posid);

	    res->add( rc, auxvalues ); 
	}
    }

    stuffData( false, &auxdata );
    deepErase( auxdata );
}


void EMObjectDisplay::selectTexture( int textureidx )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->selectActiveTexture( textureidx );
    }

    mDynamicCastGet(EM::Surface*,emsurf,em.getObject(em.multiID2ObjectID(mid)))
    if ( !emsurf ) return;

    if ( textureidx >= emsurf->auxdata.nrAuxData() )
	setSelSpec( Attrib::SelSpec(0,Attrib::SelSpec::attribNotSel) );
    else
    {
	BufferString attrnm = emsurf->auxdata.auxDataName( textureidx );
	setSelSpec( Attrib::SelSpec(attrnm,Attrib::SelSpec::otherAttrib) );
    }
}


void EMObjectDisplay::selectNextTexture( bool next )
{
    if ( !sections.size() ) return;

    mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[0]);
    if ( !psurf ) return;

    if ( next && curtextureidx < psurf->nrTextures()-1 )
	curtextureidx++;
    else if ( !next && curtextureidx )
	curtextureidx--;
    else
	return;

    selectTexture( curtextureidx );
}


int EMObjectDisplay::getAttributeFormat() const
{
    if ( !sections.size() ) return -1;
    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections[0]);
    return ps ? 2 : -1;
}


const Attrib::SelSpec* EMObjectDisplay::getSelSpec() const
{ return &as; }


void EMObjectDisplay::setSelSpec( const Attrib::SelSpec& as_ )
{
    removeAttribCache();
    as = as_;
}


const Attrib::ColorSelSpec* EMObjectDisplay::getColorSelSpec() const
{
    return getAttributeFormat() < 0 ? 0 : &colas;
}


void EMObjectDisplay::setColorSelSpec( const Attrib::ColorSelSpec& as_ )
{ colas = as_; }


void EMObjectDisplay::setDepthAsAttrib()
{
    as.set( "", Attrib::SelSpec::noAttrib, false, "" );

    ObjectSet<BinIDValueSet> positions;
    fetchData(positions);

    if ( !positions.size() )
    {
	useTexture( false );
	return;
    }

    for ( int idx=0; idx<positions.size(); idx++ )
    {
	if ( positions[idx]->nrVals()!=2 )
	    positions[idx]->setNrVals(2);

	BinIDValueSet::Pos pos;
	while ( positions[idx]->next(pos,true) )
	{
	    float* vals = positions[idx]->getVals(pos);
	    vals[1] = vals[0];
	}
    }

    stuffData( false, &positions );
    useTexture( usestexture );
    deepErase( positions );
}


void EMObjectDisplay::fetchData( ObjectSet<BinIDValueSet>& data ) const
{
    deepErase( data );
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( !psurf ) return;
	data += new BinIDValueSet( 1, false );
	BinIDValueSet& res = *data[idx];
	psurf->getDataPositions( res, true, getTranslation().z/SI().zFactor() );
    }
}


void EMObjectDisplay::stuffData( bool forcolordata,
				 const ObjectSet<BinIDValueSet>* data )
{
    if ( forcolordata )
	return;

    if ( !data || !data->size() )
    {
	for ( int idx=0; idx<sections.size(); idx++ )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	    if ( psurf ) psurf->setTextureData( 0 );
	    else useTexture(false);
	}
	return;
    }

    int idx = 0;
    for ( ; idx<data->size(); idx++ )
    {
	if ( idx>=sections.size() ) break;

	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->setTextureData( (*data)[idx], forcolordata ? colas.datatype 
		    					      : 0 );
	else
	    useTexture(false);
    }

    for ( ; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf ) psurf->setTextureData( 0 );
    }
}


bool EMObjectDisplay::hasStoredAttrib() const
{
    const char* ref = as.userRef();
    return as.id() == Attrib::SelSpec::otherAttrib && ref && *ref;
}


Coord3 EMObjectDisplay::getTranslation() const
{
    if ( !translation ) return Coord3(0,0,0);

    Coord3 shift = translation->getTranslation();
    shift.z *= -1; 
    return shift;
}


void EMObjectDisplay::setTranslation( const Coord3& nt )
{
    if ( !translation )
    {
	translation = visBase::Transformation::create();
	translation->ref();
	insertChild( 0, translation->getInventorNode() );
    }

    Coord3 shift( nt ); shift.z *= -1;
    translation->setTranslation( shift );
}


bool EMObjectDisplay::usesWireframe() const { return useswireframe; }


void EMObjectDisplay::useWireframe( bool yn )
{
    useswireframe = yn;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::CubicBezierSurface*,cbs,sections[idx]);
	if ( cbs ) cbs->useWireframe( yn );

	mDynamicCastGet(visBase::ParametricSurface*,ps,sections[idx]);
	if ( ps ) ps->useWireframe( yn );
    }
}


void EMObjectDisplay::setEdgeLineRadius(float nr)
{
    edgelineradius = nr;
    for ( int idx=0; idx<edgelinedisplays.size(); idx++ )
	edgelinedisplays[idx]->setRadius(nr);
}


float EMObjectDisplay::getEdgeLineRadius() const
{ return edgelineradius; }



MPEEditor* EMObjectDisplay::getEditor() { return editor; }


void EMObjectDisplay::enableEditing( bool yn )
{
    if ( yn && !editor )
    {
	const EM::ObjectID objid = em.multiID2ObjectID(mid);
	MPE::ObjectEditor* mpeeditor = MPE::engine().getEditor(objid,true);

	if ( !mpeeditor ) return;

	editor = MPEEditor::create();
	editor->ref();
	editor->setSceneEventCatcher( eventcatcher );
	editor->setDisplayTransformation( transformation );
	editor->setEditor(mpeeditor);
	addChild( editor->getInventorNode() );
    }

    if ( editor ) editor->turnOn(yn);
}


bool EMObjectDisplay::isEditingEnabled() const
{ return editor && editor->isOn(); }


EM::SectionID EMObjectDisplay::getSectionID( int visid ) const
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	if ( sections[idx]->id()==visid )
	    return sectionids[idx];
    }

    return -1;
}


EM::SectionID EMObjectDisplay::getSectionID( const TypeSet<int>* path ) const
{
    for ( int idx=0; path && idx<path->size(); idx++ )
    {
	const EM::SectionID sectionid = getSectionID((*path)[idx]);
	if ( sectionid!=-1 ) return sectionid;
    }

    return -1;
}


visBase::VisualObject* EMObjectDisplay::createSection( Geometry::Element* ge )
{
    mDynamicCastGet(Geometry::CubicBezierSurface*,cbs,ge);
    if ( cbs )
    {
	visBase::CubicBezierSurface* surf =
	    		visBase::CubicBezierSurface::create();
	surf->setSurface( *cbs, false, true );
	return surf;
    }

    mDynamicCastGet(Geometry::ParametricSurface*,ps,ge);
    if ( ps )
    {
	visBase::ParametricSurface* surf =
	    		visBase::ParametricSurface::create();
	surf->setSurface( ps, true );
	return surf;
    }

    return 0;
}


void EMObjectDisplay::emChangeCB( CallBacker* cb )
{
    bool triggermovement = false;
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );

    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::SectionChange )
    {
	mDynamicCastGet(EM::Surface*,emsurface,emobject)
	if ( !emsurface ) return;

	const EM::SectionID sectionid = cbdata.pid0.sectionID();
	if ( emsurface->geometry.hasSection(sectionid) )
	    addSection( sectionid );
	else
	{
	    const int idx = sectionids.indexOf( sectionid );
	    if ( idx < 0 ) return;
	    removeChild( sections[idx]->getInventorNode() );
	    sections[idx]->unRef();
	    sections.remove( idx );
	    sectionids.remove(idx);
	}

	triggermovement = true;
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	triggermovement = true;
    else if ( cbdata.event==EM::EMObjectCallbackData::PosIDChange )
    {
	if ( posattribs.indexOf(cbdata.attrib)!=-1 )
	    updatePosAttrib(cbdata.attrib);
    }

    if ( triggermovement )
	hasmoved.trigger();
}


void EMObjectDisplay::emEdgeLineChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack( EM::SectionID, section, cb );

    const EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    mDynamicCastGet(const EM::Surface*,emsurface,emobject)
    if ( !emsurface ) return;

    if ( emsurface->edgelinesets.getEdgeLineSet( section, false ) )
	 addEdgeLineDisplay(section);
    else
    {
	for ( int idx=0; idx<edgelinedisplays.size(); idx++ )
	{
	    if (edgelinedisplays[idx]->getEdgeLineSet()->getSection()==section)
  	    {
		EdgeLineSetDisplay* elsd = edgelinedisplays[idx--];
		edgelinedisplays -= elsd;
		elsd->rightClicked()->remove(
			 mCB( this, EMObjectDisplay, edgeLineRightClickCB ));
		removeChild( elsd->getInventorNode() );
		elsd->unRef();
		break;
	    }
	}
    }
}



void EMObjectDisplay::removeAttribCache()
{
    deepEraseArr( attribcache );
    attribcachesz.erase();
}


int EMObjectDisplay::nrResolutions() const
{
    if ( !sections.size() ) return 1;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections[0]);
    return ps ? ps->nrResolutions()+1 : 1;
}


BufferString EMObjectDisplay::getResolutionName( int res ) const
{
    BufferString str;
    if ( !res ) str = "Automatic";
    else
    {
	res = nrResolutions() - res;
	res--;
	int val = 1;
	for ( int idx=0; idx<res; idx++ )
	    val *= 2;

	if ( val==2 ) 		str = "Half";
	else if ( val==1 ) 	str = "Full";
	else 			{ str = "1 / "; str += val; }
    }

    return str;
}


int EMObjectDisplay::getResolution() const
{
    if ( !sections.size() ) return 1;

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections[0]);
    return ps ? ps->currentResolution()+1 : 0;
}


void EMObjectDisplay::setResolution( int res )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections[idx]);
	if ( ps ) ps->setResolution( res-1 );
    }
}


int EMObjectDisplay::getColTabID() const
{
    return usesTexture() ? coltab_->id() : -1;
}


void EMObjectDisplay::getMousePosInfo( const Coord3& pos, float& val,
				       BufferString& info ) const
{
    info = ""; val = pos.z;
    const EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( !emobject ) return;
    
    info = emobject->getTypeStr(); info += ": "; info += name();
    if ( !sections.size() || as.id() < -1 ) return;

    mDynamicCastGet(const EM::Surface*,emsurface,emobject)
    if ( !emsurface || !emsurface->auxdata.nrAuxData() )
	return;
    const RowCol rc( SI().transform(pos) );
    EM::PosID posid( emsurface->id(), 0, rc.getSerialized() );
    for ( int idx=0; idx<emsurface->nrSections(); idx++ )
    {
	posid.setSectionID( emsurface->sectionID(idx) );
	val = emsurface->auxdata.getAuxDataVal( curtextureidx, posid );
	if ( !mIsUndefined(val) )
	    break;
    }
}


void EMObjectDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID, mid );
    par.setYN( sKeyTexture, usesTexture() );
    par.setYN( sKeyWireFrame, usesWireframe() );
    par.setYN( sKeyEdit, isEditingEnabled() );
    par.setYN( sKeyOnlyAtSections, getOnlyAtSectionsDisplay() );
    par.set( sKeyResolution, getResolution() );
    par.set( sKeyShift, getTranslation().z );
    par.set( sKey::Color, (int)nontexturecol.rgb() );
    par.set( sKeyColorTableID, coltab_->id() );
    if ( saveids.indexOf(coltab_->id())==-1 )
	saveids += coltab_->id();

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle, str );
    }

    as.fillPar( par );
    colas.fillPar( par );

}


int EMObjectDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    setDisplayTransformation( SPM().getUTM2DisplayTransform() );

    MultiID surfid;
    if ( !par.get(sKeyEarthModelID,surfid) )
	return -1;

    as.usePar( par );
    colas.usePar( par );

    BufferString linestyle;
    if ( par.get( sKeyLineStyle, linestyle ) )
    {
	LineStyle ls;
	ls.fromString(linestyle);
	setLineStyle(ls);
    }

    // Editing may not be moved further down, since the enableEditing call
    // will change the wireframe, resolution++
    bool enableedit = false;
    par.getYN( sKeyEdit, enableedit );
    enableEditing( enableedit );

    if ( !par.get(sKey::Color,(int&)nontexturecol.rgb()) )
	nontexturecol = getMaterial()->getColor();

    bool usetext = true;
    par.getYN( sKeyTexture, usetext );
    useTexture( usetext );

    bool usewireframe = false;
    par.getYN( sKeyWireFrame, usewireframe );
    useWireframe( usewireframe );

    int resolution = 0;
    par.get( sKeyResolution, resolution );
    setResolution( resolution );

    bool filter = false;
    par.getYN( sKeyOnlyAtSections, filter );
    setOnlyAtSectionsDisplay(filter);

    visBase::VisColorTab* coltab;
    int coltabid = -1;
    par.get( sKeyColorTableID, coltabid );
    if ( coltabid > -1 )
    {
	DataObject* dataobj = visBase::DM().getObject( coltabid );
	if ( !dataobj ) return 0;
	coltab = (visBase::VisColorTab*)dataobj;
	if ( !coltab ) return -1;
	if ( coltab_ ) coltab_->unRef();
	coltab_ = coltab;
	coltab_->ref();
    }

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift, shift.z );
    setTranslation( shift );

    mid = surfid;

    return 1;
}


NotifierAccess* EMObjectDisplay::getMovementNotification() { return &hasmoved; }


void EMObjectDisplay::updatePosAttrib(int attrib)
{
    const int attribindex = posattribs.indexOf(attrib);
    if ( attribindex==-1 ) return;

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    const TypeSet<EM::PosID>* pids = emobject->getPosAttribList(attrib);
    if ( !pids )
    {
	showPosAttrib( attrib, false, Color(0,0,0) );
	return;
    }

    //Remove everything but material
    while ( posattribmarkers[attribindex]->size()>1 )
	posattribmarkers[attribindex]->removeObject(1);

    for ( int idx=0; idx<pids->size(); idx++ )
    {
	const Coord3 pos = emobject->getPos((*pids)[idx]);
	if ( !pos.isDefined() ) continue;

	visBase::Marker* marker = visBase::Marker::create();
	posattribmarkers[attribindex]->addObject(marker);

	marker->setMaterial(0);
	marker->setDisplayTransformation(transformation);
	marker->setCenterPos(pos);
    }
}


#define mEndLine \
{ \
    if ( cii<2 || ( cii>2 && line->getCoordIndex(cii-2)==-1 ) ) \
    { \
	while ( cii && line->getCoordIndex(cii-1)!=-1 ) \
	    line->getCoordinates()->removePos(line->getCoordIndex(--cii)); \
    } \
    else \
    { \
	line->setCoordIndex(cii++,-1); \
    } \
}

#define mTraverseLine( linetype, startbid, faststop, faststep, slowdim, fastdim ) \
    const int target##linetype = cs.hrg.start.linetype; \
    if ( !linetype##rg.includes(target##linetype) ) \
    { \
	mEndLine; \
	continue; \
    } \
 \
    const int rgindex = linetype##rg.getIndex(target##linetype); \
    const int prev##linetype = linetype##rg.atIndex(rgindex); \
    const int next##linetype = prev##linetype<target##linetype \
	? linetype##rg.atIndex(rgindex+1) \
	: prev##linetype; \
 \
    for ( BinID bid=startbid; bid[fastdim]<=faststop; bid[fastdim]+=faststep ) \
    { \
	if ( !cs.hrg.includes(bid) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	BinID prevbid( bid ); prevbid[slowdim] = prev##linetype; \
	BinID nextbid( bid ); nextbid[slowdim] = next##linetype; \
	const Coord3 prevpos(horizon->geometry.getPos(sid,prevbid)); \
	Coord3 pos = prevpos; \
	if ( nextbid!=prevbid && prevpos.isDefined() ) \
	{ \
	    const Coord3 nextpos = \
		horizon->geometry.getPos(sid,nextbid); \
	    if ( nextpos.isDefined() ) \
	    { \
		pos += nextpos; \
		pos /= 2; \
	    } \
	} \
 \
	if ( !pos.isDefined() || !cs.zrg.includes(pos.z) ) \
	{ \
	    mEndLine; \
	    continue; \
	} \
 \
	line->setCoordIndex(cii++, line->getCoordinates()->addPos(pos)); \
    }

void EMObjectDisplay::updateIntersectionLines(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{
    const EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    mDynamicCastGet(const EM::Horizon*,horizon,emobject);
    if ( !horizon ) return;

    if ( whichobj==id() )
	whichobj = -1;

    TypeSet<int> linestoupdate;
    BoolTypeSet lineshouldexist( intersectionlineids.size(), false );

    if ( displayonlyatsections )
    {
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( !plane ) continue;

	    const int idy = intersectionlineids.indexOf(plane->id());
	    if ( idy==-1 )
	    {
		linestoupdate += plane->id();
		lineshouldexist += true;
	    }
	    else
	    {
		lineshouldexist[idy] = true;
	    }
	}

	if ( whichobj!=-1 && linestoupdate.indexOf(whichobj)==-1 )
	    linestoupdate += whichobj;
    }

    for ( int idx=0; idx<intersectionlineids.size(); idx++ )
    {
	if ( !lineshouldexist[idx] )
	{
	    removeChild( intersectionlines[idx]->getInventorNode() );
	    intersectionlines[idx]->unRef();

	    lineshouldexist.remove(idx);
	    intersectionlines.remove(idx);
	    intersectionlineids.remove(idx);
	    idx--;
	}
    }

    for ( int idx=0; idx<linestoupdate.size(); idx++ )
    {
	mDynamicCastGet( PlaneDataDisplay*, plane,
			 visBase::DM().getObject(linestoupdate[idx]) );

	const CubeSampling cs = plane->getCubeSampling();

	int lineidx = intersectionlineids.indexOf(linestoupdate[idx]);
	if ( lineidx==-1 )
	{
	    lineidx = intersectionlineids.size();
	    intersectionlineids += plane->id();
	    visBase::IndexedPolyLine* newline =
		visBase::IndexedPolyLine::create();
	    newline->ref();
	    newline->setDisplayTransformation(transformation);
	    intersectionlines += newline;
	    addChild( newline->getInventorNode() );
	}

	visBase::IndexedPolyLine* line = intersectionlines[lineidx];
	line->getCoordinates()->removeAfter(-1);
	line->removeCoordIndexAfter(-1);
	int cii = 0;

	if ( plane->getType()==PlaneDataDisplay::Timeslice )
	    continue;

	for ( int sectionidx=0; sectionidx<horizon->nrSections(); sectionidx++ )
	{
	    const EM::SectionID sid = horizon->sectionID(sectionidx);
	    const StepInterval<int> inlrg = horizon->geometry.rowRange(sid);
	    const StepInterval<int> crlrg = horizon->geometry.colRange(sid);

	    if ( plane->getType()==PlaneDataDisplay::Inline )
	    {
		mTraverseLine( inl, BinID(targetinl,crlrg.start),
			       crlrg.stop, crlrg.step, 0, 1 );
	    }
	    else
	    {
		mTraverseLine( crl, BinID(inlrg.start,targetcrl),
			       inlrg.stop, inlrg.step, 1, 0 );
	    }

	    mEndLine;
	}
    }
}


void EMObjectDisplay::otherObjectsMoved(
	    const ObjectSet<const SurveyObject>& objs, int whichobj )
{ updateIntersectionLines(objs,whichobj); }




}; // namespace visSurvey
