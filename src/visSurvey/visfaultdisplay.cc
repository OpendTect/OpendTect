/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.9 2008-04-03 15:45:10 cvsjaap Exp $";

#include "visfaultdisplay.h"

#include "emeditor.h"
#include "emfault.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "faultsticksurface.h"
#include "iopar.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "survinfo.h"
#include "visdragger.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : VisualObjectImpl(true)
    , emfault_( 0 )
    , displaysurface_( 0 )
    , editor_( 0 )
    , eventcatcher_( 0 )
    , explicitsurface_( 0 )
    , displaytransform_( 0 )
    , mousedisplaypos_( Coord3::udf() )
    , editpid_( EM::PosID::udf() )
{
    setColor( getRandomColor( false ) );
    mouseplanecs_.setEmpty();
}


FaultDisplay::~FaultDisplay()
{
    setSceneEventCatcher( 0 );
    if ( editor_ ) editor_->unRef();

    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );

    if ( displaysurface_ )
	displaysurface_->unRef();

    delete explicitsurface_;

    if ( emfault_ ) emfault_->unRef();
    if ( displaytransform_ ) displaytransform_->unRef();
}


void FaultDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,FaultDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify( mCB(this,FaultDisplay,mouseCB) );
    }

    if ( editor_ ) editor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
	emfault_->unRef();

    emfault_ = 0;
    if ( editor_ ) editor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::Fault*, emfault, emobject.ptr() );
    if ( !emfault )
    {
	if ( displaysurface_ ) displaysurface_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    emfault_->ref();

    getMaterial()->setColor( emfault_->preferredColor() );
    if ( !emfault_->name().isEmpty() )
	setName( emfault_->name() );

    if ( !displaysurface_ )
    {
	displaysurface_ = visBase::GeomIndexedShape::create();
	displaysurface_->ref();
	displaysurface_->setDisplayTransformation( displaytransform_ );
	displaysurface_->setMaterial( 0 );
	displaysurface_->setSelectable( false );
	displaysurface_->setRightHandSystem( righthandsystem_ );
	addChild( displaysurface_->getInventorNode() );
    }

    if ( !explicitsurface_ )
    {
	const float zscale = SI().zFactor() * scene_->getZScale();
	explicitsurface_ = new Geometry::ExplFaultStickSurface( 0, zscale );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    explicitsurface_->setSurface( fss ); 
    displaysurface_->setSurface( explicitsurface_ );
    explicitsurface_->updateAll();
    displaysurface_->touch( false );

    if ( !editor_ )
    {
	editor_ = visSurvey::MPEEditor::create();
	editor_->ref();
	editor_->setSceneEventCatcher( eventcatcher_ );
	editor_->setDisplayTransformation( displaytransform_ );
	addChild( editor_->getInventorNode() );
    }

    editor_->setEditor( MPE::engine().getEditor( emid, true ) );
    
    displaysurface_->turnOn( true );
    showKnotMarkers( true );

    return true;
}


MultiID FaultDisplay::getMultiID() const
{
    return emfault_ ? emfault_->multiID() : MultiID();
}


void FaultDisplay::setColor( Color nc )
{
    if ( emfault_ ) emfault_->setPreferredColor(nc);
    getMaterial()->setColor( nc );
}


NotifierAccess* FaultDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultDisplay::getColor() const
{ return getMaterial()->getColor(); }


void FaultDisplay::display( bool sticks, bool panels )
{
    if ( explicitsurface_ )
	explicitsurface_->display( sticks, panels );
    if ( displaysurface_ )
	displaysurface_->touch( false );
}


bool FaultDisplay::areSticksDisplayed() const
{
    return explicitsurface_ ? explicitsurface_->areSticksDisplayed() : false;
}


bool FaultDisplay::arePanelsDisplayed() const
{
    return explicitsurface_ ? explicitsurface_->arePanelsDisplayed() : false;
}


void FaultDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID(), getMultiID() );
}


int FaultDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newmid;
    if ( par.get(sKeyEarthModelID(),newmid) )
    {
	EM::ObjectID emid = EM::EMM().getObjectID( newmid );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	if ( !emobject )
	{
	    PtrMan<Executor> loader = EM::EMM().objectLoader( newmid );
	    if ( loader ) loader->execute();
	    emid = EM::EMM().getObjectID( newmid );
	    emobject = EM::EMM().getObject( emid );
	}

	if ( emobject ) setEMID( emobject->id() );
    }

    return 1;
}


void FaultDisplay::setDisplayTransformation(visBase::Transformation* nt)
{
    if ( displaysurface_ ) displaysurface_->setDisplayTransformation( nt );
    if ( editor_ ) editor_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void FaultDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( displaysurface_ ) displaysurface_->setRightHandSystem( yn );
}


visBase::Transformation* FaultDisplay::getDisplayTransformation()
{ return displaytransform_; }


void FaultDisplay::mouseCB( CallBacker* cb )
{
    if ( !emfault_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    if ( editor_ && !editor_->clickCB( cb ) )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    
    mouseplanecs_.setEmpty();
    EM::PosID mousepid( EM::PosID::udf() );
    bool mouseondragger = false;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::Marker*, marker, dataobj );
	if ( marker )
	{
	    mousepid = getMarkerPid( marker->centerPos() );
	    break;
	}
	mDynamicCastGet( visBase::Dragger*, dragger, dataobj );
	if ( dragger )
	{
	    mouseondragger = true;
	    break;
	}
	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs_ = plane->getCubeSampling();
	    break;
	}
    }

    if ( mousedisplaypos_ != eventinfo.displaypickedpos )
    {
	mousedisplaypos_ = eventinfo.displaypickedpos;
	updateKnotMarkers();
    }

    if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) )
	return;
   
    if ( mouseondragger && OD::rightMouseButton(eventinfo.buttonstate_) )
    {
	setEditID( EM::PosID::udf() );
	updateKnotMarkers();
	eventcatcher_->setHandled();
	return;
    }

    if ( !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;
	
    if ( mousepid.objectID()!=-1 &&
	 !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
    {
	setEditID( mousepid );
	updateKnotMarkers();
	eventcatcher_->setHandled();
    }

    if ( mousepid.objectID()!=-1 &&
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
    {
	setEditID( EM::PosID::udf() );
	emfault_->geometry().removeKnot( mousepid.sectionID(),
					 mousepid.subID(), true );
	updateKnotMarkers();
	displaysurface_->touch( false );
	eventcatcher_->setHandled();
    }

    TypeSet<EM::PosID> knots;
    getNearestKnots( knots );

    Coord3 pos = mousedisplaypos_;
    pos = scene_->getZScaleTransform()->transformBack( pos ); 
    if ( displaytransform_ ) 
	pos = displaytransform_->transformBack( pos ); 

    if ( !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
    {
	emfault_->geometry().insertKnot( knots[2].sectionID(),
		mMAX(knots[2].subID(), knots[3].subID()), pos, true );
	updateKnotMarkers();
	displaysurface_->touch( false );
	eventcatcher_->setHandled();
    }

    Coord3 editnormal(0,0,1);
    if (  mouseplanecs_.defaultDir()==CubeSampling::Inl )
	editnormal = Coord3( SI().binID2Coord().rowDir(), 0);
    else if ( mouseplanecs_.defaultDir()==CubeSampling::Crl ) 
	editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

    if ( !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 OD::shiftKeyboardButton(eventinfo.buttonstate_) )
    {
	RowCol rc; rc.setSerialized( mMAX(knots[0].subID(),knots[1].subID()) );
	
	emfault_->geometry().insertStick( knots[0].sectionID(), rc.row,
					  pos, editnormal, true );
	updateKnotMarkers();
	displaysurface_->touch( false );
	eventcatcher_->setHandled();
    }
}


static int stickNrDist( const EM::PosID& knot1, const EM::PosID& knot2 )
{
   if ( knot1.objectID()!=knot2.objectID() || knot1.objectID()==-1 )
       return mUdf(int);
   if ( knot1.sectionID() != knot2.sectionID() )
       return mUdf(int);
   RowCol rc1; rc1.setSerialized( knot1.subID() );
   RowCol rc2; rc2.setSerialized( knot2.subID() );
   return abs( rc1.row-rc2.row );
}


bool FaultDisplay::segmentInPlane( const EM::PosID& knot1,
				   const EM::PosID& knot2,
				   const CubeSampling* plane ) const
{
    if ( !emfault_ || stickNrDist(knot1,knot2) )
	return false;
    if ( !plane )
	return true;

    RowCol rc; rc.setSerialized( knot1.subID() );
    Coord3 editnormal =
	emfault_->geometry().getEditPlaneNormal( knot1.sectionID(), rc.row );
    CubeSampling::Dir dir = plane->defaultDir();

    if ( dir==CubeSampling::Z && fabs(editnormal.z)<0.5 )
	return false;
    
    editnormal.z = 0;
    editnormal.normalize();
    const float abscos = fabs( SI().binID2Coord().rowDir().dot(editnormal) );

    if ( dir==CubeSampling::Inl && abscos<M_SQRT1_2 ||
	 dir==CubeSampling::Crl && abscos>=M_SQRT1_2 )
	return false;
   
    CubeSampling wideplane( *plane );
    wideplane.zrg.widen( 0.5*wideplane.zrg.step );
	    
    const Coord3 pos1 = emfault_->getPos( knot1 );
    const BinID bid1 = SI().transform( pos1 );
    if ( wideplane.hrg.includes(bid1) && wideplane.zrg.includes(pos1.z) )
	return true;	
    
    const Coord3 pos2 = emfault_->getPos( knot2 );
    const BinID bid2 = SI().transform( pos2 );
    if ( wideplane.hrg.includes(bid2) && wideplane.zrg.includes(pos2.z) )
	return true;	

    // TODO: intersecting segments with both end points outside cubesampling

    return false;
}


float FaultDisplay::nearestStickSegment( const Coord3& displaypos,
					 Coord3& nearestpos,
       					 EM::PosID& knot1, EM::PosID& knot2,
					 const CubeSampling* plane,
					 const bool* verticaldir,
					 const EM::PosID* stickpid ) const
{
    nearestpos = Coord3::udf();
    knot1=EM::PosID::udf(); knot2=EM::PosID::udf();
    
    EM::PosID prevpid( EM::PosID::udf() );
    Coord3 prevpos;
    
    float mindist=MAXFLOAT; float mincos=0;

    if ( !emfault_ )
	return mindist;

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();

	if ( pid.objectID() != -1 )
	{
	    const bool onverticalstick = 
		emfault_->geometry().areSticksVertical( pid.sectionID() );

	    if ( verticaldir && *verticaldir!=onverticalstick )
	       continue;	
	    if ( stickpid && stickNrDist(*stickpid,pid) )
		continue;
	}

	Coord3 pos = emfault_->getPos( pid ); 
	if ( displaytransform_ ) 
	    pos = displaytransform_->transform( pos ); 
	pos = scene_->getZScaleTransform()->transform( pos ); 
	
	const Coord3 vec1 = displaypos - prevpos;
	const Coord3 vec2 = displaypos - pos;
	const float d1 = vec1.abs();
	const float d2 = vec2.abs();
	
	if ( stickNrDist(prevpid,pid) )
	{
	    if ( d1<=mindist && segmentInPlane(prevpid,prevpid,plane) )
	    {							
		mindist=d1; mincos=0;			 /* Stick end */
		nearestpos = prevpos;
		knot1=prevpid; knot2=knot1;
		knot2.setSubID( knot1.subID()+1 );
	    }	

	    if ( pid.objectID() == -1 )
		break;

	    if ( d2 <= mindist && segmentInPlane(pid,pid,plane) )			
	    {
		mindist=d2; mincos=0;			 /* Stick begin */
		nearestpos = pos;
		knot1=pid; knot2=knot1;
		knot2.setSubID( knot1.subID()-1 );
	    }	
	}
	else if ( segmentInPlane(prevpid,pid,plane) )
	{
	    const Coord3 vec0 = prevpos - pos;		/* Stick segment */
	    const float d0 = vec0.abs();
	    const float cos1 = d1*d0 ?  vec1.dot(vec0)/(d1*d0) : 0;
	    const float cos2 = d2*d0 ? -vec2.dot(vec0)/(d2*d0) : 0;

	    const Coord3 cross = vec1.cross( vec2 );
	    float dist = d0 ? cross.abs()/d0 : d1;
	    float cos = 0;

	    if ( cos1 > 0 )
	    {
		dist=d1; cos=cos1;
	    }
	    else if ( cos2 > 0 )
	    {
		dist=d2; cos=cos2;
	    }

	    if ( dist<mindist || dist==mindist && cos<mincos )
	    {
		mindist = dist; mincos = cos;
		knot1 = d1 < d2 ? prevpid : pid;
		knot2 = d1 < d2 ? pid : prevpid;
		
		nearestpos = dist==d1 ? prevpos :
		   ( dist==d2 ? pos : pos + vec0*(sqrt(d2*d2-dist*dist)/d0) ); 
	    }
	}
	prevpid=pid; prevpos=pos;
    }

    return mindist;
}


void FaultDisplay::getNearestKnots( TypeSet<EM::PosID>& knots ) const
{
    knots.erase();
    knots.setSize( 4, EM::PosID::udf() );

    if ( !emfault_ || mouseplanecs_.isEmpty() )
	return;

    EM::PosID adjstickpid;
    const bool isinlcrl = mouseplanecs_.defaultDir() != CubeSampling::Z;

    TypeSet<Coord3> vec, pos(3,Coord3::udf());
    TypeSet<float> len, cos;

    for ( int idx=0; idx<3; idx++ )
    {
	const bool* isinlcrlptr = idx ? 0 : &isinlcrl;
	const EM::PosID* adjstickpidptr = idx ? &adjstickpid : 0;
	
	nearestStickSegment( mousedisplaypos_, pos[idx], knots[idx],
			     knots[idx+1], 0, isinlcrlptr, adjstickpidptr );

	vec += ( idx ? pos[idx] : mousedisplaypos_ ) - pos[0];
	len += vec[idx].abs();
	const float denom = len[idx] * len[0];
	cos += pos[idx].isDefined() && denom ? vec[idx].dot(vec[0])/denom : 0;

	if ( knots[0].objectID()==-1 )
	    knots[0] = EM::PosID( -1, 0, 0 );

	adjstickpid = knots[0];
	RowCol rc; rc.setSerialized( adjstickpid.subID() );
	rc.row += !idx || (idx==2 && pos[2].isDefined()) ? -1 : 1; 
	adjstickpid.setSubID( rc.getSerialized() );
    }
    
    if ( cos[2] > cos[1] )
	knots[1] = knots[2];

    if ( knots[1].objectID() == -1 )
	knots[1] = adjstickpid;
    
    Coord3 dummycrd;
    nearestStickSegment( mousedisplaypos_, dummycrd, knots[2], knots[3], 
	    		 &mouseplanecs_ );
}


EM::PosID FaultDisplay::getMarkerPid( const Coord3& markerpos )
{
    if ( !emfault_ )
	return EM::PosID::udf();

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    return EM::PosID::udf();

	const Coord3 pos = emfault_->getPos( pid );
	if ( pos.isDefined() && Coord(pos)==markerpos 
			     && fabs(pos.z-markerpos.z)<1e-5 )
	    return pid;
    }
}


void FaultDisplay::setEditID( const EM::PosID& pid )
{
    if ( !editor_ ) return;
    
    if ( pid.objectID() < 0 )
    {
	RefMan<MPE::ObjectEditor> emeditor = editor_->getMPEEditor();
	if ( emeditor ) 
	    emeditor->removeEditID( editpid_ );

	editpid_ = EM::PosID::udf();
    }
    else if ( pid != editpid_ )
    {
	editor_->setMarkerSize(3);
	editor_->mouseClick( pid, false, false, false );
	editpid_ = pid;
    }
}


void FaultDisplay::showKnotMarkers( bool yn )
{
    if ( yn && !knotmarkers_.size() )
    {
	for ( int idx=0; idx<3; idx++ )
	{
	    visBase::DataObjectGroup* group=visBase::DataObjectGroup::create();
	    group->ref();
	    addChild( group->getInventorNode() );
	    knotmarkers_ += group;
	    visBase::Material* knotmat = visBase::Material::create();
	    group->addObject( knotmat );
	    if ( idx==0 )
		knotmat->setColor( Color(255,255,255) );
	    else if ( idx==1 )
		knotmat->setColor( Color(0,255,255) );
	    else
		knotmat->setColor( Color(0,255,0) );
	}
	updateKnotMarkers();
    }
    if ( !yn )
    {
	for ( int idx=knotmarkers_.size()-1; idx>=0; idx-- ) 
	{
	    removeChild( knotmarkers_[idx]->getInventorNode() );
	    knotmarkers_[idx]->unRef();
	    knotmarkers_.remove( idx );
	}
    }
}


void FaultDisplay::updateKnotMarkers()
{
    if ( !emfault_ )
	return;

    for ( int idx=0; idx<knotmarkers_.size(); idx++ )
    {
	while ( knotmarkers_[idx]->size() > 1 )
	    knotmarkers_[idx]->removeObject( 1 );
    }
    if ( editor_ )
	editor_->turnOn( false );
    
    TypeSet<EM::PosID> nearestknots;
    getNearestKnots( nearestknots );

    PtrMan<EM::EMObjectIterator> iter = emfault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const Coord3 pos = emfault_->getPos( pid );

	visBase::Marker* marker = visBase::Marker::create();
	marker->setMarkerStyle( emfault_->getPosAttrMarkerStyle(0) );
	marker->setMaterial( 0 );
	marker->setDisplayTransformation( displaytransform_ );
	marker->setCenterPos(pos);
	marker->setScreenSize( 2 );

	if ( pid == nearestknots[2] )
	{
	    knotmarkers_[2]->addObject( marker );
	    marker->setScreenSize( marker->getScreenSize()+2 );
	}
	else if ( pid == nearestknots[3] )
	{
	    knotmarkers_[2]->addObject( marker );
	    marker->setScreenSize( marker->getScreenSize()+1 );
	}
	else if ( !stickNrDist(pid,nearestknots[0]) )
	{
	    knotmarkers_[1]->addObject( marker );
	    marker->setScreenSize( marker->getScreenSize()+1 );
	}
	else if ( !stickNrDist(pid,nearestknots[1]) )
	{
	    knotmarkers_[1]->addObject( marker );
	}
	else
	{
	    if ( pid==editpid_ && editor_ )
	    {
		editor_->turnOn( true );
		marker->setScreenSize( 0 );
	    }
	    knotmarkers_[0]->addObject( marker );
	}
    }
}


}; // namespace visSurvey
