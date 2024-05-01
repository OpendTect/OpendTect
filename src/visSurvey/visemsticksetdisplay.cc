/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visemsticksetdisplay.h"

#include "emfaultstickset.h"
#include "keyenum.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "uistrings.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vispolygonselection.h"


namespace visSurvey
{

#define mDefaultMarkerSize 3
#define mSceneIdx (ownerscene ? ownerscene->fixedIdx() : -1)

StickSetDisplay::StickSetDisplay( bool faultstickset )
    : faultstickset_( faultstickset )
{}


StickSetDisplay::~StickSetDisplay()
{
    deepErase( stickintersectpoints_ );
}


bool StickSetDisplay::isAlreadyTransformed() const
{
    if ( !fault_ )
	return false;

    const ZDomain::Info& zinfo = fault_->zDomain();
    return zaxistransform_ &&
	( zaxistransform_->toZDomainInfo().def_ == zinfo.def_ );
}


void StickSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    for ( auto* knotmarkerset : knotmarkersets_ )
	knotmarkerset->setDisplayTransformation( nt );

    displaytransform_ = nt;
}


const mVisTrans* StickSetDisplay::getDisplayTransformation() const
{
    return displaytransform_.ptr();
}


Geometry::FaultStickSet* StickSetDisplay::faultStickSetGeometry()
{
    if ( !fault_ )
	return nullptr;

    EM::FaultGeometry* fgeometry = &fault_->geometry();
    mDynamicCastGet(Geometry::FaultStickSet*,fss,fgeometry->geometryElement())
    return fss;
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


void StickSetDisplay::polygonSelectionCB()
{
    RefMan<Scene> ownerscene = getCurScene();
    if ( !ownerscene || ! ownerscene->getPolySelection() )
	return;

    RefMan<visBase::PolygonSelection> selection =
				ownerscene->getPolySelection();
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( !selection->hasPolygon() && !selection->singleSelection()  )
	return;

    TypeSet<int> donenr;
    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss = faultStickSetGeometry();
	if ( !fss )
	    continue;

	if ( !donenr.isPresent(sip->sticknr_) )
	    donenr += sip->sticknr_;
	mSetKnotSelectStatus( fss, sip->sticknr_, sip->pos_ );
    }

    PtrMan<EM::EMObjectIterator> iter = fault_->createIterator();
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( !pid.isValid() )
	    break;

	Geometry::FaultStickSet* fss = faultStickSetGeometry();
	if ( !fss )
	    return;

	const int sticknr = pid.getRowCol().row();
	const Coord3 pos = fault_->getPos( pid );
	if ( donenr.isPresent(sticknr) )
	    continue;
	else if ( selection->isInside(pos) )
	    donenr += sticknr;

	mSetKnotSelectStatus( fss, sticknr, pos );
    }

    updateStickMarkerSet();
    selection->clear();
}


#define mForceDrawMarkerSet()\
for ( auto* knotmarkerset : knotmarkersets_ ) \
{\
    knotmarkerset->turnAllMarkersOn( true );\
    knotmarkerset->forceRedraw( true );\
}\


void StickSetDisplay::updateStickMarkerSet()
{
    bool displayknots = false;

    for ( int idx=0; idx<knotmarkersets_.size(); idx++ )
    {
	RefMan<visBase::MarkerSet> markerset = knotmarkersets_[idx];
	markerset->clearMarkers();
	markerset->setMarkerStyle( MarkerStyle3D::Sphere );
	markerset->setMaterial( nullptr );
	markerset->setDisplayTransformation( displaytransform_.ptr() );
	markerset->setScreenSize( mDefaultMarkerSize );
    }

    if ( !faultstickset_ && !displaysticks_ )
	return;
    else if ( faultstickset_ )
	displayknots = !hideallknots_ && showmanipulator_  && stickselectmode_;

    for ( int idx=0; idx<stickintersectpoints_.size(); idx++ )
    {
	const StickIntersectPoint* sip = stickintersectpoints_[idx];
	Geometry::FaultStickSet* fss = faultStickSetGeometry();
	if ( !fss )
	    continue;

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

    PtrMan<EM::EMObjectIterator> iter = fault_->createIterator();
    ConstRefMan<Scene> ownerscene = getCurScene();
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( !pid.isValid() )
	    break;

	const int sticknr = pid.getRowCol().row();
	Geometry::FaultStickSet* fss = faultStickSetGeometry();
	if ( !fss || fss->isStickHidden(sticknr,mSceneIdx) )
	    continue;

	const int groupidx = fss->isStickSelected(sticknr) ? 1 : 0;
	const MarkerStyle3D& style = fault_->getPosAttrMarkerStyle( 0 );
	knotmarkersets_[groupidx]->setMarkerStyle( style );
	knotmarkersets_[groupidx]->addPos( fault_->getPos(pid), false );
    }

    mForceDrawMarkerSet();
}


void StickSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
			Coord3& pos, BufferString& val, uiString& info ) const
{
    info.setEmpty();
    val.setEmpty();
    if ( !fault_ )
	return;

    info = faultstickset_ ? uiStrings::sFaultStickSet() : uiStrings::sFault();
    info.appendPhrase( toUiString(fault_->name()), uiString::MoreInfo,
							uiString::OnSameLine );
}


bool StickSetDisplay::matchMarker( int sticknr, const Coord3 mousepos,
				   const Coord3 pos, const Coord3 eps )
{
    if ( !mousepos.isSameAs(pos,eps) )
	return false;

    Geometry::FaultStickSet* fss = faultStickSetGeometry();
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
    if ( !s3dgeom )
	return;

    mCBCapsuleUnpack( const visBase::EventInfo&,eventinfo, cb );

    bool leftmousebutton = OD::leftMouseButton( eventinfo.buttonstate_ );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.tabletinfo_ &&
	 eventinfo.tabletinfo_->pointertype_==TabletInfo::Eraser )
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
	const VisID visid = eventinfo.pickedobjids[idx];
	visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	mDynamicCastGet( visBase::MarkerSet*, markerset, dataobj );

	if ( markerset )
	{
	    pickmarker_ = true;
	    const int markeridx = markerset->findClosestMarker( selectpos );
	    if ( markeridx ==  -1 )
		continue;

	    const Coord3 markerpos =
		markerset->getCoordinates()->getPos( markeridx );

	    for ( int sipidx=0; sipidx<stickintersectpoints_.size(); sipidx++ )
	    {
		const StickIntersectPoint* sip = stickintersectpoints_[sipidx];
		matchMarker( sip->sticknr_,markerpos,sip->pos_,eps );
	    }

	    PtrMan<EM::EMObjectIterator> iter = fault_->createIterator();
	    while ( true )
	    {
		const EM::PosID pid = iter->next();
		if ( !pid.isValid() )
		    return;

		const int sticknr = pid.getRowCol().row();
		matchMarker( sticknr, markerpos, fault_->getPos(pid),eps );
	    }
	}
    }
}


void StickSetDisplay::setCurScene( Scene* scene )
{
    ownerscene_ = scene;
}


ConstRefMan<Scene> StickSetDisplay::getCurScene() const
{
    return ownerscene_.get();
}


RefMan<Scene> StickSetDisplay::getCurScene()
{
    return ownerscene_.get();
}


const MarkerStyle3D* StickSetDisplay::markerStyle() const
{
    mDynamicCastGet(const EM::FaultStickSet*,emfss,fault_.ptr());
    if ( emfss )
    {
	mDynamicCastGet(const FaultStickSetDisplay*,ftssdspl,this);
	if ( ftssdspl )
	    return ftssdspl->getPreferedMarkerStyle();
    }
    else
    {
	mDynamicCastGet(const FaultDisplay*,ftdspl,this);
	if ( ftdspl )
	    return ftdspl->getPreferedMarkerStyle();
    }

    return nullptr;
}


void StickSetDisplay::setMarkerStyle( const MarkerStyle3D& mkstyle )
{
    mDynamicCastGet(EM::FaultStickSet*,emfss,fault_.ptr());
    if ( emfss )
    {
	mDynamicCastGet(FaultStickSetDisplay*,ftssdspl,this);
	if ( ftssdspl )
	    ftssdspl->setPreferedMarkerStyle( mkstyle );
    }
    else
    {
	mDynamicCastGet(FaultDisplay*,ftdspl,this);
	if ( ftdspl )
	    ftdspl->setPreferedMarkerStyle(mkstyle);
    }
}


void StickSetDisplay::setStickMarkerStyle( const MarkerStyle3D& mkstyle )
{
    for ( auto* markerset : knotmarkersets_ )
	markerset->setMarkerStyle( mkstyle );

    mForceDrawMarkerSet();
}


bool StickSetDisplay::areAllKnotsHidden() const
{
    return hideallknots_;
}


void StickSetDisplay::hideAllKnots( bool yn )
{
    hideallknots_ = yn;
}

} // namespace visSurvey
