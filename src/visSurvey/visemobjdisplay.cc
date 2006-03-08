/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: visemobjdisplay.cc,v 1.74 2006-03-08 18:19:52 cvskris Exp $
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
#include "vismpe.h"
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

#include <math.h>



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
const char* EMObjectDisplay::sKeyRowRange = "Row range";
const char* EMObjectDisplay::sKeyColRange = "Col range";
const char* EMObjectDisplay::sKeySections = "Displayed Sections";


EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , em(EM::EMM())
    , oid(-1)
    , parmid(-1)
    , parrowrg( -1, -1, -1 )
    , parcolrg( -1, -1, -1 )
    , curtextureidx(0)
    , usestexture(true)
    , useswireframe(false)
    , editor(0)
    , eventcatcher(0)
    , transformation(0)
    , translation(0)
    , displayonlyatsections( false )
    , hasmoved( this )
    , changedisplay( this )
    , drawstyle(visBase::DrawStyle::create())
    , edgelineradius( 3.5 )
    , validtexture_( false )
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    coltabs_[0]->ref();

    drawstyle->ref();
    addChild( drawstyle->getInventorNode() );

    LineStyle defls; defls.width = 3;
    drawstyle->setLineStyle( defls );
}


EMObjectDisplay::~EMObjectDisplay()
{
    removeChild( drawstyle->getInventorNode() );
    drawstyle->unRef();
    drawstyle = 0;

    deepErase(as_);

    if ( transformation ) transformation->unRef();

    setSceneEventCatcher( 0 );
    deepUnRef( coltabs_ );

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

    if ( editor && !editor->clickCB( cb ) )
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

    if ( !onobject && editor )
	onobject = eventinfo.pickedobjids.indexOf( editor->id() )!=-1;

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

    const EM::EMObject* emobject = em.getObject( oid );
    mDynamicCastGet(const EM::Surface*,emsurface,emobject)
    if ( !emsurface ) return;

    const mVisTrans* ztrans = scene_->getZScaleTransform();
    Coord3 newpos = ztrans->transformBack( eventinfo.pickedpos );
    if ( transformation )
	newpos = transformation->transformBack( newpos );

    const float tracedist = SI().transform(BinID(0,0)).distance(
	    SI().transform(BinID(SI().inlStep(),SI().crlStep())));

    const BinID pickedbid = SI().transform( newpos );
    TypeSet<EM::PosID> closestnodes;
    emsurface->geometry.findPos( pickedbid, closestnodes );
    if ( !closestnodes.size() ) return;

    EM::PosID closestnode = closestnodes[0];
    float mindist=mUndefValue;
    for ( int idx=0; idx<closestnodes.size(); idx++ )
    {
	const Coord3 coord = emsurface->getPos( closestnodes[idx] );
	const Coord3 displaypos = ztrans->transform(
		transformation ? transformation->transform(coord) : coord );

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
	const RowCol closestrc( closestnode.subID() );
	BufferString str = "Section: "; str += closestnode.sectionID();
	str += " ("; str += closestrc.row;
	str += ","; str += closestrc.col; str += ",";
	str += emsurface->getPos(closestnode.sectionID(),closestnode.subID()).z;
	str+=")";
	pErrMsg(str);
    }
}


void EMObjectDisplay::edgeLineRightClickCB( CallBacker* )
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

    EM::EMObject* emobject = em.getObject( oid );
    if ( emobject )
    {
	emobject->notifier.remove( mCB(this,EMObjectDisplay,emChangeCB));
	const int trackeridx = MPE::engine().getTrackerByObject(emobject->id());
	if ( trackeridx >= 0 )
	    MPE::engine().removeTracker( trackeridx );
	MPE::engine().removeEditor(emobject->id());

	mDynamicCastGet( EM::Surface*, emsurface, emobject );
	if ( emsurface )
	    emsurface->edgelinesets.addremovenotify.remove(
			    mCB(this,EMObjectDisplay,emEdgeLineChangeCB ));
	emobject->unRef();
    }

}


bool EMObjectDisplay::setEMObject( const EM::ObjectID& newid )
{
    EM::EMObject* emobject = em.getObject( newid );
    if ( !emobject ) return false;

    if ( sections.size() ) removeEMStuff();

    oid = newid;
    emobject->ref();
    emobject->notifier.notify( mCB(this,EMObjectDisplay,emChangeCB) );
    mDynamicCastGet( EM::Surface*, emsurface, emobject );
    if ( emsurface ) emsurface->edgelinesets.addremovenotify.notify(
			mCB(this,EMObjectDisplay,emEdgeLineChangeCB ));

    return updateFromEM();
}


MultiID EMObjectDisplay::getMultiID() const
{
    const EM::EMObject* emobject = em.getObject( oid );
    if ( !emobject ) return parmid;

    return emobject->multiID();
}


BufferStringSet EMObjectDisplay::displayedSections() const
{
    const EM::EMObject* emobject = em.getObject( oid );
    if ( !emobject )
	return parsections;
   
    BufferStringSet res; 
    for ( int idx=emobject->nrSections()-1; idx>=0; idx-- )
	res +=new BufferString(emobject->sectionName(emobject->sectionID(idx)));

    return res;
}


StepInterval<int> EMObjectDisplay::displayedRowRange() const
{
    mDynamicCastGet(const EM::Surface*, surface, em.getObject( oid ) );
    if ( !surface ) return parrowrg;
    
    return surface->geometry.rowRange();
}


StepInterval<int> EMObjectDisplay::displayedColRange() const
{
    mDynamicCastGet(const EM::Surface*, surface, em.getObject( oid ) );
    if ( !surface ) return parcolrg;
    
    return surface->geometry.colRange();
}


bool EMObjectDisplay::updateFromEM()
{ 
    if ( sections.size() ) removeEMStuff();

    EM::EMObject* emobject = em.getObject( oid );
    if ( !emobject ) return false;

    setName( emobject->name() );

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
	addSection( emobject->sectionID(idx) );

    nontexturecol = emobject->preferredColor();
    getMaterial()->setColor( nontexturecol );
    if ( usestexture ) useTexture( true );
    updateFromMPE();

    return true;
}


void EMObjectDisplay::updateFromMPE()
{
    const bool hastracker = MPE::engine().getTrackerByObject(oid) >= 0;
    if ( hastracker )
    {
	useWireframe( true );
	useTexture( false );
	setResolution( nrResolutions()-1 );
	showPosAttrib( EM::EMObject::sSeedNode, true, Color(255,255,255) );
    }

    if ( MPE::engine().getEditor(oid,hastracker) )
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

	mDynamicCastGet(visBase::Material*, posattrmat,
			posattribmarkers[attribindex]->getObject(0) );
	posattrmat->setColor( color );

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
    EM::EMObject* emobject = em.getObject( oid );
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
	while ( psurf->nrTextures()<nrAttribs() )
	    psurf->addTexture();

	for ( int idx=0; idx<nrAttribs(); idx++ )
	    psurf->setColorTab( idx, *coltabs_[idx] );

	psurf->useWireframe(useswireframe);
	psurf->setResolution( getResolution()-1 );
    }

    return addEdgeLineDisplay(sid);
}


bool EMObjectDisplay::addEdgeLineDisplay(EM::SectionID sid)
{
    mDynamicCastGet( EM::Surface*, emsurface, em.getObject( oid) );
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


void EMObjectDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( as_[idx]->id()==Attrib::SelSpec::cNoAttrib() )
	    {
		usestexture = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture = yn;

    getMaterial()->setColor( yn ? Color::White : nontexturecol );

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->useTexture( yn );
    }
    
    if ( trigger )
	changedisplay.trigger();
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
    changedisplay.trigger();
}


bool EMObjectDisplay::getOnlyAtSectionsDisplay() const
{ return displayonlyatsections; }


void EMObjectDisplay::setColor( Color col )
{

    EM::EMObject* emobject = em.getObject( oid );
    if ( emobject )
    {
	emobject->setPreferredColor( col );
    }
    changedisplay.trigger();
}


Color EMObjectDisplay::getColor() const
{
    return nontexturecol;
}


void EMObjectDisplay::selectTexture( int attrib, int textureidx )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->selectActiveVersion( attrib, textureidx );
    }

    mDynamicCastGet(EM::Surface*,emsurf,em.getObject(oid))
    if ( !emsurf ) return;

    if ( textureidx >= emsurf->auxdata.nrAuxData() )
	setSelSpec( 0, Attrib::SelSpec(0,Attrib::SelSpec::cAttribNotSel()) );
    else
    {
	BufferString attrnm = emsurf->auxdata.auxDataName( textureidx );
	setSelSpec( 0, Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
    }
}


SurveyObject::AttribFormat EMObjectDisplay::getAttributeFormat() const
{
    if ( !sections.size() ) return SurveyObject::None;
    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections[0]);
    return ps ? SurveyObject::RandomPos : SurveyObject::None;
}


bool EMObjectDisplay::canHaveMultipleAttribs() const
{ return true; }


int EMObjectDisplay::nrAttribs() const
{ return as_.size(); }


bool EMObjectDisplay::addAttrib()
{
    as_ += new Attrib::SelSpec;
    coltabs_ += visBase::VisColorTab::create();
    coltabs_[coltabs_.size()-1]->ref();

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	{
	    psurf->addTexture();
	    psurf->setColorTab( coltabs_.size()-1,
		    		*coltabs_[coltabs_.size()-1] );
	}
    }

    return true;
}


bool EMObjectDisplay::removeAttrib( int attrib )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->removeTexture( attrib );
    }

    delete as_[attrib];
    as_.remove( attrib );
    return true;
}


bool EMObjectDisplay::swapAttribs( int a0, int a1 )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->swapTextures( a0, a1 );
    }

    as_.swap( a0, a1 );
    return true;
}

void EMObjectDisplay::enableAttrib( int attribnr, bool yn )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf )
	    psurf->enableTexture( attribnr, yn );
    }
}


bool EMObjectDisplay::isAttribEnabled( int attribnr ) const
{
    if ( !sections.size() )
	return true;

    mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[0]);
    return psurf ? psurf->isTextureEnabled(attribnr) : true;
}


const Attrib::SelSpec* EMObjectDisplay::getSelSpec( int attrib ) const
{ return as_[attrib]; }


void EMObjectDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    (*as_[attrib]) = as;
}


void EMObjectDisplay::setDepthAsAttrib( int attrib )
{
    as_[attrib]->set( "", Attrib::SelSpec::cNoAttrib(), false, "" );

    ObjectSet<BinIDValueSet> positions;
    getRandomPos( positions );

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

    setRandomPosData( attrib, &positions );
    useTexture( usestexture );
    deepErase( positions );
}


void EMObjectDisplay::getRandomPos( ObjectSet<BinIDValueSet>& data ) const
{
    deepErase( data );
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( !psurf ) return;

	data += new BinIDValueSet( 1, false );
	BinIDValueSet& res = *data[idx];
	psurf->getDataPositions( res, getTranslation().z/SI().zFactor() );
    }
}


void EMObjectDisplay::getRandomPosCache( int attrib,
				 ObjectSet<const BinIDValueSet>& data ) const
{
    data.erase();
    data.allowNull( true );
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( !psurf )
	{
	    data += 0;
	    continue;
	}

	data += psurf->getCache( attrib );
    }
}


void EMObjectDisplay::setRandomPosData( int attrib,
				 const ObjectSet<BinIDValueSet>* data )
{
    validtexture_ = true;

    if ( !data || !data->size() )
    {
	for ( int idx=0; idx<sections.size(); idx++ )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	    if ( psurf ) psurf->setTextureData( 0, attrib );
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
	    psurf->setTextureData( (*data)[idx], attrib );
	else
	    useTexture(false);
    }

    for ( ; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,psurf,sections[idx]);
	if ( psurf ) psurf->setTextureData( 0, attrib );
    }
}


bool EMObjectDisplay::hasStoredAttrib( int attrib ) const
{
    const char* userref = as_[attrib]->userRef();
    return as_[attrib]->id()==Attrib::SelSpec::cOtherAttrib() &&
	   userref && *userref;
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

    const EM::EMObject* emobject = em.getObject( oid );
    mDynamicCastGet(const EM::Horizon*,horizon,emobject);
    if ( horizon ) horizon->geometry.setShift( shift.z );

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::ParametricSurface*,ps,sections[idx]);
	if ( ps ) ps->inValidateCache(-1);
    }
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
	MPE::ObjectEditor* mpeeditor = MPE::engine().getEditor(oid,true);

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

    EM::EMObject* emobject = em.getObject( oid );

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
	    sectionids.remove( idx );
	}

	triggermovement = true;
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	for ( int idx=0; idx<posattribs.size(); idx++ )
	{
	    const TypeSet<EM::PosID>* pids =
		emobject->getPosAttribList(posattribs[idx]);

	    if ( !pids || pids->indexOf(cbdata.pid0)==-1 )
		continue;

	    updatePosAttrib(posattribs[idx]);
	}
	
	validtexture_ = false;
	if ( usesTexture() ) useTexture(false);

	const EM::SectionID sid = cbdata.pid0.sectionID();
	const int idx = sectionids.indexOf( sid );
	if ( idx>=0 )
	{
	    mDynamicCastGet(visBase::ParametricSurface*,ps,sections[idx]);
	    if ( ps ) ps->inValidateCache(-1);
	}

	triggermovement = true;
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::AttribChange )
    {
	if ( posattribs.indexOf(cbdata.attrib)!=-1 )
	    updatePosAttrib(cbdata.attrib);
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol = emobject->preferredColor();
	if ( !usestexture )
	    getMaterial()->setColor( nontexturecol );
    }

    if ( triggermovement )
	hasmoved.trigger();
}


void EMObjectDisplay::emEdgeLineChangeCB(CallBacker* cb)
{
    mCBCapsuleUnpack( EM::SectionID, section, cb );

    const EM::EMObject* emobject = em.getObject( oid );
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


int EMObjectDisplay::getColTabID(int attrib) const
{
    return usesTexture() ? coltabs_[attrib]->id() : -1;
}


float EMObjectDisplay::calcDist( const Coord3& pickpos ) const
{
    const EM::EMObject* emobject = em.getObject( oid );
    if ( !emobject ) return mUdf(float);

    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pickpos );
    mDynamicCastGet(const EM::Horizon*,hor,emobject)
    if ( hor )
    {
	const BinID bid = SI().transform( xytpos );
	TypeSet<Coord3> positions;
	hor->geometry.getPos( bid, positions );

	float mindist = mUndefValue;
	for ( int idx=0; idx<positions.size(); idx++ )
	{
	    const Coord3& pos = positions[idx] + 
				getTranslation()/SI().zFactor();
	    const float dist = fabs(xytpos.z-pos.z);
	    if ( dist < mindist ) mindist = dist;
	}

	return mindist;
    }

    return mUdf(float);
}


float EMObjectDisplay::maxDist() const
{
    return SI().zRange(true).step;
}


void EMObjectDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       const Coord3& pos,
				       float& val, BufferString& info ) const
{
    info = ""; val = pos.z;
    const EM::EMObject* emobject = em.getObject( oid );
    if ( !emobject ) return;
    
    info = emobject->getTypeStr(); info += ": "; info += name();

    if ( !sections.size() )
	return;
   
    const EM::SectionID sid = getSectionID(&eventinfo.pickedobjids);
    BufferString sectionname = emobject->sectionName(sid);
    if ( !sectionname.size() ) sectionname = sid;
    info += ", Section: "; info += sectionname;

    if ( as_[0]->id()<-1 )
	return;

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

    const EM::EMObject* emobj = em.getObject(oid);
    if ( emobj && !emobj->isFullyLoaded() )
    {
	par.set( sKeySections, displayedSections() );
	par.set( sKeyRowRange, displayedRowRange().start,
		 displayedRowRange().stop,
		 displayedRowRange().step );
	par.set( sKeyColRange, displayedColRange().start,
		  displayedColRange().stop,
		  displayedColRange().step );
    }

    par.set( sKeyEarthModelID, getMultiID() );
    par.setYN( sKeyTexture, usesTexture() );
    par.setYN( sKeyWireFrame, usesWireframe() );
    par.setYN( sKeyEdit, isEditingEnabled() );
    par.setYN( sKeyOnlyAtSections, getOnlyAtSectionsDisplay() );
    par.set( sKeyResolution, getResolution() );
    par.set( sKeyShift, getTranslation().z );
    par.set( sKey::Color, (int)nontexturecol.rgb() );

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle, str );
    }

    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );
	const int coltabid = getColTabID(attrib);
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );

    //par.set( sKeyColorTableID, coltab_->id() );
    //if ( saveids.indexOf(coltab_->id())==-1 )
	//saveids += coltab_->id();
    as_[0]->fillPar( par );
}


int EMObjectDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    if ( scene_ )
	setDisplayTransformation( scene_->getUTM2DisplayTransform() );

    if ( !par.get(sKeyEarthModelID,parmid) )
	return -1;

    par.get( sKeySections, parsections );
    par.get( sKeyRowRange, parrowrg.start, parrowrg.stop, parrowrg.step );
    par.get( sKeyColRange, parcolrg.start, parcolrg.stop, parcolrg.step );

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

    if ( !par.getYN( sKeyTexture, usestexture ) )
	usestexture = true;

    bool usewireframe = false;
    par.getYN( sKeyWireFrame, usewireframe );
    useWireframe( usewireframe );

    int resolution = 0;
    par.get( sKeyResolution, resolution );
    setResolution( resolution );

    bool filter = false;
    par.getYN( sKeyOnlyAtSections, filter );
    setOnlyAtSectionsDisplay(filter);

    Coord3 shift( 0, 0, 0 );
    par.get( sKeyShift, shift.z );
    setTranslation( shift );

    int nrattribs;
    if ( par.get( sKeyNrAttribs(), nrattribs ) ) //Current format
    {
	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    int coltabid = -1;
	    if ( attribpar->get( sKeyColTabID(), coltabid ) )
	    {
		visBase::DataObject* dataobj=visBase::DM().getObject(coltabid);
		if ( !dataobj ) return 0;

		mDynamicCastGet( visBase::VisColorTab*, coltab, dataobj );
		if ( !coltab ) coltabid=-1;
		coltabs_[attribnr]->unRef();
		coltabs_.replace( attribnr, coltab );

		for ( int idx=0; idx<sections.size(); idx++ )
		{
		    mDynamicCastGet( visBase::ParametricSurface*,psurf,
			    	     sections[idx]);
		    if ( psurf )
			psurf->setColorTab( attribnr, *coltab);
		}
	    }

	    as_[attribnr]->usePar( *attribpar );
	}
    }
    else //old format
    {
	as_[0]->usePar( par );
	visBase::VisColorTab* coltab;
	int coltabid = -1;
	par.get( sKeyColorTableID, coltabid );
	if ( coltabid>-1 )
	{
	    DataObject* dataobj = visBase::DM().getObject( coltabid );
	    if ( !dataobj ) return 0;

	    mDynamicCastGet( visBase::VisColorTab*, coltab, dataobj );
	    if ( !coltab ) return -1;
	    if ( coltabs_[0] ) coltabs_[0]->unRef();
	    coltabs_.replace( 0, coltab );
	    coltab->ref();
	    for ( int idx=0; idx<sections.size(); idx++ )
	    {
		mDynamicCastGet( visBase::ParametricSurface*,psurf,
				 sections[idx]);
		if ( psurf ) psurf->setColorTab( 0, *coltab);
	    }
	}
    }

    return 1;
}


NotifierAccess* EMObjectDisplay::getMovementNotification() { return &hasmoved; }


void EMObjectDisplay::updatePosAttrib(int attrib)
{
    const int attribindex = posattribs.indexOf(attrib);
    if ( attribindex==-1 ) return;

    EM::EMObject* emobject = em.getObject( oid );
    const TypeSet<EM::PosID>* pids = emobject->getPosAttribList(attrib);

    //Remove everything but material
    while ( posattribmarkers[attribindex]->size()>1 )
	posattribmarkers[attribindex]->removeObject(1);

    if ( !pids )
	return;

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


EM::PosID EMObjectDisplay::getPosAttribPosID( int attrib,
					      const TypeSet<int>& path ) const
{
    EM::PosID res(-1,-1,-1);
    const int attribidx = posattribs.indexOf(attrib);
    if ( attribidx<0 )
	return res;

    visBase::DataObjectGroup* group = posattribmarkers[attribidx];
    TypeSet<int> visids;
    for ( int idx=1; idx<group->size(); idx++ )
	visids += group->getObject( idx )->id();

    for ( int idx=0; idx<path.size(); idx++ )
    {
	const int index = visids.indexOf( path[idx] );
	if ( index==-1 )
	    continue;

	EM::EMObject* emobject = em.getObject( oid );
	const TypeSet<EM::PosID>* pids = emobject->getPosAttribList(attrib);
	res = (*pids)[index];
	break;
    }

    return res;
}
    

#define mEndLine \
{ \
    if ( cii<2 || ( cii>2 && line->getCoordIndex(cii-2)==-1 ) ) \
    { \
	while ( cii && line->getCoordIndex(cii-1)!=-1 ) \
	    line->getCoordinates()->removePos(line->getCoordIndex(--cii)); \
    } \
    else if ( cii && line->getCoordIndex(cii-1)!=-1 )\
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
	const Coord3 prevpos(horizon->getPos(sid,prevbid.getSerialized())); \
	Coord3 pos = prevpos; \
	if ( nextbid!=prevbid && prevpos.isDefined() ) \
	{ \
	    const Coord3 nextpos = \
		horizon->getPos(sid,nextbid.getSerialized()); \
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
    const EM::EMObject* emobject = em.getObject( oid );
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
	    int objectid = -1;
	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
		objectid = plane->id();
	    else
	    {
		mDynamicCastGet( const MPEDisplay*, mped, objs[idx] );
		if ( mped && mped->isDraggerShown() )
		{
		    CubeSampling cs;
		    if ( mped->getPlanePosition(cs) &&
			  ( cs.hrg.start.inl==cs.hrg.stop.inl ||
			    cs.hrg.start.crl==cs.hrg.stop.crl ))
			objectid = mped->id();
		}
	    }

	    if ( objectid==-1 )
		continue;

	    const int idy = intersectionlineids.indexOf(objectid);
	    if ( idy==-1 )
	    {
		linestoupdate += objectid;
		lineshouldexist += true;
	    }
	    else
	    {
		if ( whichobj==objectid && linestoupdate.indexOf(whichobj)==-1 )
		    linestoupdate += whichobj;

		lineshouldexist[idy] = true;
	    }
	}
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
	CubeSampling cs(false);
	mDynamicCastGet( PlaneDataDisplay*, plane,
			 visBase::DM().getObject(linestoupdate[idx]) );
	if ( plane )
	    cs = plane->getCubeSampling();
	else
	{
	    mDynamicCastGet( const MPEDisplay*, mped,
		    	     visBase::DM().getObject(linestoupdate[idx]));
	    mped->getPlanePosition(cs);
	}

	int lineidx = intersectionlineids.indexOf(linestoupdate[idx]);
	if ( lineidx==-1 )
	{
	    lineidx = intersectionlineids.size();
	    intersectionlineids += linestoupdate[idx];
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

	for ( int sectionidx=0; sectionidx<horizon->nrSections(); sectionidx++ )
	{
	    const EM::SectionID sid = horizon->sectionID(sectionidx);
	    const StepInterval<int> inlrg = horizon->geometry.rowRange(sid);
	    const StepInterval<int> crlrg = horizon->geometry.colRange(sid);

	    if ( cs.hrg.start.inl==cs.hrg.stop.inl )
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
