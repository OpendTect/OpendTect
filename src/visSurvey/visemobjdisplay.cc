/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: visemobjdisplay.cc,v 1.25 2005-05-20 12:53:57 cvsnanne Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: visemobjdisplay.cc,v 1.25 2005-05-20 12:53:57 cvsnanne Exp $";


#include "vissurvemobj.h"

#include "attribsel.h"
#include "binidvalset.h"
#include "cubicbeziersurface.h"
#include "emmanager.h"
#include "emobject.h"
#include "mpeengine.h"
#include "emeditor.h"
#include "viscubicbeziersurface.h"
#include "visdataman.h"
#include "visevent.h"
#include "visparametricsurface.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "survinfo.h"
#include "iopar.h"

#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"



mCreateFactoryEntry( visSurvey::EMObjectDisplay );

visBase::FactoryEntry visSurvey::EMObjectDisplay::oldnameentry(
	(FactPtr) visSurvey::EMObjectDisplay::create,
	"visSurvey::SurfaceDisplay");

namespace visSurvey
{

const char* EMObjectDisplay::earthmodelidstr = "EarthModel ID";
const char* EMObjectDisplay::texturestr = "Use texture";
const char* EMObjectDisplay::colortabidstr = "ColorTable ID";
const char* EMObjectDisplay::shiftstr = "Shift";
const char* EMObjectDisplay::editingstr = "Edit";
const char* EMObjectDisplay::wireframestr = "WireFrame on";
const char* EMObjectDisplay::resolutionstr = "Resolution";
const char* EMObjectDisplay::colorstr = "Color";

EMObjectDisplay::EMObjectDisplay()
    : VisualObjectImpl(true)
    , em(EM::EMM())
    , mid(-1)
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , curtextureidx(0)
    , usestexture(true)
    , editor(0)
    , eventcatcher(0)
    , transformation(0)
    , translation(0)
    , coltab_(visBase::VisColorTab::create())
{
    coltab_->ref();
}


EMObjectDisplay::~EMObjectDisplay()
{
    delete &as;
    delete &colas;
    if ( transformation ) transformation->unRef();

    setSceneEventCatcher( 0 );
    if ( coltab_ ) coltab_->unRef();

    const EM::ObjectID objid = em.multiID2ObjectID( mid );
    const int trackeridx = MPE::engine().getTrackerByObject( objid );
    if ( trackeridx >= 0 )
	MPE::engine().removeTracker( trackeridx );
    MPE::engine().removeEditor( objid );

    removeAll();
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
    if ( !isOn() || eventcatcher->isEventHandled() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.pickedobjids.indexOf(id())==-1 )
	return;

    bool keycb = false;
    bool mousecb = false;
    if ( eventinfo.type == visBase::Keyboard )
	keycb = eventinfo.key=='n' && eventinfo.pressed;
    else if ( eventinfo.type == visBase::MouseClick )
	mousecb = !eventinfo.mousebutton && eventinfo.pressed;

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

    if ( mousecb )
    {
	MPE::ObjectEditor* mpeeditor = 
			    MPE::engine().getEditor(emobject->id(),false);
	if ( editor && mpeeditor )
	{
	    TypeSet<EM::PosID> pids; pids += closestnode;
	    mpeeditor->setEditIDs( pids );
	}
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


void EMObjectDisplay::removeAll()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
    }

    sections.erase();
    sectionids.erase();

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( emobject )
    {
	emobject->notifier.remove( mCB(this,EMObjectDisplay,emSectionChangeCB));
	emobject->unRef();
    }
    if ( editor ) editor->unRef();
    if ( translation )
    {
	removeChild( translation->getInventorNode() );
	translation->unRef();
	translation = 0;
    }
}


bool EMObjectDisplay::setEMObject( const MultiID& newmid )
{
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(newmid) );
    if ( !emobject ) return false;

    if ( sections.size() ) removeAll();

    mid = newmid;
    emobject->ref();
    emobject->notifier.notify( mCB(this,EMObjectDisplay,emSectionChangeCB) );
    return updateFromEM();
}


bool EMObjectDisplay::updateFromEM()
{ 
    if ( sections.size() ) removeAll();

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    if ( !emobject ) return false;

    setName( emobject->name() );

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
	addSection( emobject->sectionID(idx) );

    const EM::ObjectID objid = em.multiID2ObjectID(mid);
    if ( MPE::engine().getEditor(objid,false) )
	enableEditing(true);

    nontexturecol = emobject->preferredColor();
    getMaterial()->setColor( nontexturecol );

    const bool hastracker = MPE::engine().getTrackerByObject(objid) >= 0;
    useWireframe( hastracker );
    useTexture( hastracker ? false : usestexture );
    setResolution( hastracker ? nrResolutions()-1 : 0 );

    return true;
}


bool EMObjectDisplay::addSection( EM::SectionID sectionid )
{
    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    Geometry::Element* ge = const_cast<Geometry::Element*>(
	const_cast<const EM::EMObject*>(emobject)->getElement(sectionid));
    if ( !ge ) return false;

    visBase::VisualObject* vo = createSection( ge );
    if ( !vo ) return false;

    vo->ref();
    vo->setMaterial( 0 );
    vo->setDisplayTransformation( transformation );
    addChild( vo->getInventorNode() );

    sections += vo;
    sectionids += sectionid;

    mDynamicCastGet(visBase::ParametricSurface*,psurf,vo)
    if ( psurf )
	psurf->setColorTab( *coltab_ );

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


bool EMObjectDisplay::hasColor() const
{
    return !usesTexture();
}


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
	setSelSpec( AttribSelSpec(0,AttribSelSpec::attribNotSel) );
    else
    {
	BufferString attrnm = emsurf->auxdata.auxDataName( textureidx );
	setSelSpec( AttribSelSpec(attrnm,AttribSelSpec::otherAttrib) );
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


const AttribSelSpec* EMObjectDisplay::getSelSpec() const
{ return &as; }


void EMObjectDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    removeAttribCache();
    as = as_;
}


const ColorAttribSel* EMObjectDisplay::getColorSelSpec() const
{
    return getAttributeFormat() < 0 ? 0 : &colas;
}


void EMObjectDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void EMObjectDisplay::setDepthAsAttrib()
{
    as.set( "", AttribSelSpec::noAttrib, false, "" );

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
    return as.id() == AttribSelSpec::otherAttrib && ref && *ref;
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


bool EMObjectDisplay::usesWireframe() const
{
    if ( !sections.size() )
	return false;

    mDynamicCastGet(const visBase::CubicBezierSurface*,cbs,sections[0]);
    if ( cbs ) return cbs->usesWireframe();

    mDynamicCastGet(const visBase::ParametricSurface*,ps,sections[0]);
    if ( ps ) return ps->usesWireframe();

    return false;
}


void EMObjectDisplay::useWireframe( bool yn )
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	mDynamicCastGet(visBase::CubicBezierSurface*,cbs,sections[idx]);
	if ( cbs ) cbs->useWireframe( yn );

	mDynamicCastGet(visBase::ParametricSurface*,ps,sections[idx]);
	if ( ps ) ps->useWireframe( yn );
    }
}


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


void EMObjectDisplay::emSectionChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event!=EM::EMObjectCallbackData::SectionChange )
	return;

    EM::EMObject* emobject = em.getObject( em.multiID2ObjectID(mid) );
    mDynamicCastGet(EM::Surface*,emsurface,emobject)
    if ( !emsurface ) return;

    const EM::SectionID sectionid = cbdata.pid0.sectionID();
    if ( emsurface->geometry.hasSection(sectionid) )
    {
	addSection( sectionid );
	useWireframe( usesWireframe() );
	setResolution( getResolution() );
    }
    else
    {
	const int idx = sectionids.indexOf( sectionid );
	if ( idx < 0 ) return;
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
	sections.remove( idx );
	sectionids.remove(idx);
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
    return coltab_->id();
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

    par.set( earthmodelidstr, mid );
    par.setYN( texturestr, usesTexture() );
    par.setYN( wireframestr, usesWireframe() );
    par.setYN( editingstr, isEditingEnabled() );
    par.set( resolutionstr, getResolution() );
    par.set( shiftstr, getTranslation().z );
    par.set( colorstr, (int)nontexturecol.rgb() );
    par.set( colortabidstr, coltab_->id() );
    if ( saveids.indexOf(coltab_->id())==-1 )
	saveids += coltab_->id();

    as.fillPar( par );
    colas.fillPar( par );

}


int EMObjectDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    setDisplayTransformation( SPM().getUTM2DisplayTransform() );

    MultiID surfid;
    if ( !par.get(earthmodelidstr,surfid) )
	return -1;

    as.usePar( par );
    colas.usePar( par );

    //Editing may not be moved further down, since the enableEditing call
    //will change the wireframe, resolution++
    bool enableedit = false;
    par.getYN( editingstr, enableedit );
    enableEditing( enableedit );

    if ( !par.get(colorstr,(int&)nontexturecol.rgb()) )
	nontexturecol = getMaterial()->getColor();

    bool usetext = true;
    par.getYN( texturestr, usetext );
    useTexture( usetext );

    bool usewireframe = false;
    par.getYN( wireframestr, usewireframe );
    useWireframe( usewireframe );

    int resolution = 0;
    par.get( resolutionstr, resolution );
    setResolution( resolution );

    visBase::VisColorTab* coltab;
    int coltabid = -1;
    par.get( colortabidstr, coltabid );
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
    par.get( shiftstr, shift.z );
    setTranslation( shift );

    mid = surfid;

    return 1;
}


}; // namespace visSurvey
