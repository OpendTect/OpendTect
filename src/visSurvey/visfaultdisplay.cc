/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.11 2008-05-15 20:28:25 cvskris Exp $";

#include "visfaultdisplay.h"

#include "emeditor.h"
#include "emfault.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "faultsticksurface.h"
#include "faulteditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "randcolor.h"
#include "survinfo.h"
#include "visdragger.h"
#include "visevent.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismultitexture2.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "visshapehints.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

#define mNearestKnotSize 4
#define mDefaultKnotSize 2

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : emfault_( 0 )
    , displaysurface_( 0 )
    , viseditor_( 0 )
    , faulteditor_( 0 )
    , eventcatcher_( 0 )
    , explicitsurface_( 0 )
    , displaytransform_( 0 )
    , mousepos_( Coord3::udf() )
    , shapehints_( visBase::ShapeHints::create() )
{
    setColor( getRandomColor( false ) );
    getMaterial()->setAmbience( 0.2 );
    shapehints_->ref();
    addChild( shapehints_->getInventorNode() );
    shapehints_->setVertexOrder( visBase::ShapeHints::CounterClockWise );
}


FaultDisplay::~FaultDisplay()
{
    setSceneEventCatcher( 0 );
    if ( viseditor_ ) viseditor_->unRef();

    if ( emfault_ ) MPE::engine().removeEditor( emfault_->id() );
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;

    if ( displaysurface_ )
	displaysurface_->unRef();

    delete explicitsurface_;

    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
       	emfault_->unRef();
    }

    if ( displaytransform_ ) displaytransform_->unRef();
    shapehints_->unRef();
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

    if ( viseditor_ ) viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultDisplay::getEMID() const
{ return emfault_ ? emfault_->id() : -1; }


#define mErrRet(s) { errmsg = s; return false; }

bool FaultDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
	emfault_->unRef();
    }

    emfault_ = 0;
    if ( faulteditor_ ) faulteditor_->unRef();
    faulteditor_ = 0;
    if ( viseditor_ ) viseditor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet( EM::Fault*, emfault, emobject.ptr() );
    if ( !emfault )
    {
	if ( displaysurface_ ) displaysurface_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    emfault_->change.notify( mCB(this,FaultDisplay,emChangeCB) );
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
    displaysurface_->touch( true );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	addChild( viseditor_->getInventorNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, fe, editor.ptr() );
    faulteditor_ =  fe;
    if ( faulteditor_ ) faulteditor_->ref();

    viseditor_->setEditor( faulteditor_ );
    
    displaysurface_->turnOn( true );

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
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );

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
    if ( !emfault_ || !faulteditor_ || !isOn() || eventcatcher_->isHandled() ||
	 !isSelected() )
	return;

    //if ( viseditor_ && !viseditor_->clickCB( cb ) )
	//return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
   
    CubeSampling mouseplanecs; 
    mouseplanecs.setEmpty();
    EM::PosID mousepid( EM::PosID::udf() );
    bool mouseondragger = false;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	mDynamicCastGet( visSurvey::PlaneDataDisplay*, plane, dataobj );
	if ( plane )
	{
	    mouseplanecs = plane->getCubeSampling();
	    break;
	}
    }

    Coord3 pos = eventinfo.displaypickedpos;
    pos = scene_->getZScaleTransform()->transformBack( pos ); 
    if ( displaytransform_ ) pos = displaytransform_->transformBack( pos ); 

    if ( mousepos_ != pos )
    {
	mousepos_ = pos;
	//updateKnotMarkerColor( pos );
    }

    if ( !pos.isDefined() )
	return;

    if ( eventinfo.type==visBase::MouseClick &&
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	 OD::leftMouseButton(eventinfo.buttonstate_) )
    {
	if ( !eventinfo.pressed )
	{
	    const EM::PosID pid =
		viseditor_->mouseClickDragger( eventinfo.pickedobjids );
	    if ( !pid.isUdf() )
	    {
		const int stick = RowCol(pid.subID()).row;
		if ( emfault_->geometry().nrKnots(pid.sectionID(),stick)==1 )
		{
		    emfault_->geometry().removeStick( pid.sectionID(), stick,
			    			      true );
		}
		else
		{
		    emfault_->geometry().removeKnot( pid.sectionID(),
			    			     pid.subID(), true );
		}

		displaysurface_->touch( false );
	    }
	}
	eventcatcher_->setHandled();
	return;
    }


    if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    EM::PosID nearestpid0, nearestpid1, insertpid;
    faulteditor_->getInteractionInfo( nearestpid0, nearestpid1, insertpid,
	    			      pos, SI().zFactor() );

    if ( insertpid.isUdf() )
	return;

    if ( nearestpid0.isUdf() )
    {
	//Add Stick
	Coord3 editnormal(0,0,1);
	if (  mouseplanecs.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0);
	else if ( mouseplanecs.defaultDir()==CubeSampling::Crl ) 
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	const RowCol rc( insertpid.subID() );
    
	emfault_->geometry().insertStick( insertpid.sectionID(), rc.row,
					  pos, editnormal, true );
	displaysurface_->touch( false );
	faulteditor_->editpositionchange.trigger();
    }
    else
    {
	emfault_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true );
	faulteditor_->editpositionchange.trigger();
    }

    eventcatcher_->setHandled();
}


void FaultDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( cbdata.event==EM::EMObjectCallbackData::BurstAlert ||
	 cbdata.event==EM::EMObjectCallbackData::SectionChange ||
	 cbdata.event==EM::EMObjectCallbackData::PositionChange )
    {
	displaysurface_->touch( false );
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


void FaultDisplay::showManipulator( bool yn )
{
    viseditor_->turnOn( yn );
}


bool  FaultDisplay::isManipulatorShown() const
{ return viseditor_->isOn(); }


int FaultDisplay::nrResolutions() const
{
    return texture_->canUseShading() ? 1 : 3;
}


void FaultDisplay::setResolution( int res )
{
    if ( texture_->canUseShading() )
	return;

    if ( res==resolution_ )
	return;

    resolution_ = res;
    texture_->clearAll();
}


bool FaultDisplay::getCacheValue( int attrib, int version, const Coord3& crd,
	                          float& value ) const
{ return true; }


void FaultDisplay::addCache()
{}

void FaultDisplay::removeCache( int attrib )
{}

void FaultDisplay::swapCache( int attr0, int attr1 )
{}

void FaultDisplay::emptyCache( int attrib )
{}

bool FaultDisplay::hasCache( int attrib ) const
{ return false; }


}; // namespace visSurvey
