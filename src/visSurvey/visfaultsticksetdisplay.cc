/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visfaultsticksetdisplay.cc,v 1.10 2009-07-31 06:49:48 cvsjaap Exp $";

#include "visfaultsticksetdisplay.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "faultstickseteditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultStickSetDisplay );

namespace visSurvey
{

FaultStickSetDisplay::FaultStickSetDisplay()
    : VisualObjectImpl(true)
    , emfss_(0)
    , eventcatcher_(0)
    , displaytransform_(0)
    , colorchange(this)
    , viseditor_(0)
    , fsseditor_(0)
    , neareststicknr_( mUdf(int) )
    , sticks_( visBase::IndexedPolyLine3D::create() )
    , neareststick_( visBase::IndexedPolyLine3D::create() )
    , showmanipulator_( false )
    , neareststickpickstyle_( visBase::PickStyle::create() )
    , stickspickstyle_( visBase::PickStyle::create() )
    , displayonlyatsections_( false )
{
    neareststickpickstyle_->ref();
    neareststickpickstyle_->setStyle( visBase::PickStyle::Unpickable );
    stickspickstyle_->ref();
    stickspickstyle_->setStyle( visBase::PickStyle::Unpickable );
    
    sticks_->ref();
    sticks_->setMaterial( 0 );
    sticks_->setRadius( 1, true );
    addChild( sticks_->getInventorNode() );
    neareststick_->ref();
    neareststick_->setMaterial( 0 );
    neareststick_->setRadius( 1.5, true );
    addChild( neareststick_->getInventorNode() );
    getMaterial()->setAmbience( 0.2 );

    sticks_->insertNode( stickspickstyle_->getInventorNode() );
    neareststick_->insertNode( neareststickpickstyle_->getInventorNode() );
}


FaultStickSetDisplay::~FaultStickSetDisplay()
{
    setSceneEventCatcher( 0 );

    if ( viseditor_ )
	viseditor_->unRef();
    if ( emfss_ )
	MPE::engine().removeEditor( emfss_->id() );
    if ( fsseditor_ )
	fsseditor_->unRef();
    fsseditor_ = 0;

    if ( emfss_ )
    {
	emfss_->change.remove( mCB(this,FaultStickSetDisplay,emChangeCB) );
       	emfss_->unRef();
	emfss_ = 0;
    }

    if ( displaytransform_ ) displaytransform_->unRef();

    sticks_->unRef();
    neareststick_->unRef();
    stickspickstyle_->unRef();
    neareststickpickstyle_->unRef();
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


#define mErrRet(s) { errmsg = s; return false; }

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
    viseditor_->setMarkerSize( 5 );

    updateSticks();
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
}


NotifierAccess* FaultStickSetDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultStickSetDisplay::getColor() const
{ return getMaterial()->getColor(); }


void FaultStickSetDisplay::setDisplayTransformation(
						visBase::Transformation* nt )
{
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );

    sticks_->setDisplayTransformation( nt );
    neareststick_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


visBase::Transformation* FaultStickSetDisplay::getDisplayTransformation()
{ return displaytransform_; }


void FaultStickSetDisplay::updateEditPids()
{
    if ( !emfss_ ) return;

    editpids_.erase();

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
	    if ( emfss_->geometry().pickedOn2DLine(sid,rc.row) )
	    { 
		const MultiID* lset = emfss_->geometry().lineSet( sid, rc.row );
		const char* lnm = emfss_->geometry().lineName( sid, rc.row );
		Seis2DDisplay* s2dd =
		    		Seis2DDisplay::getSeis2DDisplay( *lset, lnm );
		if ( (!s2dd || !s2dd->isOn()) && displayonlyatsections_ )
		    continue;
	    }
	    else if ( emfss_->geometry().pickedOnPlane(sid,rc.row) )
	    {
		// TODO: Disable editing of sticks picked on undisplayed planes
	    }
	    else
		continue;

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
	    {
		editpids_ += EM::PosID( emfss_->id(), sid, rc.getSerialized() );
	    }
	}
    }
    if ( fsseditor_ )
	fsseditor_->editpositionchange.trigger();
}


void FaultStickSetDisplay::updateSticks( bool nearestonly )
{
    if ( !emfss_ ) return;

    visBase::IndexedPolyLine3D* poly = nearestonly ? neareststick_ : sticks_;

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
	    if ( nearestonly && rc.row!=neareststicknr_ )
		continue;

	    Seis2DDisplay* s2dd = 0;
	    if ( emfss_->geometry().pickedOn2DLine(sid,rc.row) )
	    {
		const MultiID* lset = emfss_->geometry().lineSet( sid, rc.row );
		const char* lnm = emfss_->geometry().lineName( sid, rc.row );
		s2dd = Seis2DDisplay::getSeis2DDisplay( *lset, lnm );
		if ( (!s2dd || !s2dd->isOn()) && displayonlyatsections_ )
		    continue;
	    }

	    const StepInterval<int> colrg = fss->colRange( rc.row );
	    if ( !colrg.width() )
	    {
		rc.col = colrg.start;
		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.x += SI().inlDistance() * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    poly->setCoordIndex( cii++, ci );
		}
		poly->setCoordIndex( cii++, -1 );

		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.y += SI().inlDistance() * 0.5 * dir;
		    const int ci = poly->getCoordinates()->addPos( pos );
		    poly->setCoordIndex( cii++, ci );
		}
		poly->setCoordIndex( cii++, -1 );

		for ( int dir=-1; dir<=1; dir+=2 )
		{
		    Coord3 pos = fss->getKnot( rc );
		    pos.z += SI().zStep() * 0.5 * dir;
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
    if ( !nearestonly )
	updateSticks( true );
}


void FaultStickSetDisplay::mouseCB( CallBacker* cb )
{
    if ( !emfss_ || !fsseditor_ || !isOn() ||
	 eventcatcher_->isHandled() || !isSelected() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    const EM::PosID mousepid =
		    viseditor_->mouseClickDragger( eventinfo.pickedobjids );

    EM::FaultStickSetGeometry& fssg = emfss_->geometry();

    PlaneDataDisplay* plane = 0;
    Seis2DDisplay* s2dd = 0;
    const MultiID* lineset = 0;
    const char* linenm = 0;

    Coord3 pos;

    if ( !mousepid.isUdf() )
    {
	const int sticknr = RowCol( mousepid.subID() ).row;
	lineset = fssg.lineSet( mousepid.sectionID(), sticknr );
	linenm = fssg.lineName( mousepid.sectionID(), sticknr );
	pos = emfss_->getPos( mousepid );
    }
    else
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );

	    mDynamicCastGet(Seis2DDisplay*,disp2d,dataobj);
	    if ( disp2d )
	    {
		s2dd = disp2d;
		lineset = &s2dd->lineSetID();
		linenm = s2dd->name();
		break;
	    }
	    mDynamicCastGet(PlaneDataDisplay*,pdd,dataobj);
	    if ( pdd )
	    {
		plane = pdd;
		break;	
	    }
	}

	pos = eventinfo.displaypickedpos;
	if ( scene_ )
	    pos = scene_->getZScaleTransform()->transformBack( pos ); 
	if ( displaytransform_ )
	    pos = displaytransform_->transformBack( pos ); 
    }

    EM::PosID insertpid;
    const float zscale = scene_
	? scene_->getZScale() *scene_->getZStretch()
	: SI().zScale();

    fsseditor_->getInteractionInfo( insertpid, lineset, linenm, pos, zscale );

    const int neareststicknr = insertpid.isUdf() ?
				    mUdf(int) : RowCol(insertpid.subID()).row;
    if ( neareststicknr_ != neareststicknr )
    {
	neareststicknr_ = neareststicknr;
	updateSticks( true );
    }

    if ( locked_ || !pos.isDefined() || 
	 eventinfo.type!=visBase::MouseClick || viseditor_->isDragging() ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) ||
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
	const bool res = fssg.nrKnots(mousepid.sectionID(), rmnr) == 1 
	    ? fssg.removeStick( mousepid.sectionID(), rmnr, true )
	    : fssg.removeKnot( mousepid.sectionID(), mousepid.subID(), true );

	updateEditPids();
	return;
    }

    if ( !mousepid.isUdf() || eventinfo.pressed ||
	 OD::ctrlKeyboardButton(eventinfo.buttonstate_)  )
	return;


    if ( OD::shiftKeyboardButton(eventinfo.buttonstate_) || insertpid.isUdf() )
    {
	// Add stick
	if ( plane || s2dd )
	{
	    Coord3 editnormal( 0, 0, 1 );
	    if ( s2dd )
	    {
		editnormal = 
		    Coord3( s2dd->getNormal(s2dd->getNearestTraceNr(pos)), 0 );
	    }
	    else if ( plane->getCubeSampling().defaultDir()==CubeSampling::Inl )
		editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	    else if ( plane->getCubeSampling().defaultDir()==CubeSampling::Crl )
		editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	    const int sid = emfss_->sectionID(0);
	    Geometry::FaultStickSet* fss = fssg.sectionGeometry( sid );

	    const int insertsticknr = 
			!fss || fss->isEmpty() ? 0 : fss->rowRange().stop+1;

	    editpids_.erase();
	    fssg.insertStick( sid, insertsticknr, 0, pos, editnormal,
			      lineset, linenm, true );
	    updateEditPids();
	}
    }
    else
    {
	// Add knot
	editpids_.erase();
	fssg.insertKnot( insertpid.sectionID(), insertpid.subID(), pos, true );
	updateEditPids();
    }

    eventcatcher_->setHandled();
}


void FaultStickSetDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
	getMaterial()->setColor( emfss_->preferredColor() );

    if ( cbdata.event==EM::EMObjectCallbackData::PositionChange && emfss_ )
    {
	const int sid = cbdata.pid0.sectionID();
	RowCol rc( cbdata.pid0.subID() );

	if ( emfss_->geometry().pickedOn2DLine(sid, rc.r()) )
	{
	    const MultiID* lset = emfss_->geometry().lineSet( sid, rc.r() );
	    const char* lnm = emfss_->geometry().lineName( sid, rc.r() );

	    Seis2DDisplay* s2dd = Seis2DDisplay::getSeis2DDisplay( *lset, lnm );
	    if ( !s2dd ) 
		return;

	    const Coord3 curpos = emfss_->getPos( cbdata.pid0 ); 
	    const Coord3 newpos = s2dd->getNearestSubPos( curpos, true );

	    emfss_->change.remove( mCB(this,FaultStickSetDisplay,emChangeCB) );
	    emfss_->setPos( cbdata.pid0, newpos, false );
	    emfss_->change.notify( mCB(this,FaultStickSetDisplay,emChangeCB) );
	}
    }
    updateSticks();
}


void FaultStickSetDisplay::showManipulator( bool yn )
{
    showmanipulator_ = yn;
    viseditor_->turnOn( yn );
    if ( scene_ )
	scene_->blockMouseSelection( yn );
}


bool  FaultStickSetDisplay::isManipulatorShown() const
{ return showmanipulator_; }


void FaultStickSetDisplay::removeSelection( const Selector<Coord3>& sel )
{
    if ( fsseditor_ )
	fsseditor_->removeSelection( sel );
}


void FaultStickSetDisplay::otherObjectsMoved(
				const ObjectSet<const SurveyObject>& objs,
				int whichobj )
{
    updateSticks();
    updateEditPids();
}


void FaultStickSetDisplay::setDisplayOnlyAtSections( bool yn )
{
    displayonlyatsections_ = yn;
    updateSticks();
    updateEditPids();
}


bool FaultStickSetDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


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
