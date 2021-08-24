/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D.Zheng
 * DATE     : July 2015
-*/


#include "visemsticksetdisplay.h"
#include "vistransform.h"
#include "vismarkerset.h"
#include "vissurvscene.h"
#include "vispolygonselection.h"
#include "visevent.h"
#include "vismpeeditor.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "emfaultstickset.h"

#include "mpeengine.h"
#include "keyenum.h"
#include "emfault.h"
#include "survinfo.h"
#include "mousecursor.h"
#include "mouseevent.h"

namespace visSurvey
{

#define mDefaultMarkerSize 3
#define mSceneIdx (ownerscene_ ? ownerscene_->fixedIdx() : -1)

StickSetDisplay::StickSetDisplay( bool faultstick )
    : fault_( 0 )
    , displaytransform_( 0 )
    , ctrldown_( false )
    , showmanipulator_( false )
    , stickselectmode_( false )
    , faultstickset_( faultstick )
    , hideallknots_( true )
    , displaysticks_( false )
    , eventcatcher_( 0 )
    , pickmarker_( false )
    , ownerscene_( 0 )
{}


StickSetDisplay::~StickSetDisplay()
{
    if ( displaytransform_ ) displaytransform_->unRef();

    if ( fault_ )
    {
	fault_->unRef();
	fault_ = 0;
    }

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->unRef();

    deepErase( stickintersectpoints_ );
}


void StickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    for ( int idx = 0; idx<knotmarkersets_.size(); idx++ )
	knotmarkersets_[idx]->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
	displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();

}


const mVisTrans* StickSetDisplay::getDisplayTransformation() const
{  return displaytransform_; }


Geometry::FaultStickSet* StickSetDisplay::faultStickSetGeometry( int id )
{
    if ( !fault_ || !&fault_->geometry() ) return 0;

    EM::FaultGeometry* fgeometry = &fault_->geometry();
    mDynamicCastGet( Geometry::FaultStickSet*,fss,
	fgeometry->sectionGeometry((EM::SectionID)id) );

    return fss ? fss : 0;
}


#define mSetKnotSelectStatus( fss, nr, pos )\
if ( !ctrldown_ )\
{\
    if ( !selection->isInside(pos) )\
	fss->selectStick( nr, false );\
    else\
	fss->selectStick( nr, true );\
}\
else \
{\
    if ( selection->isInside(pos) )\
    {\
	if ( fss->isStickSelected(nr) )\
	    fss->selectStick( nr, false ); \
	else \
	   fss->selectStick(nr,true); \
     }\
}\


#define mGetStickNrAndPos()\
EM::PosID pid = iter->next();\
if ( pid.objectID() == -1 )\
    break;\
const EM::SectionID sid = pid.sectionID();\
Geometry::FaultStickSet* fss = faultStickSetGeometry(sid);\
if ( !fss ) return;\
const int sticknr = pid.getRowCol().row();\
const Coord3 pos = fault_->getPos(pid);\


void StickSetDisplay::polygonSelectionCB()
{
    if ( !ownerscene_ || ! ownerscene_->getPolySelection() ) return;

    visBase::PolygonSelection* selection =  ownerscene_->getPolySelection();
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( !selection->hasPolygon() && !selection->singleSelection()  )
	return;

    TypeSet<int> donenr;
    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss = faultStickSetGeometry( sip->sid_ );
	if ( !fss ) continue;

	if ( !donenr.isPresent(sip->sticknr_) )
	    donenr += sip->sticknr_;
	mSetKnotSelectStatus( fss, sip->sticknr_, sip->pos_ );
    }

    PtrMan<EM::EMObjectIterator> iter = fault_->geometry().createIterator( -1 );
    while ( true )
    {
	mGetStickNrAndPos();
	if ( donenr.isPresent( sticknr ) )
	    continue;
	else if ( selection->isInside(pos) )
	    donenr += sticknr;
	mSetKnotSelectStatus( fss, sticknr, pos );
    }
    updateStickMarkerSet();
    selection->clear();
}


#define mForceDrawMarkerSet()\
for ( int idx = 0; idx<knotmarkersets_.size(); idx++ )\
{\
    knotmarkersets_[idx]->turnAllMarkersOn( true );\
    knotmarkersets_[idx]->forceRedraw( true );\
}\


void StickSetDisplay::updateStickMarkerSet()
{
    bool displayknots = false;

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = knotmarkersets_[idx];
	markerset->clearMarkers();
	markerset->setMarkerStyle( MarkerStyle3D::Sphere );
	markerset->setMaterial(0);
	markerset->setDisplayTransformation( displaytransform_ );
	markerset->setScreenSize( mDefaultMarkerSize );
    }

    if ( !faultstickset_ && !displaysticks_ )
	return;
    else if ( faultstickset_ )
	displayknots = !hideallknots_ && showmanipulator_  && stickselectmode_;

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss = faultStickSetGeometry( sip->sid_ );
	if ( !fss ) continue;

	int groupidx = 0;
	if ( faultstickset_ )
	    groupidx = displayknots ? 0 : 2;
	else
	    groupidx = !showmanipulator_ || !stickselectmode_ ? 2 : 0;

	if ( fss->isStickSelected(sip->sticknr_) )
	    groupidx = 1;

	knotmarkersets_[groupidx]->addPos( sip->pos_, false );
    }

    mForceDrawMarkerSet();

    if ( (faultstickset_&&!displayknots ) ||
	 (!showmanipulator_||!stickselectmode_) )
	return;

    PtrMan<EM::EMObjectIterator> iter = fault_->geometry().createIterator(-1);
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;

	const EM::SectionID sid = pid.sectionID();
	const int sticknr = pid.getRowCol().row();
	Geometry::FaultStickSet* fss = faultStickSetGeometry( sid );
	if ( !fss || fss->isStickHidden(sticknr,mSceneIdx) )
	    continue;
	const int groupidx = fss->isStickSelected(sticknr) ? 1 : 0;
	const MarkerStyle3D& style = fault_->getPosAttrMarkerStyle( 0 );
	knotmarkersets_[groupidx]->setMarkerStyle( style );
	knotmarkersets_[groupidx]->addPos( fault_->getPos(pid), false );
    }

    mForceDrawMarkerSet();
}


void StickSetDisplay::getMousePosInfo(const visBase::EventInfo& eventinfo,
    Coord3& pos,BufferString& val,
    BufferString& info) const
{
    info = ""; val = "";
    if ( !fault_ ) return;

    info = faultstickset_ ? "FaultStickSet" : "Fault: ";
    info.add( fault_->name() );
}


bool StickSetDisplay::matchMarker( int sid, int sticknr, const Coord3 mousepos,
    const Coord3 pos, const Coord3 eps )
{
    if ( !mousepos.isSameAs(pos,eps) ) return false;
    Geometry::FaultStickSet* fss = faultStickSetGeometry( sid );
    if ( fss )
    {
	if ( ctrldown_ )
	    fss->selectStick( sticknr, !fss->isStickSelected( sticknr ) );
	else
	    fss->selectStick( sticknr, true );
	updateStickMarkerSet();
	eventcatcher_->setHandled();
	return true;
    }
    return false;
}


void StickSetDisplay::stickSelectionCB( CallBacker* cb,
    const Survey::Geometry3D* s3dgeom )
{
    if ( !s3dgeom ) return;

    mCBCapsuleUnpack( const visBase::EventInfo&,eventinfo, cb );

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

    const double epsxy = s3dgeom->inlDistance()*0.1f;
    const double epsz = 0.01*s3dgeom->zStep();
    const Coord3 eps( epsxy,epsxy,epsz );
    pickmarker_ = false;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const Coord3 selectpos = eventinfo.worldpickedpos;
	const int visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::MarkerSet*, markerset, dataobj );

	if ( markerset )
	{
	    pickmarker_ = true;
	    const int markeridx = markerset->findClosestMarker( selectpos );
	    if( markeridx ==  -1 ) continue;

	    const Coord3 markerpos =
		markerset->getCoordinates()->getPos( markeridx );

	    for ( int sipidx=0; sipidx<stickintersectpoints_.size(); sipidx++ )
	    {
		const StickIntersectPoint* sip = stickintersectpoints_[sipidx];
		matchMarker(
		    sip->sid_,sip->sticknr_,markerpos,sip->pos_,eps );
	    }

	    PtrMan<EM::EMObjectIterator> iter =
					fault_->geometry().createIterator(-1);
	    while ( true )
	    {
		const EM::PosID pid = iter->next();
		if ( pid.objectID() == -1 )
		    return;

		const int sticknr = pid.getRowCol().row();
		matchMarker( pid.sectionID(), sticknr, markerpos,
		    fault_->getPos(pid),eps );
	    }
	}
    }
}


const MarkerStyle3D* StickSetDisplay::markerStyle() const
{
    mDynamicCastGet( EM::FaultStickSet*, emfss, fault_ );
    if ( emfss )
    {
	FaultStickSetDisplay* ftssdspl = (FaultStickSetDisplay*)( this );
	return ftssdspl->getPreferedMarkerStyle();
    }
    else
    {
	FaultDisplay* ftdspl = (FaultDisplay*)( this );
	return ftdspl->getPreferedMarkerStyle();
    }

}


void StickSetDisplay::setMarkerStyle( const MarkerStyle3D& mkstyle )
{
    mDynamicCastGet( EM::FaultStickSet*, emfss, fault_ );
    if ( emfss )
    {
	FaultStickSetDisplay* ftssdspl = (FaultStickSetDisplay*)( this );
	ftssdspl->setPreferedMarkerStyle( mkstyle );
    }
    else
    {
	FaultDisplay* ftdspl = (FaultDisplay*)( this );
	ftdspl->setPreferedMarkerStyle(mkstyle);
    }
}


void StickSetDisplay::setStickMarkerStyle( const MarkerStyle3D& mkstyle )
{
    for( int idx=0; idx<knotmarkersets_.size(); idx++ )
    {
	visBase::MarkerSet* markerset = knotmarkersets_[idx];
	markerset->setMarkerStyle(mkstyle);
    }
    mForceDrawMarkerSet();
}

} // namespace visSurvey
