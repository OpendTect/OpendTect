/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "visfaultsticksetdisplay.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "faultstickseteditor.h"
#include "iopar.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "zdomain.h"

mCreateFactoryEntry( visSurvey::FaultStickSetDisplay );

namespace visSurvey
{

FaultStickSetDisplay::FaultStickSetDisplay()
    : VisualObjectImpl(true)
    , emfss_(0)
    , eventcatcher_(0)
    , displaytransform_(0)
    , colorchange(this)
    , displaymodechange(this)
    , viseditor_(0)
    , fsseditor_(0)
    , activesticknr_( mUdf(int) )
    , sticks_( visBase::IndexedPolyLine3D::create() )
    , activestick_( visBase::IndexedPolyLine3D::create() )
    , showmanipulator_( false )
    , activestickpickstyle_( visBase::PickStyle::create() )
    , stickspickstyle_( visBase::PickStyle::create() )
    , displayonlyatsections_( false )
    , stickselectmode_( false )
{
    activestickpickstyle_->ref();
    activestickpickstyle_->setStyle( visBase::PickStyle::Unpickable );
    stickspickstyle_->ref();
    stickspickstyle_->setStyle( visBase::PickStyle::Unpickable );
    
    sticks_->ref();
    sticks_->setMaterial( 0 );
    sticks_->setRadius( 1, true );
    addChild( sticks_->getInventorNode() );
    activestick_->ref();
    activestick_->setMaterial( 0 );
    activestick_->setRadius( 1.5, true );
    addChild( activestick_->getInventorNode() );
    getMaterial()->setAmbience( 0.2 );

    sticks_->insertNode( stickspickstyle_->getInventorNode() );
    activestick_->insertNode( activestickpickstyle_->getInventorNode() );

    for ( int idx=0; idx<3; idx++ )
    {
	visBase::DataObjectGroup* group=visBase::DataObjectGroup::create();
	group->ref();
	addChild( group->getInventorNode() );
	knotmarkers_ += group;
	visBase::Material* knotmat = visBase::Material::create();
	group->addObject( knotmat );
	knotmat->setColor( idx ? Color(0,255,0) : Color(255,0,255) );
    }
}


FaultStickSetDisplay::~FaultStickSetDisplay()
{
    if ( scene_ && scene_->getPolySelection() &&
	 scene_->getPolySelection()->polygonFinished() )
    {
	const CallBack cb = mCB(this, FaultStickSetDisplay, polygonFinishedCB);
	scene_->getPolySelection()->polygonFinished()->remove( cb );
    }

    setSceneEventCatcher( 0 );
    showManipulator( false );


    if ( viseditor_ )
	viseditor_->unRef();
    if ( fsseditor_ )
	fsseditor_->unRef();
    if ( emfss_ )
	MPE::engine().removeEditor( emfss_->id() );
    fsseditor_ = 0;

    if ( emfss_ )
    {
	emfss_->change.remove( mCB(this,FaultStickSetDisplay,emChangeCB) );
       	emfss_->unRef();
	emfss_ = 0;
    }

    if ( displaytransform_ ) displaytransform_->unRef();

    sticks_->unRef();
    activestick_->unRef();
    stickspickstyle_->unRef();
    activestickpickstyle_->unRef();

    for ( int idx=knotmarkers_.size()-1; idx>=0; idx-- )
    {
	removeChild( knotmarkers_[idx]->getInventorNode() );
	knotmarkers_[idx]->unRef();
	knotmarkers_.remove( idx );
    }

    deepErase( stickintersectpoints_ );
}


void FaultStickSetDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
				    mCB(this,FaultStickSetDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = vec;
    
    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
				    mCB(this,FaultStickSetDisplay,mouseCB) );
    }

    if ( viseditor_ )
	viseditor_->setSceneEventCatcher( eventcatcher_ );
}


EM::ObjectID FaultStickSetDisplay::getEMID() const
{ return emfss_ ? emfss_->id() : -1; }


#define mSetStickIntersectPointColor( color ) \
    ((visBase::Material*) knotmarkers_[2]->getObject(0))->setColor(color);

#define mErrRet(s) { errmsg_ = s; return false; }

bool FaultStickSetDisplay::setEMID( const EM::ObjectID& emid )
{
    if ( emfss_ )
    {
	emfss_->change.remove( mCB(this,FaultStickSetDisplay,emChangeCB) );
	emfss_->unRef();
    }

    emfss_ = 0;
    if ( fsseditor_ )
    {
	fsseditor_->setEditIDs( 0 );
	fsseditor_->unRef();
    }
    fsseditor_ = 0;
    if ( viseditor_ )
	viseditor_->setEditor( (MPE::ObjectEditor*) 0 );

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return false;

    emfss_ = emfss;
    emfss_->change.notify( mCB(this,FaultStickSetDisplay,emChangeCB) );
    emfss_->ref();

    if ( !emfss_->name().isEmpty() )
	setName( emfss_->name() );

    if ( !viseditor_ )
    {
	viseditor_ = MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	viseditor_->sower().setIfDragInvertMask();
	insertChild( childIndex(sticks_->getInventorNode()),
		     viseditor_->getInventorNode() );
    }
    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    fsseditor_ =  fsseditor;
    if ( fsseditor_ )
    {
	fsseditor_->ref();
	fsseditor_->setEditIDs( &editpids_ );
    }


    viseditor_->setEditor( fsseditor_ );

    getMaterial()->setColor( emfss_->preferredColor() );
    mSetStickIntersectPointColor( emfss_->preferredColor() );
    viseditor_->setMarkerSize(3);

    updateSticks();
    updateKnotMarkers();
    return true;
}


MultiID FaultStickSetDisplay::getMultiID() const
{
    return emfss_ ? emfss_->multiID() : MultiID();
}


void FaultStickSetDisplay::setColor( Color nc )
{
    if ( emfss_ )
	emfss_->setPreferredColor( nc );
    else
	getMaterial()->setColor( nc );

    colorchange.trigger();
}


NotifierAccess* FaultStickSetDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultStickSetDisplay::getColor() const
{ return getMaterial()->getColor(); }


void FaultStickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );

    sticks_->setDisplayTransformation( nt );
    activestick_->setDisplayTransformation( nt );

    for ( int idx=0; idx<knotmarkers_.size(); idx++ )
	knotmarkers_[idx]->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


const mVisTrans* FaultStickSetDisplay::getDisplayTransformation() const
{ return displaytransform_; }


void FaultStickSetDisplay::updateEditPids()
{
    if ( !emfss_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    editpids_.erase();

    for ( int sidx=0; !stickselectmode_ && sidx<emfss_->nrSections(); sidx++ )
    {
	int sid = emfss_->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( fss->isStickHidden(rc.row) )
		continue;

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		editpids_ += EM::PosID( emfss_->id(), sid, rc.toInt64() );
	    }
	}
    }
    if ( fsseditor_ )
	fsseditor_->editpositionchange.trigger();
}


void FaultStickSetDisplay::updateSticks( bool activeonly )
{
    if ( !emfss_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    visBase::IndexedPolyLine3D* poly = activeonly ? activestick_ : sticks_;

    poly->removeCoordIndexAfter(-1);
    poly->getCoordinates()->removeAfter(-1);
    int cii = 0;

    for ( int sidx=0; sidx<emfss_->nrSections(); sidx++ )
    {
	int sid = emfss_->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    if ( activeonly && rc.row!=activesticknr_ )
		continue;

	    if ( fss->isStickHidden(rc.row) )
		continue;

	    Seis2DDisplay* s2dd = 0;
	    if ( emfss_->geometry().pickedOn2DLine(sid, rc.row) )
	    { 
		const char* lnm = emfss_->geometry().pickedName( sid, rc.row );
		const MultiID* lset =
			    emfss_->geometry().pickedMultiID( sid, rc.row );
		if ( lset )
		    s2dd = Seis2DDisplay::getSeis2DDisplay( *lset, lnm );
	    }

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    if ( !colrg.width() )
	    {
		rc.col = colrg.start;
		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.x += inlCrlSystem()->inlDistance() * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    poly->setCoordIndex( cii++, ci );
		}
		poly->setCoordIndex( cii++, -1 );

		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.y += inlCrlSystem()->inlDistance() * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    poly->setCoordIndex( cii++, ci );
		}
		poly->setCoordIndex( cii++, -1 );

		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.z += inlCrlSystem()->zStep() * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    poly->setCoordIndex( cii++, ci );
		}
		poly->setCoordIndex( cii++, -1 );
		continue;
	    }

	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		const Coord3 pos1 = fss->getKnot( rc );
		int ci = poly->getCoordinates()->addPos( pos1 );
		poly->setCoordIndex( cii++, ci );
		if ( !s2dd || rc.col==colrg.stop )
		    continue;

		RowCol nextrc = rc;
		nextrc.col += colrg.step;
		const Coord3 pos2 = fss->getKnot( nextrc );
		int trc11, trc12, trc21, trc22;
		float dummy;
		if ( s2dd->getNearestSegment(pos1,true,trc11,trc12,dummy) < 0 )
		    continue;
		if ( s2dd->getNearestSegment(pos2,true,trc21,trc22,dummy) < 0 )
		    continue;

		const int dir = trc11<=trc21 ? 1 : -1;
		const int trcnr1 = dir>0 ? trc12 : trc11;
		const int trcnr2 = dir>0 ? trc22 : trc21;

		double totarclen = 0.0;
		Coord prevpos = pos1;
		for ( int trcnr=trcnr1; dir*trcnr<=dir*trcnr2; trcnr+=dir )
		{
		    const Coord curpos = trcnr==trcnr2 ? (Coord) pos2
						       : s2dd->getCoord(trcnr);
		    if ( !curpos.isDefined() )
			continue;
		    totarclen += prevpos.distTo( curpos );
		    prevpos = curpos;
		}

		prevpos = pos1;
		int prevnr = trcnr1;
		Coord curpos = prevpos;
		int curnr = prevnr;
		double arclen = 0.0;
		double partarclen = 0.0;

		for ( int trcnr=trcnr1; dir*trcnr<=dir*trcnr2; trcnr+=dir )
		{
		    const Coord nextpos = trcnr==trcnr2 ? (Coord) pos2
							: s2dd->getCoord(trcnr);
		    if ( !nextpos.isDefined() )
			continue;

		    const double dist = curpos.distTo( nextpos );
		    partarclen += dist;
		    arclen += dist;
		    const double basedist = prevpos.distTo(nextpos);
		    if ( partarclen-basedist>0.001*partarclen ) 
		    {
			double maxperpdev = 0.0;
			partarclen = dist;
			for ( int backtrcnr=trcnr-dir;
			      dir*backtrcnr>dir*prevnr; backtrcnr-=dir )
			{
			    const Coord backpos = s2dd->getCoord( backtrcnr );
			    if ( !backpos.isDefined() )
				continue;

			    const double prevdist = backpos.distTo( prevpos );
			    const double nextdist = backpos.distTo( nextpos );
			    const double sp = (prevdist+nextdist+basedist) / 2;
			    const double dev = (sp-prevdist)*(sp-nextdist)*
					       (sp-basedist)*sp;

			    if ( dev < maxperpdev )
				break;

			    maxperpdev = dev;
			    partarclen += backpos.distTo( curpos ); 
			    curpos = backpos;
			    curnr = backtrcnr;
			}

			const double frac = (arclen-partarclen) / totarclen;
			if ( frac>mDefEps && 1.0-frac>mDefEps )
			{
			    const Coord3 pos( curpos,
					      (1-frac)*pos1.z+frac*pos2.z );
			    ci = poly->getCoordinates()->addPos( pos );
			    poly->setCoordIndex( cii++, ci );
			}

			prevpos = curpos;
			prevnr = curnr;
		    }
		    curpos = nextpos;
		    curnr = trcnr;
		}
	    }
	    poly->setCoordIndex( cii++, -1 );
	}
    }
    if ( !activeonly )
	updateSticks( true );
}


Coord3 FaultStickSetDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;  
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    pos = scene_->getZScaleTransform()->transformBack( pos ); 
	if ( displaytransform_ )
	    pos = displaytransform_->transformBack( pos ); 
    }
    return pos;
}


static float zdragoffset = 0;

#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getZStretch() : inlCrlSystem()->zScale() )

#define mSetUserInteractionEnd() \
    if ( !viseditor_->sower().moreToSow() ) \
	EM::EMM().undo().setUserInteractionEnd( \
					EM::EMM().undo().currentEventID() );

void FaultStickSetDisplay::mouseCB( CallBacker* cb )
{
    if ( stickselectmode_ )
	return stickSelectCB( cb );

    if ( !emfss_ || !fsseditor_ || !viseditor_ || !isOn() ||
	 eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    fsseditor_->setSowingPivot( disp2world(viseditor_->sower().pivotPos()) );
    if ( viseditor_->sower().accept(eventinfo) )
	return;

    const EM::PosID mousepid =
		    viseditor_->mouseClickDragger( eventinfo.pickedobjids );

    EM::FaultStickSetGeometry& fssg = emfss_->geometry();

    PlaneDataDisplay* plane = 0;
    Seis2DDisplay* s2dd = 0;
    HorizonDisplay* hordisp = 0;
    const MultiID* pickedmid = 0;
    const char* pickednm = 0;
    PtrMan<Coord3> normal = 0;
    PtrMan<const MultiID> horid;
    BufferString horshiftname;
    Coord3 pos;

    if ( !mousepid.isUdf() )
    {
	const int sticknr = RowCol( mousepid.subID() ).row;
	pos = emfss_->getPos( mousepid );
	pickedmid = fssg.pickedMultiID( mousepid.sectionID(), sticknr );
	pickednm = fssg.pickedName( mousepid.sectionID(), sticknr );
	zdragoffset = 0;
    }
    else
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	    mDynamicCast(Seis2DDisplay*,s2dd,dataobj);
	    if ( s2dd )
	    {
		pickedmid = &s2dd->lineSetID();
		pickednm = s2dd->name();
		break;
	    }
	    mDynamicCast(HorizonDisplay*,hordisp,dataobj);
	    if ( hordisp )
	    {
		horid = new MultiID( hordisp->getMultiID() );
		pickedmid = horid;
		horshiftname = hordisp->getTranslation().z *
		    scene_->zDomainInfo().userFactor();
		pickednm = horshiftname.buf();
		break;
	    }
	    mDynamicCast(PlaneDataDisplay*,plane,dataobj);
	    if ( plane )
	    {
		normal = new Coord3( plane->getNormal(Coord3::udf()) );
		break;	
	    }
	    mDynamicCastGet(FaultStickSetDisplay*,fssd,dataobj);
	    if ( fssd )
		return;
	}

	if ( !s2dd && !plane && !hordisp )
	{
	    setActiveStick( EM::PosID::udf() );
	    return;
	}

	pos = disp2world( eventinfo.displaypickedpos );
    }

    EM::PosID insertpid;
    fsseditor_->setZScale( mZScale() );
    fsseditor_->getInteractionInfo(insertpid, pickedmid, pickednm, pos, normal);

    if ( mousepid.isUdf() && !viseditor_->isDragging() )
	setActiveStick( insertpid );

    if ( locked_ || !pos.isDefined() ||
	 eventinfo.type!=visBase::MouseClick || viseditor_->isDragging() )
	return;

    if ( !mousepid.isUdf() )
    {
	fsseditor_->setLastClicked( mousepid );
	setActiveStick( mousepid );
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) || 
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;


    if ( !mousepid.isUdf() && OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	 !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
    {
	// Remove knot/stick
	eventcatcher_->setHandled();
	if ( eventinfo.pressed )
	    return;

	editpids_.erase();
	const int rmnr = RowCol(mousepid.subID()).row;
	if ( fssg.nrKnots(mousepid.sectionID(), rmnr) == 1 )
	    fssg.removeStick( mousepid.sectionID(), rmnr, true );
	else
	    fssg.removeKnot( mousepid.sectionID(), mousepid.subID(), true );

	mSetUserInteractionEnd();
	updateEditPids();
	return;
    }

    if ( !mousepid.isUdf() || OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	return;
    
    if ( viseditor_->sower().activate(emfss_->preferredColor(), eventinfo) )
	return;

    if ( eventinfo.pressed )
	return;

    if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) || insertpid.isUdf() )
    {
	// Add stick
	const Coord3 editnormal(
		    plane ? plane->getNormal(Coord3()) :
		    hordisp ? Coord3(0,0,1) :
		    Coord3(s2dd->getNormal(s2dd->getNearestTraceNr(pos)),0) );

	const int sid = emfss_->sectionID(0);
	Geometry::FaultStickSet* fss = fssg.sectionGeometry( sid );

	const int insertsticknr = 
			!fss || fss->isEmpty() ? 0 : fss->rowRange().stop+1;

	editpids_.erase();
	fssg.insertStick( sid, insertsticknr, 0, pos, editnormal,
			  pickedmid, pickednm, true );
	const EM::SubID subid = RowCol(insertsticknr,0).toInt64();
	fsseditor_->setLastClicked( EM::PosID(emfss_->id(),sid,subid) );
	setActiveStick( EM::PosID(emfss_->id(),sid,subid) );
	mSetUserInteractionEnd();
	updateEditPids();
    }
    else
    {
	// Add knot
	editpids_.erase();
	fssg.insertKnot( insertpid.sectionID(), insertpid.subID(), pos, true );
	fsseditor_->setLastClicked( insertpid );
	mSetUserInteractionEnd();
	updateEditPids();
    }

    eventcatcher_->setHandled();
}


static bool isSameMarkerPos( const Coord3& pos1, const Coord3& pos2 )
{
    const Coord3 diff = pos2 - pos1;
    float xymargin = 0.01 * SI().inlDistance();
    if ( diff.x*diff.x + diff.y*diff.y > xymargin*xymargin )
	return false;

    return fabs(diff.z) < 0.01 * SI().zStep();
}


#define mMatchMarker( sid, sticknr, pos1, pos2 ) \
    if ( isSameMarkerPos(pos1,pos2) ) \
    { \
	Geometry::FaultStickSet* fss = \
				emfss_->geometry().sectionGeometry( sid ); \
	if ( fss ) \
	{ \
	    fss->selectStick( sticknr, !ctrldown_ ); \
	    updateKnotMarkers(); \
	    eventcatcher_->setHandled(); \
	    return; \
	} \
    } 

void FaultStickSetDisplay::stickSelectCB( CallBacker* cb )
{
    if ( !emfss_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    bool leftmousebutton = OD::leftMouseButton( eventinfo.buttonstate_ );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.tabletinfo &&
	 eventinfo.tabletinfo->pointertype_==TabletInfo::Eraser )
    {
	leftmousebutton = true;
	ctrldown_ = true;
    }

    if ( eventinfo.type!=visBase::MouseClick || !leftmousebutton )
	return;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::Marker*, marker, dataobj );
	if ( marker )
	{
	    for ( int sipidx=0; sipidx<stickintersectpoints_.size(); sipidx++ )
	    {
		const StickIntersectPoint* sip = stickintersectpoints_[sipidx];
		mMatchMarker( sip->sid_, sip->sticknr_,
			      marker->centerPos(), sip->pos_ );
	    }

	    PtrMan<EM::EMObjectIterator> iter =
					 emfss_->geometry().createIterator(-1);
	    while ( true )
	    {
		const EM::PosID pid = iter->next();
		if ( pid.objectID() == -1 )
		    return;

		const int sticknr = RowCol( pid.subID() ).row;
		mMatchMarker( pid.sectionID(), sticknr,
			      marker->centerPos(), emfss_->getPos(pid) );
	    }
	}
    }
}


void FaultStickSetDisplay::setActiveStick( const EM::PosID& pid )
{
    const int sticknr = pid.isUdf() ? mUdf(int) : RowCol(pid.subID()).row;
    if ( activesticknr_ != sticknr )
    {
	activesticknr_ = sticknr;
	updateSticks( true );
    }
}


void FaultStickSetDisplay::emChangeCB( CallBacker* cber )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cber);
    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	getMaterial()->setColor( emfss_->preferredColor() );
	mSetStickIntersectPointColor( emfss_->preferredColor() );
    }

    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange && emfss_ )
    {
	const int sid = cbdata.pid0.sectionID();
	RowCol rc( cbdata.pid0.subID() );

	const MultiID* mid = emfss_->geometry().pickedMultiID( sid, rc.row );
	if ( mid && !emfss_->geometry().pickedOnPlane(sid, rc.row) )
	{
	    const char* nm = emfss_->geometry().pickedName( sid, rc.row );
	    const Coord3 dragpos = emfss_->getPos( cbdata.pid0 );
	    Coord3 pos = dragpos;

	    Seis2DDisplay* s2dd = Seis2DDisplay::getSeis2DDisplay( *mid, nm );
	    if ( s2dd ) 
		pos = s2dd->getNearestSubPos( pos, true );

	    HorizonDisplay* hordisp = HorizonDisplay::getHorizonDisplay( *mid );
	    if ( hordisp )
	    {
		if ( displaytransform_ )
		    pos = displaytransform_->transform( pos );

		const float dist = hordisp->calcDist( pos );
		if ( mIsUdf(dist) )
		{
		    pos = dragpos;
		    pos.z += zdragoffset;
		}
		else
		{
		    pos.z += dist;
		    pos.z -= hordisp->calcDist( pos );

		    if ( displaytransform_ )
			pos = displaytransform_->transformBack( pos );
		    if ( nm )
			pos.z += atof(nm) / scene_->zDomainInfo().userFactor();

		    zdragoffset = pos.z - dragpos.z;
		}
	    }

	    CallBack cb = mCB(this,FaultStickSetDisplay,emChangeCB);
	    emfss_->change.remove( cb );
	    emfss_->setPos( cbdata.pid0, pos, false );
	    emfss_->change.notify( cb );
	}
    }

    if ( emfss_ && !emfss_->hasBurstAlert() )
	updateAll();
}


void FaultStickSetDisplay::showManipulator( bool yn )
{
    showmanipulator_ = yn;
    if ( viseditor_ )
	viseditor_->turnOn( yn );

    updateKnotMarkers();

    if ( scene_ )
	scene_->blockMouseSelection( yn );
}


bool  FaultStickSetDisplay::isManipulatorShown() const
{ return showmanipulator_; }


void FaultStickSetDisplay::removeSelection( const Selector<Coord3>& sel,
	TaskRunner* tr )
{
    if ( fsseditor_ )
	fsseditor_->removeSelection( sel );
}


void FaultStickSetDisplay::otherObjectsMoved(
				const ObjectSet<const SurveyObject>& objs,
				int whichobj )
{
    if ( !displayonlyatsections_ )
	return;

    updateAll();
}


bool FaultStickSetDisplay::coincidesWith2DLine(
			const Geometry::FaultStickSet& fss, int sticknr,
			const MultiID& pickedmid, const char* pickednm ) const
{
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	 rowrg.snap(sticknr)!=sticknr )
	return false;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if ( !s2dd || !s2dd->isOn() || pickedmid!=s2dd->lineSetID() ||
	     !pickednm || strcmp(pickednm,s2dd->getLineName()) )
	    continue;

	const float onestepdist = Coord3(1,1,mZScale()).dot(
		inlCrlSystem()->oneStepTranslation(Coord3(0,0,1)) );

	const StepInterval<int> colrg = fss.colRange( rc.row );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    Coord3 pos = fss.getKnot(rc);
	    if ( displaytransform_ )
		pos = displaytransform_->transform( pos ); 

	    if ( s2dd->calcDist( pos ) <= 0.5*onestepdist )
		return true;
	}
    }
    return false;
}


bool FaultStickSetDisplay::coincidesWithPlane(
			const Geometry::FaultStickSet& fss, int sticknr,
			TypeSet<Coord3>& intersectpoints ) const
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	  rowrg.snap(sticknr)!=sticknr )
	return res;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	if ( !plane || !plane->isOn() )
	    continue;

	const Coord3 vec1 = fss.getEditPlaneNormal(sticknr).normalize();
	const Coord3 vec2 = plane->getNormal(Coord3()).normalize();

	const bool coincidemode = fabs(vec1.dot(vec2)) > 0.5;

	const float onestepdist = Coord3(1,1,mZScale()).dot(
	    inlCrlSystem()->oneStepTranslation(plane->getNormal(Coord3::udf())) );

	float prevdist;
	Coord3 prevpos;

	const StepInterval<int> colrg = fss.colRange( rc.row );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	{
	    Coord3 curpos = fss.getKnot(rc);
	    if ( displaytransform_ )
		curpos = displaytransform_->transform( curpos ); 

	    const float curdist = plane->calcDist( curpos );
	    if ( curdist <= 0.5*onestepdist )
	    {
		res = res || coincidemode;
		intersectpoints += curpos;
	    }
	    else if ( rc.col != colrg.start )
	    {
		const float frac = prevdist / (prevdist+curdist);
		Coord3 interpos = (1-frac)*prevpos + frac*curpos;

		if ( plane->calcDist(interpos) <= 0.5*onestepdist )
		{
		    if ( prevdist <= 0.5*onestepdist )
			intersectpoints.remove( intersectpoints.size()-1 );

		    res = res || coincidemode;
		    intersectpoints += interpos;
		}
	    }

	    prevdist = curdist;
	    prevpos = curpos;
	}
    }
    return res;
}


void FaultStickSetDisplay::displayOnlyAtSectionsUpdate()
{
    if ( !emfss_ || !fsseditor_ )
	return;

    NotifyStopper ns( fsseditor_->editpositionchange );
    deepErase( stickintersectpoints_ );

    for ( int sidx=0; sidx<emfss_->nrSections(); sidx++ )
    {
	int sid = emfss_->sectionID( sidx );
	mDynamicCastGet( Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
	{
	    TypeSet<Coord3> intersectpoints;
	    fss->hideStick( rc.row, displayonlyatsections_ );
	    if ( !displayonlyatsections_ )
		continue;

	    if ( emfss_->geometry().pickedOn2DLine(sid,rc.row) )
	    { 
		const char* lnm = emfss_->geometry().pickedName( sid, rc.row );
		const MultiID* lset =
			    emfss_->geometry().pickedMultiID( sid, rc.row );
		if ( lset && coincidesWith2DLine(*fss, rc.row, *lset, lnm) )
		{
		    fss->hideStick( rc.row, false );
		    continue;
		}
	    }

	    if ( coincidesWithPlane(*fss, rc.row, intersectpoints) )
	    {
		if ( emfss_->geometry().pickedOnPlane(sid,rc.row) )
		{
		    fss->hideStick( rc.row, false );
		    continue;
		}
	    }

	    for (  int idx=0; idx<intersectpoints.size(); idx++ )
	    {
		StickIntersectPoint* sip = new StickIntersectPoint();
		sip->sid_ = sid;
		sip->sticknr_ = rc.row;
		sip->pos_ = intersectpoints[idx];
		if ( displaytransform_ )
		    sip->pos_ = displaytransform_->transformBack( sip->pos_ ); 

		stickintersectpoints_ += sip;
	    }
	}
    }
}


void FaultStickSetDisplay::setDisplayOnlyAtSections( bool yn )
{
    displayonlyatsections_ = yn;
    updateAll();
    displaymodechange.trigger();
}


bool FaultStickSetDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


void FaultStickSetDisplay::setStickSelectMode( bool yn )
{
    stickselectmode_ = yn;
    ctrldown_ = false;

    setActiveStick( EM::PosID::udf() );
    updateEditPids();
    updateKnotMarkers();

    if ( scene_ && scene_->getPolySelection() &&
	 scene_->getPolySelection()->polygonFinished() )
    {
	const CallBack cb = mCB(this,FaultStickSetDisplay,polygonFinishedCB);
	if ( yn )
	{
	    scene_->getPolySelection()->polygonFinished()->
						    notifyIfNotNotified(cb);
	}
	else
	    scene_->getPolySelection()->polygonFinished()->remove( cb );
    }
}


bool FaultStickSetDisplay::isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const 
{
    visBase::PolygonSelection* polysel = scene_->getPolySelection();
    if ( !polysel->isInside(markerworldpos) )
	return false;

    TypeSet<int> pickedobjids;
    for ( int depthidx=0; true; depthidx++ )
    {
	if ( !polysel->rayPickThrough(markerworldpos, pickedobjids, depthidx) )
	    break;

	for ( int idx=0; true; idx++ )
	{
	    if ( idx == pickedobjids.size() )
		return false;

	    const int visid = pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    mDynamicCastGet( visBase::Marker*, marker, dataobj );
	    if ( marker )
	    {
		if ( isSameMarkerPos(marker->centerPos(),markerworldpos) )
		    return true;

		break;
	    }
	}
    }
    return false;
}


void FaultStickSetDisplay::polygonFinishedCB( CallBacker* cb )
{
    if ( !stickselectmode_ || !emfss_ || !scene_ || !isOn() || !isSelected() )
	return;

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss =
			    emfss_->geometry().sectionGeometry( sip->sid_ );

	if ( !fss || fss->isStickSelected(sip->sticknr_)!=ctrldown_ )
	    continue;

	if ( !isSelectableMarkerInPolySel(sip->pos_) )
	    continue;

	fss->selectStick( sip->sticknr_, !ctrldown_ ); 
    }

    PtrMan<EM::EMObjectIterator> iter = emfss_->geometry().createIterator(-1);
    while ( true )
    {
	EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sticknr = RowCol( pid.subID() ).row;
	const EM::SectionID sid = pid.sectionID();
	Geometry::FaultStickSet* fss = emfss_->geometry().sectionGeometry(sid);

	if ( fss->isStickSelected(sticknr) != ctrldown_ )
	    continue;

	if ( !isSelectableMarkerInPolySel(emfss_->getPos(pid)) )
	    continue;

	fss->selectStick( sticknr, !ctrldown_ ); 
    }

    updateKnotMarkers();
    scene_->getPolySelection()->clear();
}


bool FaultStickSetDisplay::isInStickSelectMode() const
{ return stickselectmode_; }


#define mAddKnotMarker( groupidx, style, pos ) \
{ \
    visBase::Marker* marker = visBase::Marker::create(); \
    marker->setMarkerStyle( style ); \
    marker->setMaterial(0); \
    marker->setDisplayTransformation( displaytransform_ ); \
    marker->setCenterPos( pos ); \
    marker->setScreenSize(3); \
    knotmarkers_[groupidx]->addObject( marker ); \
}

void FaultStickSetDisplay::updateKnotMarkers()
{
    if ( !emfss_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;
    
    for ( int idx=0; idx<knotmarkers_.size(); idx++ )
    {
	while ( knotmarkers_[idx]->size() > 1 )
	    knotmarkers_[idx]->removeObject( 1 );
    }

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss =
			    emfss_->geometry().sectionGeometry( sip->sid_ );
	if ( !fss ) continue;

	int groupidx = 0;
	if ( !showmanipulator_ || !stickselectmode_ )
	    groupidx = 2;
	else if ( fss->isStickSelected(sip->sticknr_) )
	    groupidx = 1;

	mAddKnotMarker( groupidx, MarkerStyle3D::Sphere, sip->pos_ );
    }

    if ( !showmanipulator_ || !stickselectmode_ )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfss_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const int sid = pid.sectionID();
	const int sticknr = RowCol( pid.subID() ).row;
	Geometry::FaultStickSet* fss = emfss_->geometry().sectionGeometry(sid);
	if ( !fss || fss->isStickHidden(sticknr) )
	    continue;

	const int groupidx = fss->isStickSelected(sticknr) ? 1 : 0;
	const MarkerStyle3D& style = emfss_->getPosAttrMarkerStyle(0);
	mAddKnotMarker( groupidx, style, emfss_->getPos(pid) );
    }
}


void FaultStickSetDisplay::updateAll()
{
    displayOnlyAtSectionsUpdate();
    updateSticks();
    updateEditPids();
    updateKnotMarkers();
}


void FaultStickSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
					    Coord3& pos, BufferString& val,
					    BufferString& info ) const
{
    info = ""; val = "";
    if ( !emfss_ ) return;

    info = "FaultStickSet: "; info.add( emfss_->name() );
}


void FaultStickSetDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID(), getMultiID() );
}


int FaultStickSetDisplay::usePar( const IOPar& par )
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

} // namespace visSurvey
