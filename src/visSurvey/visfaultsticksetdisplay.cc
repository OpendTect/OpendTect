/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visfaultsticksetdisplay.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "faultstickseteditor.h"
#include "iopar.h"
#include "keystrs.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vislines.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vismpeeditor.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "vispolyline.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "viscoord.h"
#include "vissurvobj.h"
#include "zdomain.h"
#include "visdrawstyle.h"
#include "hiddenparam.h"
#include "limits.h"

namespace visSurvey
{
HiddenParam< FaultStickSetDisplay,EM::SectionID > secionid_( 0 );
HiddenParam< FaultStickSetDisplay,
   ObjectSet<FaultStickSetDisplay::SectionKnotsStatus>* >sectionknotsstatus_(0);

HiddenParam<FaultStickSetDisplay,char> hideallknots_( true );


const char* FaultStickSetDisplay::sKeyEarthModelID()	{ return "EM ID"; }
const char* FaultStickSetDisplay::sKeyDisplayOnlyAtSections()
					{ return "Display only at sections"; }

#define mDefaultMarkerSize 3


#define mGetValidStickIdx( stickidx, sticknr, extra, errorres ) \
\
 int stickidx = sticknr - firstrow_; \
 if ( stickidx<-extra || stickidx>=knotsstatus_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, stickidx, extra, errorres ) \
\
 if ( !firstcols_.size() ) return errorres; \
 int knotidx = knotnr - firstcols_[stickidx]; \
 if ( knotidx<-extra || knotidx>=knotsstatus_[stickidx].size()+extra ) \
	return errorres;


void FaultStickSetDisplay::SectionKnotsStatus::addUdfRow( int firstrow, 
    int firstcol, int nrknots )
{
    firstrow_ = firstrow;
    firstcols_ += firstcol;
    knotsstatus_ +=
	TypeSet<unsigned int>( nrknots, Geometry::FaultStickSet::NoStatus );
}


bool FaultStickSetDisplay::SectionKnotsStatus::insertStickKnotStatus(
     int sticknr, int firstcol )
{
    if( knotsstatus_.size()==0 )
	firstrow_ = sticknr;

    mGetValidStickIdx(stickidx,sticknr,1,false);

    if( stickidx==-1 )
    {
	firstrow_--;
	stickidx++;
    }

    if( stickidx==knotsstatus_.size() )
    {
	firstcols_ += firstcol;
	knotsstatus_ += TypeSet<unsigned int>();
    }
    else
    {
	knotsstatus_.insert( stickidx,TypeSet<unsigned int>() );
	firstcols_.insert( stickidx,firstcol );
    }

    knotsstatus_[stickidx].insert( 0, Geometry::FaultStickSet::NoStatus );

    return true;
}


bool FaultStickSetDisplay::SectionKnotsStatus::removeStickKnotStatus(
    int sticknr )
{
    mGetValidStickIdx(stickidx,sticknr,0,false);
    firstcols_.removeSingle( stickidx );
    knotsstatus_.removeSingle( stickidx );

    if( !stickidx )
	firstrow_++;

    return true;
}


bool FaultStickSetDisplay::SectionKnotsStatus::insertKnotStatus(
    const RowCol& rc )
{
    mGetValidStickIdx(stickidx,rc.row(),0,false);
    mGetValidKnotIdx(knotidx,rc.col(),stickidx,1,false);

    if( knotidx==-1 )
    {
	firstcols_[stickidx]--;
	knotidx++;
    }

    if( knotidx==knotsstatus_[stickidx].size() )
	knotsstatus_[stickidx] += Geometry::FaultStickSet::NoStatus;
    else
	knotsstatus_[stickidx].insert(
	knotidx,Geometry::FaultStickSet::NoStatus );

    return true;

}


bool FaultStickSetDisplay::SectionKnotsStatus::removeKnotStatus(
    const RowCol& rc )
{
    mGetValidStickIdx(stickidx,rc.row(),0,false);
    mGetValidKnotIdx(knotidx,rc.col(),stickidx,0,false);

    if( knotsstatus_[stickidx].size() <= 1 )
	return removeStickKnotStatus( rc.row() );

    knotsstatus_[stickidx].removeSingle( knotidx );

    if( !knotidx )
	firstcols_[stickidx]++;

    return true;
}


bool FaultStickSetDisplay::SectionKnotsStatus::hideKnot(
     const RowCol& rc, bool yn )
{
    mGetValidStickIdx(stickidx,rc.row(),0,false);
    mGetValidKnotIdx(knotidx,rc.col(),stickidx,0,false);

    if(yn)
	knotsstatus_[stickidx][knotidx] |= Geometry::FaultStickSet::Hidden;
    else
	knotsstatus_[stickidx][knotidx] &= ~Geometry::FaultStickSet::Hidden;

    return true;
}


bool FaultStickSetDisplay::SectionKnotsStatus::isKnotHidden(
    const RowCol& rc ) const
{
    mGetValidStickIdx(stickidx,rc.row(),0,false);
    mGetValidKnotIdx(knotidx,rc.col(),stickidx,0,false);

    return knotsstatus_[stickidx][knotidx] & Geometry::FaultStickSet::Hidden;
}


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
    , sticks_(visBase::Lines::create())
    , activestick_(visBase::Lines::create())
    , showmanipulator_(false)
    , displayonlyatsections_(false)
    , stickselectmode_(false)
{
    hideallknots_.setParam( this, true );

    sticks_->ref();
    stickdrawstyle_ = sticks_->addNodeState( new visBase::DrawStyle );
    stickdrawstyle_->ref();

    LineStyle stickls( LineStyle::Solid, 3 );
    stickdrawstyle_->setLineStyle( stickls );
    addChild( sticks_->osgNode() );
    sticks_->setName( "FaultSticks" );

    activestick_->ref();
    activestickdrawstyle_ = activestick_->addNodeState(new visBase::DrawStyle);
    activestickdrawstyle_->ref();

    stickls.width_ += 2;
    activestickdrawstyle_->setLineStyle( stickls );
    addChild( activestick_->osgNode() );

    for ( int idx=0; idx<3; idx++ )
    {
	visBase::MarkerSet* markerset = visBase::MarkerSet::create();
	markerset->ref();
	addChild( markerset->osgNode() );
	markerset->setMarkersSingleColor( idx ? Color(0,255,0) :
						Color(255,0,255) );
	knotmarkersets_ += markerset;
    }

    activestick_->setPickable( false );
    activestick_->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				   false );
    sticks_->setPickable( false );
    sticks_->enableTraversal( visBase::cDraggerIntersecTraversalMask(), false );

    ObjectSet<SectionKnotsStatus>* sectknotsstatus =
	new ObjectSet<SectionKnotsStatus>;
    sectionknotsstatus_.setParam( this,sectknotsstatus );

}


FaultStickSetDisplay::~FaultStickSetDisplay()
{
    detachAllNotifiers();
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

    sticks_->removeNodeState( stickdrawstyle_ );
    stickdrawstyle_->unRef();
    sticks_->unRef();

    activestick_->removeNodeState( activestickdrawstyle_ );
    activestickdrawstyle_->unRef();
    activestick_->unRef();

    for ( int idx=knotmarkersets_.size()-1; idx>=0; idx-- )
    {
	removeChild( knotmarkersets_[idx]->osgNode() );
	knotmarkersets_.removeSingle( idx )->unRef();
    }

    deepErase( stickintersectpoints_ );
    secionid_.removeParam( this );
    ObjectSet<SectionKnotsStatus>* secknotsstatus =
	sectionknotsstatus_.getParam( this );
    
    sectionknotsstatus_.removeParam( this );
    deepErase( *secknotsstatus );
    delete secknotsstatus;
    
    hideallknots_.removeParam( this );
}


FaultStickSetDisplay::SectionKnotsStatus*
FaultStickSetDisplay::getSectionKnotsStatus( EM::SectionID sid ) const
{
    ObjectSet<SectionKnotsStatus>* secknotsstatus = 
	sectionknotsstatus_.getParam( this );
    if ( !secknotsstatus )
	return 0;

    for ( int idx = 0; idx<secknotsstatus->size(); idx++ )
    {
	if ( (*secknotsstatus)[idx]->sid_==sid )
	return (FaultStickSetDisplay::SectionKnotsStatus*)
		(*secknotsstatus)[idx];
    }

    return 0;
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
     knotmarkersets_[2]->setMarkersSingleColor( color );

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

    hideallknots_.setParam( this, !emfss_->isEmpty() );

    if ( !viseditor_ )
    {
	viseditor_ = MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	viseditor_->sower().alternateSowingOrder();
	viseditor_->sower().setIfDragInvertMask();
	addChild( viseditor_->osgNode() );
	mAttachCB( viseditor_->draggingStarted,
		   FaultStickSetDisplay::draggingStartedCB );
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
    viseditor_->setMarkerSize( mDefaultMarkerSize );

    ObjectSet<SectionKnotsStatus>* sectknotsstatus = 
	sectionknotsstatus_.getParam( this );

    for( int sidx = 0; sidx<emfss_->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emfss_->sectionID( sidx );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
	emfss_->sectionGeometry( sid ) );

	SectionKnotsStatus* sksts = new SectionKnotsStatus( sid );
	if ( fss->isEmpty() )
	{
	    sksts->addUdfRow( 0, 0, 0 );
	}
	else
	{
	    const int nrsticks = fss->nrSticks();
	    for( int idy = 0; idy<nrsticks; idy++ )
	    {
		const int firstrow = fss->rowRange().start;
		const int firstcol = fss->colRange().start;
		sksts->addUdfRow( firstrow, firstcol, fss->nrKnots(idy) );
	    }
	}
	if ( sectknotsstatus )
	    *sectknotsstatus += sksts;
    }

    updateSticks();
    updateKnotMarkers();
    updateManipulator();
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


const LineStyle* FaultStickSetDisplay::lineStyle() const
{ return &stickdrawstyle_->lineStyle(); }


void FaultStickSetDisplay::setLineStyle( const LineStyle& ls )
{
    stickdrawstyle_->setLineStyle( ls );

    LineStyle activestickls( ls );
    activestickls.width_ += 2;
    activestickdrawstyle_->setLineStyle( activestickls );

    requestSingleRedraw();
}


void FaultStickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );

    sticks_->setDisplayTransformation( nt );
    activestick_->setDisplayTransformation( nt );

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->setDisplayTransformation( nt );

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

    const bool displayknots = !areAllKnotsHidden() && !stickselectmode_;
    for ( int sidx=0; displayknots && sidx<emfss_->nrSections(); sidx++ )
    {
	EM::SectionID sid = emfss_->sectionID( sidx );
	const SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() || !sksts )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
	{
	    if ( fss->isStickHidden(rc.row()) )
		continue;

	    const StepInterval<int> colrg = fss->colRange( rc.row() );
	    for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
		  rc.col()+=colrg.step )
	    {
		if( sksts->isKnotHidden(rc) )
		    continue;
		editpids_ += EM::PosID( emfss_->id(), sid, rc.toInt64() );
	    }
	}
    }
    if ( fsseditor_ )
	fsseditor_->editpositionchange.trigger();
}


static void addPolyLineCoordIdx( TypeSet<int>& coordidxlist, int idx )
{
    if ( coordidxlist.size()%2 )
    {
	if ( idx >= 0 )
	    { coordidxlist += idx; coordidxlist += idx; }
	else	// Negative index represents line break or end
	    coordidxlist.removeSingle( coordidxlist.size()-1 );
    }
    else if ( idx >= 0 )
	coordidxlist += idx;
}

static void addPolyLineCoordBreak( TypeSet<int>& coordidxlist )
{ addPolyLineCoordIdx( coordidxlist, -1 ); }


void FaultStickSetDisplay::updateSticks( bool activeonly )
{
    if ( !emfss_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;
    visBase::Lines* poly = activeonly ? activestick_ : sticks_;

    int maxpos = 0;
    for ( int sidx=0; !activeonly && sidx<emfss_->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emfss_->sectionID( sidx );
	mDynamicCastGet( const Geometry::RowColSurface*, rcs,
			 emfss_->sectionGeometry( sid ) );
	maxpos += (rcs->rowRange().nrSteps()+1)*(rcs->colRange().nrSteps()+1);
    }

    poly->removeAllPrimitiveSets();
    Geometry::IndexedPrimitiveSet* primitiveset =
		    Geometry::IndexedPrimitiveSet::create( maxpos>USHRT_MAX );
    poly->addPrimitiveSet( primitiveset );

    if ( poly->getCoordinates()->size() )
	poly->getCoordinates()->setEmpty();

    TypeSet<int> coordidxlist;
    for ( int sidx=0; sidx<emfss_->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emfss_->sectionID( sidx );
	const SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
	mDynamicCastGet( const Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() || !sksts )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
	{
	    if ( activeonly && rc.row()!=activesticknr_ )
		continue;

	    if ( fss->isStickHidden(rc.row()) )
		continue;

	    Seis2DDisplay* s2dd = 0;
	    if ( emfss_->geometry().pickedOn2DLine(sid, rc.row()) )
	    {
		const Pos::GeomID geomid =
				emfss_->geometry().pickedGeomID(sid, rc.row());
		if ( geomid != Survey::GeometryManager::cUndefGeomID() )
		    s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	    }

	    const StepInterval<int> colrg = fss->colRange( rc.row() );

	    if ( !colrg.width() )
	    {
		rc.col() = colrg.start;
		if ( isSelected() || sksts->isKnotHidden(rc) )
		    continue;

		for ( int dim=0; dim<3; dim++ )
		{
		    const float step = dim==2 ? s3dgeom_->zStep()
					      : s3dgeom_->inlDistance();

		    for ( int dir=-1; dir<=1; dir+=2 )
		    {
			Coord3 pos = fss->getKnot( rc );
			pos[dim] += step * 0.5 * dir;
			const int ci = poly->getCoordinates()->addPos( pos );
			addPolyLineCoordIdx( coordidxlist, ci );
		    }
		    addPolyLineCoordBreak( coordidxlist );
		}
		continue;
	    }

	    for ( rc.col()=colrg.start; rc.col()<colrg.stop;
		  rc.col()+=colrg.step )
	    {
		if ( sksts->isKnotHidden(rc) )
		    continue;

		RowCol nextrc( rc );
		nextrc.col() += colrg.step;

		if ( sksts->isKnotHidden(nextrc) )
		{
		    addPolyLineCoordBreak( coordidxlist );
		    continue;
		}

		TypeSet<Coord3> coords;
		coords += fss->getKnot( rc );
		coords += fss->getKnot( nextrc );

		if ( s2dd )
		    s2dd->getLineSegmentProjection(coords[0],coords[1],coords);

		for ( int idx=0; idx<coords.size(); idx++ )
		{
		    if ( idx || coordidxlist.size()%2==0 )
		    {
			const Coord3& pos = coords[idx];
			const int ci = poly->getCoordinates()->addPos( pos );
			addPolyLineCoordIdx( coordidxlist, ci );
		    }
		}
	    }
	    addPolyLineCoordBreak( coordidxlist );
	}
    }

    if ( poly->getCoordinates()->size()>=2 && coordidxlist.size()>=2 )
    {
	primitiveset->append( coordidxlist.arr(), coordidxlist.size() );
	poly->dirtyCoordinates();
    }
    else
	poly->removeAllPrimitiveSets();

    if ( !activeonly )
	updateSticks( true );
}


Coord3 FaultStickSetDisplay::disp2world( const Coord3& displaypos ) const
{
    Coord3 pos = displaypos;
    if ( pos.isDefined() )
    {
	if ( scene_ )
	    scene_->getTempZStretchTransform()->transformBack( pos );
	if ( displaytransform_ )
	    displaytransform_->transformBack( pos );
    }
    return pos;
}


static float zdragoffset = 0;

#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getFixedZStretch()\
    : s3dgeom_->zScale() )\

#define mSetUserInteractionEnd() \
    if ( !viseditor_->sower().moreToSow() ) \
	EM::EMM().undo().setUserInteractionEnd( \
					EM::EMM().undo().currentEventID() );

void FaultStickSetDisplay::mouseCB( CallBacker* cb )
{

    if ( stickselectmode_ )
	return stickSelectCB( cb );

    if ( !emfss_ || !fsseditor_ || !viseditor_ || !viseditor_->isOn() ||
	 !isOn() || eventcatcher_->isHandled() || !isSelected() )
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
    Pos::GeomID pickedgeomid = Survey::GeometryManager::cUndefGeomID();
    const char* pickednm = 0;
    PtrMan<Coord3> normal = 0;
    ConstPtrMan<MultiID> horid;
    BufferString horshiftname;
    Coord3 pos;

    if ( !mousepid.isUdf() )
    {
	const int sticknr = mousepid.getRowCol().row();
	pos = emfss_->getPos( mousepid );
	pickedmid = fssg.pickedMultiID( mousepid.sectionID(), sticknr );
	pickednm = fssg.pickedName( mousepid.sectionID(), sticknr );
	pickedgeomid = fssg.pickedGeomID( mousepid.sectionID(), sticknr );
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
		pickedgeomid = s2dd->getGeomID();
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
    fsseditor_->getInteractionInfo( insertpid, pickedmid, pickednm,
				    pickedgeomid, pos, normal);

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
	const int rmnr = mousepid.getRowCol().row();
	SectionKnotsStatus* sksts = getSectionKnotsStatus(mousepid.sectionID());
	if ( fssg.nrKnots(mousepid.sectionID(), rmnr) == 1 )
	{
	    fssg.removeStick( mousepid.sectionID(), rmnr, true );
	    if( sksts )
		sksts->removeStickKnotStatus( rmnr );
	}
	else
	{
	    if( fssg.removeKnot(mousepid.sectionID(),mousepid.subID(),true) )
	    {
		const RowCol rc = RowCol::fromInt64( mousepid.subID() );
		if( sksts )
		    sksts->removeKnotStatus(rc);
	    }
	}

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

	const EM::SectionID sid = emfss_->sectionID(0);
	Geometry::FaultStickSet* fss = fssg.sectionGeometry( sid );

	SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
	if( !sksts )
	    return;

	const int insertsticknr =
			!fss || fss->isEmpty() ? 0 : fss->rowRange().stop+1;

	editpids_.erase();

	bool insert = false;
	if ( pickedgeomid == Survey::GeometryManager::cUndefGeomID() )
	{
	    insert = fssg.insertStick( sid, insertsticknr, 0, pos, editnormal,
		pickedmid, pickednm, true );
	}
	else
	    insert = fssg.insertStick( sid, insertsticknr, 0 ,pos, editnormal,
	    pickedgeomid, true);

	if( insert && sksts )
	    sksts->insertStickKnotStatus( insertsticknr, 0 );

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
	if( fssg.insertKnot(insertpid.sectionID(),insertpid.subID(),pos,true) )
	{
	    const RowCol rc = RowCol::fromInt64( insertpid.subID() );
	    SectionKnotsStatus* sksts =
		getSectionKnotsStatus( insertpid.sectionID() );
	    if( sksts )
		sksts->insertKnotStatus( rc );
	}
	fsseditor_->setLastClicked( insertpid );
	mSetUserInteractionEnd();
	updateEditPids();
    }

    eventcatcher_->setHandled();
}


void FaultStickSetDisplay::draggingStartedCB( CallBacker* )
{
    fsseditor_->setLastClicked( viseditor_->getActiveDragger() );
    setActiveStick( viseditor_->getActiveDragger() );
}


#define mMatchMarker( sid, sticknr, pos1, pos2,eps ) \
if ( pos1.isSameAs(pos2,eps) ) \
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

    const double epsxy = get3DSurvGeom()->inlDistance()*0.1f;
    const double epsz = 0.01f * get3DSurvGeom()->zStep();
    const Coord3 eps( epsxy,epsxy,epsz );
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const Coord3 selectpos = eventinfo.worldpickedpos;
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::MarkerSet*, markerset, dataobj );
	if ( markerset )
	{
	    const int markeridx = markerset->findClosestMarker( selectpos );
	    if( markeridx ==  -1 ) continue;

	    const Coord3 markerpos =
		markerset->getCoordinates()->getPos( markeridx );

	    for ( int sipidx=0; sipidx<stickintersectpoints_.size(); sipidx++ )
	    {
		const StickIntersectPoint* sip = stickintersectpoints_[sipidx];
		mMatchMarker( (EM::SectionID)sip->sid_, sip->sticknr_,
		    markerpos, sip->pos_,eps );
	    }

	    PtrMan<EM::EMObjectIterator> iter =
					 emfss_->geometry().createIterator(-1);
	    while ( true )
	    {
		const EM::PosID pid = iter->next();
		if ( pid.objectID() == -1 )
		    return;
		const int sticknr = pid.getRowCol().row();
		mMatchMarker( pid.sectionID(), sticknr,
			      markerpos, emfss_->getPos(pid),eps );
	    }
	}
    }
}


void FaultStickSetDisplay::setActiveStick( const EM::PosID& pid )
{
    if ( !viseditor_ )
	return;

    const bool allowactivestick = viseditor_->isOn() && !pid.isUdf();
    const int sticknr = allowactivestick ? pid.getRowCol().row() : mUdf(int);

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
	EM::SectionID sid = cbdata.pid0.sectionID();
	RowCol rc = cbdata.pid0.getRowCol();

	const MultiID* mid = emfss_->geometry().pickedMultiID( sid, rc.row() );
	const Pos::GeomID geomid = emfss_->geometry().pickedGeomID( sid,
								    rc.row() );
	if ( !emfss_->geometry().pickedOnPlane(sid, rc.row()) )
	{
	    const char* nm = emfss_->geometry().pickedName( sid, rc.row() );
	    const Coord3 dragpos = emfss_->getPos( cbdata.pid0 );
	    Coord3 pos = dragpos;

	    Seis2DDisplay* s2dd = Seis2DDisplay::getSeis2DDisplay( geomid );
	    if ( s2dd )
		pos = s2dd->getNearestSubPos( pos, true );

	    HorizonDisplay* hordisp = mid ?
			HorizonDisplay::getHorizonDisplay( *mid ) : 0;
	    if ( hordisp )
	    {
		if ( displaytransform_ )
		    displaytransform_->transform( pos );

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
			displaytransform_->transformBack( pos );
		    if ( nm )
			pos.z += toFloat(nm)/scene_->zDomainInfo().userFactor();

		    zdragoffset = (float) ( pos.z - dragpos.z );
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
    updateSticks();
    updateKnotMarkers();
    updateManipulator();
}


void FaultStickSetDisplay::updateManipulator()
{
    const bool show = showmanipulator_ && !areAllKnotsHidden();

    if ( viseditor_ )
	 viseditor_->turnOn( show && !stickselectmode_ );

    if ( scene_ )
	scene_->blockMouseSelection( show );
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


#define mHideKnotTolerance 2.0

bool FaultStickSetDisplay::coincidesWith2DLine(
			  const Geometry::FaultStickSet& fss, int sticknr,
			  Pos::GeomID geomid ) const
{
    bool res = false;
    RowCol rc( sticknr, 0 );
    const EM::SectionID sid = secionid_.getParam( this );
    SectionKnotsStatus* sksts = getSectionKnotsStatus( sid);
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	 rowrg.snap(sticknr)!=sticknr || !sksts )
	return res;

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	visBase::DataObject* dataobj = scene_->getObject( idx );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if ( !s2dd || !s2dd->isOn() || geomid!=s2dd->getGeomID() )
	    continue;
	
	const double onestepdist = Coord3(1,1,mZScale()).dot(
		s3dgeom_->oneStepTranslation(Coord3(0,0,1)) );

	bool curobjcoincides = false;
	TypeSet<int> showcols;
	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 pos = fss.getKnot(rc);
	    if ( displaytransform_ )
		displaytransform_->transform( pos );

	    const float curdist = s2dd->calcDist( pos );
	    if ( curdist <= 0.5*onestepdist )
		curobjcoincides = true;
	    if ( curdist <= (mHideKnotTolerance+0.5)*onestepdist )
		showcols += rc.col();
	}
	res = res || curobjcoincides;

	for ( int idy=showcols.size()-1; curobjcoincides && idy>=0; idy-- )
	    sksts->hideKnot( RowCol(sticknr,showcols[idy]), false );
    }

    return res;
}


bool FaultStickSetDisplay::coincidesWithPlane(
			const Geometry::FaultStickSet& fss, int sticknr,
			TypeSet<Coord3>& intersectpoints ) const
{
    bool res = false;
    const EM::SectionID sid = secionid_.getParam( this );
    SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
    RowCol rc( sticknr, 0 );
    const StepInterval<int> rowrg = fss.rowRange();
    if ( !scene_ || !rowrg.includes(sticknr,false) ||
	  rowrg.snap(sticknr)!=sticknr || !sksts )
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

	const double onestepdist = Coord3(1,1,mZScale()).dot(
	    s3dgeom_->oneStepTranslation(plane->getNormal(Coord3::udf())));

	float prevdist = -1;
	Coord3 prevpos;

	bool curobjcoincides = false;
	TypeSet<int> showcols;
	const StepInterval<int> colrg = fss.colRange( rc.row() );
	for ( rc.col()=colrg.start; rc.col()<=colrg.stop; rc.col()+=colrg.step )
	{
	    Coord3 curpos = fss.getKnot(rc);
	    if ( displaytransform_ )
		displaytransform_->transform( curpos );

	    const float curdist = plane->calcDist( curpos );
	    if ( curdist <= 0.5*onestepdist )
	    {
		intersectpoints += curpos;
		if ( coincidemode )
		    curobjcoincides = true;
	    }
	    else if ( rc.col() != colrg.start )
	    {
		const float frac = prevdist / (prevdist+curdist);
		Coord3 interpos = (1-frac)*prevpos + frac*curpos;

		if ( plane->calcDist(interpos) <= 0.5*onestepdist )
		{
		    if ( prevdist <= 0.5*onestepdist )
			intersectpoints.removeSingle(intersectpoints.size()-1);

		    intersectpoints += interpos;
		    if ( coincidemode )
			curobjcoincides = true;
		}
	    }

	    if ( coincidemode && curdist<=(mHideKnotTolerance+0.5)*onestepdist )
		showcols += rc.col();

	    prevdist = curdist;
	    prevpos = curpos;
	}
	res = res || curobjcoincides;

	for ( int idy=showcols.size()-1; curobjcoincides && idy>=0; idy-- )
	    sksts->hideKnot( RowCol(sticknr,showcols[idy]), false );
    }
    return res;
}


void FaultStickSetDisplay::displayOnlyAtSectionsUpdate()
{
    if ( !emfss_ || !fsseditor_ )
	return;

    NotifyStopper ns( fsseditor_->editpositionchange );
    deepErase( stickintersectpoints_ );

    const EM::PosID curdragger = viseditor_->getActiveDragger();

    for ( int sidx=0; sidx<emfss_->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emfss_->sectionID( sidx );
	SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
	mDynamicCastGet( Geometry::FaultStickSet*, fss,
			 emfss_->sectionGeometry( sid ) );
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();
	secionid_.setParam( this, sid );
	for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
	{
	    const StepInterval<int> colrg = fss->colRange();
	    if ( sksts )
	    {
		for( rc.col()=colrg.start; rc.col()<=colrg.stop;
		    rc.col()+=colrg.step )
		{
		    sksts->hideKnot( rc, displayonlyatsections_ );
		    if ( curdragger==EM::PosID(emfss_->id(),sid,rc.toInt64()) )
			sksts->hideKnot( rc, false );
		}
	    }

	    TypeSet<Coord3> intersectpoints;
	    fss->hideStick( rc.row(), displayonlyatsections_ );
	    if ( !displayonlyatsections_ )
		continue;

	    if ( emfss_->geometry().pickedOn2DLine(sid,rc.row()) )
	    {
		const Pos::GeomID geomid =
				emfss_->geometry().pickedGeomID(sid,rc.row());
		if ( coincidesWith2DLine(*fss,rc.row(),geomid) )
		{
		    fss->hideStick( rc.row(), false );
		    continue;
		}
	    }

	    if ( coincidesWithPlane(*fss,rc.row(),intersectpoints) )
	    {
		if ( emfss_->geometry().pickedOnPlane(sid,rc.row()) )
		{
		    fss->hideStick( rc.row(), false );
		    continue;
		}
	    }

	    for (  int idx=0; idx<intersectpoints.size(); idx++ )
	    {
		StickIntersectPoint* sip = new StickIntersectPoint();
		sip->sid_ = sid;
		sip->sticknr_ = rc.row();
		sip->pos_ = intersectpoints[idx];
		if ( displaytransform_ )
		    displaytransform_->transformBack( sip->pos_ );

		stickintersectpoints_ += sip;
	    }
	}
    }
}


void FaultStickSetDisplay::setDisplayOnlyAtSections( bool yn )
{
    if ( displayonlyatsections_ == yn ) return;
    displayonlyatsections_ = yn;
    updateAll();
    displaymodechange.trigger();
}


bool FaultStickSetDisplay::displayedOnlyAtSections() const
{ return displayonlyatsections_; }


void FaultStickSetDisplay::setStickSelectMode( bool yn )
{
    if ( yn==stickselectmode_ )
	return;

    stickselectmode_ = yn;
    ctrldown_ = false;

    setActiveStick( EM::PosID::udf() );
    updateManipulator();
    updateEditPids();
    updateKnotMarkers();

    if ( scene_ && scene_->getPolySelection() )
    {
	if ( yn )
	    mAttachCBIfNotAttached(
	    scene_->getPolySelection()->polygonFinished(),
	    FaultStickSetDisplay::polygonFinishedCB );
	else
	    mDetachCB(
	    scene_->getPolySelection()->polygonFinished(),
	    FaultStickSetDisplay::polygonFinishedCB );
    }
}


bool FaultStickSetDisplay::isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const
{
    visBase::PolygonSelection* polysel = scene_->getPolySelection();
    if ( polysel )
	return polysel->isInside( markerworldpos );
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
	      emfss_->geometry().sectionGeometry( (EM::SectionID)sip->sid_ );

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

	const int sticknr = pid.getRowCol().row();
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


void FaultStickSetDisplay::updateKnotMarkers()
{
    if ( !emfss_ || (viseditor_ && viseditor_->sower().moreToSow()) )
	return;

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = knotmarkersets_[idx];
	markerset->clearMarkers();
	markerset->setMarkerStyle( MarkerStyle3D::Sphere );
	markerset->setMaterial(0);
	markerset->setDisplayTransformation( displaytransform_ );
	markerset->setScreenSize( mDefaultMarkerSize );
    }

    const bool displayknots = !areAllKnotsHidden() && showmanipulator_
						   && stickselectmode_;
    int groupidx = displayknots ? 0 : 2;

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss =
	     emfss_->geometry().sectionGeometry( (EM::SectionID)sip->sid_ );
	if ( !fss ) continue;
	if ( fss->isStickSelected(sip->sticknr_) )
	    groupidx = 1;

	knotmarkersets_[groupidx]->addPos( sip->pos_, false );
    }

    knotmarkersets_[groupidx]->forceRedraw( true );

    if ( !displayknots )
	return;

    PtrMan<EM::EMObjectIterator> iter = emfss_->geometry().createIterator(-1);

    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const EM::SectionID sid = pid.sectionID();
	const int sticknr = pid.getRowCol().row();
	const SectionKnotsStatus* sksts = getSectionKnotsStatus( sid );
	Geometry::FaultStickSet* fss = emfss_->geometry().sectionGeometry(sid);
	if( !fss || fss->isStickHidden(sticknr) || !sksts ||
	    sksts->isKnotHidden(pid.getRowCol()) )
	    continue;

	groupidx = fss->isStickSelected(sticknr) ? 1 : 0;

	const MarkerStyle3D& style = emfss_->getPosAttrMarkerStyle(0);
	knotmarkersets_[groupidx]->setMarkerStyle( style );
	knotmarkersets_[groupidx]->setScreenSize( mDefaultMarkerSize );
	knotmarkersets_[groupidx]->addPos( emfss_->getPos(pid), false );
    }

    knotmarkersets_[groupidx]->forceRedraw( true );

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


void FaultStickSetDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    par.set( sKeyEarthModelID(), getMultiID() );
    par.setYN( sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    par.set( sKey::Color(), (int) getColor().rgb() );
}


bool FaultStickSetDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

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

    par.getYN(  sKeyDisplayOnlyAtSections(), displayonlyatsections_ );
    Color col;
    par.get( sKey::Color(), (int&) col.rgb() );
    setColor( col );

    return true;
}


void FaultStickSetDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( sticks_ )
	sticks_->setPixelDensity( dpi );

    if ( activestick_ )
	activestick_ ->setPixelDensity( dpi );
    
    for ( int idx =0; idx<knotmarkersets_.size(); idx++ )
        knotmarkersets_[idx]->setPixelDensity( dpi );

}


bool FaultStickSetDisplay::areAllKnotsHidden() const
{ return hideallknots_.getParam( this ); }


void FaultStickSetDisplay::hideAllKnots( bool yn )
{
    if ( areAllKnotsHidden() != yn )
    {
	hideallknots_.setParam( this, yn );
	updateAll();
	updateManipulator();
    }
}


} // namespace visSurvey
