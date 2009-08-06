/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaultdisplay.cc,v 1.41 2009-08-06 02:04:13 cvskris Exp $";

#include "visfaultdisplay.h"

#include "arrayndimpl.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "emeditor.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "faulteditor.h"
#include "iopar.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "undo.h"
#include "viscoord.h"
#include "visdragger.h"
#include "visevent.h"
#include "viscolortab.h"
#include "visgeomindexedshape.h"
#include "vismaterial.h"
#include "vismarker.h"
#include "vismultitexture2.h"
#include "vismpeeditor.h"
#include "vispickstyle.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visshapehints.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::FaultDisplay );

namespace visSurvey
{

FaultDisplay::FaultDisplay()
    : emfault_( 0 )
    , neareststickmarker_( visBase::IndexedPolyLine3D::create() )
    , validtexture_( false )
    , paneldisplay_( 0 )
    , stickdisplay_( 0 )
    , intersectiondisplay_( 0 )
    , viseditor_( 0 )
    , faulteditor_( 0 )
    , eventcatcher_( 0 )
    , explicitpanels_( 0 )
    , explicitsticks_( 0 )
    , explicitintersections_( 0 )
    , displaytransform_( 0 )
    , neareststick_( mUdf(int) )
    , shapehints_( visBase::ShapeHints::create() )
    , neareststickmarkerpickstyle_( visBase::PickStyle::create() )
    , showmanipulator_( false )
    , colorchange( this )
    , usestexture_( false )
    , displaysticks_( false )
{
    neareststickmarkerpickstyle_->ref();
    neareststickmarkerpickstyle_->setStyle( visBase::PickStyle::Unpickable );

    neareststickmarker_->ref();
    neareststickmarker_->setRadius( 1, true );
    if ( !neareststickmarker_->getMaterial() )
	neareststickmarker_->setMaterial( visBase::Material::create() );
    neareststickmarker_->insertNode(
	    neareststickmarkerpickstyle_->getInventorNode() );
    insertChild( childIndex(texture_->getInventorNode() ),
		 neareststickmarker_->getInventorNode() );

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

    if ( paneldisplay_ )
	paneldisplay_->unRef();

    if ( stickdisplay_ )
	stickdisplay_->unRef();

    if ( intersectiondisplay_ )
	intersectiondisplay_->unRef();

    delete explicitpanels_;
    delete explicitsticks_;
    delete explicitintersections_;

    if ( emfault_ )
    {
	emfault_->change.remove( mCB(this,FaultDisplay,emChangeCB) );
       	emfault_->unRef();
	emfault_ = 0;
    }

    if ( displaytransform_ ) displaytransform_->unRef();
    shapehints_->unRef();

    neareststickmarker_->unRef();
    neareststickmarkerpickstyle_->unRef();
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
    mDynamicCastGet(EM::Fault3D*,emfault,emobject.ptr());
    if ( !emfault )
    {
	if ( paneldisplay_ ) paneldisplay_->turnOn( false );
	if ( intersectiondisplay_ ) intersectiondisplay_->turnOn( false );
	if ( stickdisplay_ ) stickdisplay_->turnOn( false );
	return false;
    }

    emfault_ = emfault;
    emfault_->change.notify( mCB(this,FaultDisplay,emChangeCB) );
    emfault_->ref();


    if ( !emfault_->name().isEmpty() )
	setName( emfault_->name() );

    if ( !paneldisplay_ )
    {
	paneldisplay_ = visBase::GeomIndexedShape::create();
	paneldisplay_->ref();
	paneldisplay_->setDisplayTransformation( displaytransform_ );
	paneldisplay_->setMaterial( 0 );
	paneldisplay_->setSelectable( false );
	paneldisplay_->setRightHandSystem( righthandsystem_ );
	addChild( paneldisplay_->getInventorNode() );
    }

    if ( !intersectiondisplay_ )
    {
	intersectiondisplay_ = visBase::GeomIndexedShape::create();
	intersectiondisplay_->ref();
	intersectiondisplay_->setDisplayTransformation( displaytransform_ );
	intersectiondisplay_->setMaterial( 0 );
	intersectiondisplay_->setSelectable( false );
	intersectiondisplay_->setRightHandSystem( righthandsystem_ );
	insertChild( childIndex(texture_->getInventorNode() ),
		     intersectiondisplay_->getInventorNode() );
	intersectiondisplay_->turnOn( false );
    }

    if ( !stickdisplay_ )
    {
	stickdisplay_ = visBase::GeomIndexedShape::create();
	stickdisplay_->ref();
	stickdisplay_->setDisplayTransformation( displaytransform_ );
	if ( !stickdisplay_->getMaterial() )
	    stickdisplay_->setMaterial( visBase::Material::create() );
	stickdisplay_->setSelectable( false );
	stickdisplay_->setRightHandSystem( righthandsystem_ );
	insertChild( childIndex(texture_->getInventorNode() ),
		     stickdisplay_->getInventorNode() );
    }


    if ( !explicitpanels_ )
    {
	const float zscale = scene_
	    ? scene_->getZScale() *scene_->getZStretch()
	    : SI().zScale();

	mTryAlloc( explicitpanels_,Geometry::ExplFaultStickSurface(0,zscale));
	explicitpanels_->display( false, true );
	explicitpanels_->setMaximumTextureSize( texture_->getMaxTextureSize() );
	explicitpanels_->setTexturePowerOfTwo( true );
	explicitpanels_->setTextureSampling(
		BinIDValue( BinID(SI().inlRange(true).step,
				  SI().crlRange(true).step),
				  SI().zStep() ) );

	mTryAlloc( explicitsticks_,Geometry::ExplFaultStickSurface(0,zscale) );
	explicitsticks_->display( true, false );
	explicitsticks_->setMaximumTextureSize( texture_->getMaxTextureSize() );
	explicitsticks_->setTexturePowerOfTwo( true );
	explicitsticks_->setTextureSampling(
		BinIDValue( BinID(SI().inlRange(true).step,
				  SI().crlRange(true).step),
				  SI().zStep() ) );

	mTryAlloc( explicitintersections_, Geometry::ExplPlaneIntersection );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     emfault_->sectionGeometry( emfault_->sectionID(0)) );

    paneldisplay_->setSurface( explicitpanels_ );
    explicitpanels_->setSurface( fss ); 
    paneldisplay_->touch( true );

    explicitintersections_->setShape( *explicitpanels_ );
    intersectiondisplay_->setSurface( explicitintersections_ );

    stickdisplay_->setSurface( explicitsticks_ );
    explicitsticks_->setSurface( fss ); 
    stickdisplay_->touch( true );

    if ( !viseditor_ )
    {
	viseditor_ = visSurvey::MPEEditor::create();
	viseditor_->ref();
	viseditor_->setSceneEventCatcher( eventcatcher_ );
	viseditor_->setDisplayTransformation( displaytransform_ );
	insertChild( childIndex(texture_->getInventorNode() ),
		viseditor_->getInventorNode() );
    }

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, fe, editor.ptr() );
    faulteditor_ = fe;
    if ( faulteditor_ ) faulteditor_->ref();

    viseditor_->setEditor( faulteditor_ );
    
    paneldisplay_->turnOn( true );
    displaysticks_ = emfault_->isEmpty();

    nontexturecol_ = emfault_->preferredColor();
    updateSingleColor();
    updateStickDisplay();
    updateManipulator();

    return true;
}


void FaultDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    if ( faulteditor_ )
	faulteditor_->removeSelection( selector );
}


MultiID FaultDisplay::getMultiID() const
{
    return emfault_ ? emfault_->multiID() : MultiID();
}


void FaultDisplay::setColor( Color nc )
{
    if ( emfault_ ) emfault_->setPreferredColor(nc);
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void FaultDisplay::updateSingleColor()
{
    const bool usesinglecolor = !showingTexture();

    const Color prevcol = getMaterial()->getColor();
    const Color newcol = usesinglecolor ? nontexturecol_*0.8 : Color::White();
    if ( newcol==prevcol )
	return;

    getMaterial()->setColor( newcol );
    neareststickmarker_->getMaterial()->setColor( nontexturecol_ );
    if ( stickdisplay_ )
	stickdisplay_->getMaterial()->setColor( nontexturecol_ );

    texture_->turnOn( !usesinglecolor );
    colorchange.trigger();
}


void FaultDisplay::useTexture( bool yn, bool trigger )
{
    if ( yn && !validtexture_ )
    {
	for ( int idx=0; idx<nrAttribs(); idx++ )
	{
	    if ( getSelSpec(idx) &&
		 getSelSpec(idx)->id()==Attrib::SelSpec::cNoAttrib() )
	    {
		usestexture_ = yn;
		setDepthAsAttrib(idx);
		return;
	    }
	}
    }

    usestexture_ = yn;

    updateSingleColor();

    if ( trigger )
	colorchange.trigger();
}


void FaultDisplay::setDepthAsAttrib( int attrib )
{
    const Attrib::SelSpec as( "", Attrib::SelSpec::cNoAttrib(), false, "" );
    setSelSpec( attrib, as );

    texture_->getColorTab( attrib ).setAutoScale( true );
    texture_->getColorTab( attrib ).setClipRate( 0 );
    texture_->getColorTab( attrib ).setSymMidval( mUdf(float) );

    TypeSet<DataPointSet::DataRow> pts; 
    BufferStringSet nms; 
    DataPointSet positions( pts, nms, false, true ); 
    getRandomPos( positions, 0 ); 

    if ( !positions.size() ) return;

    setRandomPosData( attrib, &positions, 0 );
}


bool FaultDisplay::usesTexture() const
{ return usestexture_; }


bool FaultDisplay::showingTexture() const
{ return validtexture_ && usestexture_; }


NotifierAccess* FaultDisplay::materialChange()
{ return &getMaterial()->change; }


Color FaultDisplay::getColor() const
{ return neareststickmarker_->getMaterial()->getColor(); }


void FaultDisplay::updateStickDisplay()
{
    if ( !stickdisplay_ )
	return;

    const EM::SectionID sid = emfault_->sectionID( 0 );

    const bool dodisplay = displaysticks_ ||
	    (arePanelsDisplayed() && emfault_->geometry().nrSticks(sid)==1 );
    
    stickdisplay_->turnOn( dodisplay );
}


void FaultDisplay::display( bool sticks, bool panels )
{
    displaysticks_ = sticks;

    if ( neareststickmarker_ )
	neareststickmarker_->turnOn( sticks );

    if ( viseditor_ ) viseditor_->turnOn( sticks && showmanipulator_ );

    if ( paneldisplay_ )
	paneldisplay_->turnOn( panels );

    updateStickDisplay();
}


bool FaultDisplay::areSticksDisplayed() const { return displaysticks_; }


bool FaultDisplay::arePanelsDisplayed() const
{
    return paneldisplay_ ? paneldisplay_->isOn() : false;
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
    if ( paneldisplay_ ) paneldisplay_->setDisplayTransformation( nt );
    if ( stickdisplay_ ) stickdisplay_->setDisplayTransformation( nt );
    if ( intersectiondisplay_ )
	intersectiondisplay_->setDisplayTransformation( nt );
    if ( viseditor_ ) viseditor_->setDisplayTransformation( nt );
    neareststickmarker_->setDisplayTransformation( nt );

    if ( displaytransform_ ) displaytransform_->unRef();
    displaytransform_ = nt;
    if ( displaytransform_ ) displaytransform_->ref();
}


void FaultDisplay::setRightHandSystem(bool yn)
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    if ( paneldisplay_ ) paneldisplay_->setRightHandSystem( yn );
    if ( stickdisplay_ ) stickdisplay_->setRightHandSystem( yn );
    if ( intersectiondisplay_ ) intersectiondisplay_->setRightHandSystem( yn );
}


visBase::Transformation* FaultDisplay::getDisplayTransformation()
{ return displaytransform_; }


void FaultDisplay::mouseCB( CallBacker* cb )
{
    if ( !emfault_ || !faulteditor_ || !isOn() || eventcatcher_->isHandled() ||
	 !isSelected() || !viseditor_ || !viseditor_->isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
   
    CubeSampling mouseplanecs; 
    mouseplanecs.setEmpty();

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

    if ( locked_ )
	return;

    Coord3 pos = eventinfo.displaypickedpos;
    pos = scene_->getZScaleTransform()->transformBack( pos ); 
    if ( displaytransform_ ) pos = displaytransform_->transformBack( pos ); 

    EM::PosID nearestpid0, nearestpid1, insertpid;
    const float zscale = scene_
	? scene_->getZScale() *scene_->getZStretch()
	: SI().zScale();

    faulteditor_->getInteractionInfo( nearestpid0, nearestpid1, insertpid,
				      pos, zscale );

    const int neareststick = nearestpid0.isUdf()
	? mUdf(int)
	: RowCol(insertpid.subID()).row;

    if ( neareststick_!=neareststick )
    {
	neareststick_ = neareststick;
	updateNearestStickMarker();
    }

    if ( !pos.isDefined() || viseditor_->isDragging() )
	return;

    const EM::PosID pid = viseditor_ ?
       viseditor_->mouseClickDragger(eventinfo.pickedobjids) : EM::PosID::udf();
    if ( !pid.isUdf() )
    {
	if ( eventinfo.type==visBase::MouseClick &&
	     OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) &&
	     OD::leftMouseButton(eventinfo.buttonstate_) )
	{
	    eventcatcher_->setHandled();
	    if ( !eventinfo.pressed )
	    {
		const int removestick = RowCol(pid.subID()).row;
		const bool res =
		   emfault_->geometry().nrKnots( pid.sectionID(),removestick)==1
		    ? emfault_->geometry().removeStick( pid.sectionID(),
							removestick, true )
		    : emfault_->geometry().removeKnot( pid.sectionID(),
						       pid.subID(), true );

		if ( res )
		{
		    EM::EMM().undo().setUserInteractionEnd(
			    EM::EMM().undo().currentEventID() );
		    paneldisplay_->touch( false );
		    stickdisplay_->touch( false );
		    if ( intersectiondisplay_ )
			intersectiondisplay_->touch( false );
		}
	    }
	}

	return;
    }

    if ( eventinfo.type!=visBase::MouseClick || eventinfo.pressed ||
	 OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    if ( insertpid.isUdf() )
	return;

    if ( nearestpid0.isUdf() )
    {
	//Add Stick
	Coord3 editnormal(0,0,1);
	if ( mouseplanecs.isEmpty() )
	    return;

	const int insertstick = insertpid.isUdf()
	    ? mUdf(int)
	    : RowCol(insertpid.subID()).row;

	if (  mouseplanecs.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( mouseplanecs.defaultDir()==CubeSampling::Crl ) 
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	if ( emfault_->geometry().insertStick( insertpid.sectionID(),
	       insertstick, 0, pos, editnormal, true ) )
	{
	    EM::EMM().undo().setUserInteractionEnd(
		    EM::EMM().undo().currentEventID() );

	    paneldisplay_->touch( false );
	    stickdisplay_->touch( false );
	    intersectiondisplay_->touch( false );
	    faulteditor_->editpositionchange.trigger();
	}
    }
    else
    {
	if ( emfault_->geometry().insertKnot( insertpid.sectionID(),
		insertpid.subID(), pos, true ) )
	{
	    EM::EMM().undo().setUserInteractionEnd(
		    EM::EMM().undo().currentEventID() );
	    faulteditor_->editpositionchange.trigger();
	}
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
	validtexture_ = false;
	updateSingleColor();
	if ( cbdata.event==EM::EMObjectCallbackData::PositionChange )
	{
	    if ( RowCol(cbdata.pid0.subID()).row==neareststick_ )
		updateNearestStickMarker();
	}
	else
	    updateNearestStickMarker();

	paneldisplay_->touch( false );
	stickdisplay_->touch( false );
	intersectiondisplay_->touch( false );
    }
    else if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = emfault_->preferredColor();
	updateSingleColor();
    }
}


void FaultDisplay::updateNearestStickMarker()
{
    if ( mIsUdf(neareststick_) || !showmanipulator_ || !displaysticks_ )
	neareststickmarker_->turnOn( false );
    else
    {
	mDynamicCastGet( Geometry::FaultStickSurface*, fss,
			 emfault_->sectionGeometry( emfault_->sectionID(0)) );

	const StepInterval<int> rowrg = fss->rowRange();
	if ( rowrg.isUdf() || !rowrg.includes(neareststick_) )
	{
	    neareststickmarker_->turnOn( false );
	    return;
	}

	const StepInterval<int> colrg = fss->colRange( neareststick_ );
	if ( colrg.isUdf() || colrg.start==colrg.stop )
	{
	    neareststickmarker_->turnOn( false );
	    return;
	}

	neareststickmarker_->removeCoordIndexAfter(-1);
	neareststickmarker_->getCoordinates()->removeAfter(-1);

	int idx = 0;
	RowCol rc( neareststick_, 0 );
	for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col += colrg.step )
	{
	    const Coord3 pos = fss->getKnot( rc );
	    const int ci = neareststickmarker_->getCoordinates()->addPos( pos );
	    neareststickmarker_->setCoordIndex( idx++, ci );
	}

	neareststickmarker_->turnOn( true );
    }
}


void FaultDisplay::showManipulator( bool yn )
{
    showmanipulator_ = yn;
    updateStickDisplay();
    updateManipulator();
}


void FaultDisplay::updateManipulator()
{
    const bool show = showmanipulator_ && areSticksDisplayed();
    if ( viseditor_ ) viseditor_->turnOn( show );
    neareststickmarker_->turnOn( show );
    if ( scene_ ) scene_->blockMouseSelection( show );
}


bool  FaultDisplay::isManipulatorShown() const
{ return showmanipulator_; }


int FaultDisplay::nrResolutions() const
{ return 1; }


void FaultDisplay::getRandomPos( DataPointSet& dpset, TaskRunner* tr ) const
{
    if ( explicitpanels_ )
    {
	explicitpanels_->getTexturePositions( dpset, tr );
	paneldisplay_->touch( true );
    }
}


void FaultDisplay::setRandomPosData( int attrib, const DataPointSet* dpset,
				     TaskRunner* )
{
    const DataColDef texturej(Geometry::ExplFaultStickSurface::sKeyTextureJ());
    const int columnj =
	dpset->dataSet().findColDef(texturej,PosVecDataSet::NameExact);

    setRandomPosDataInternal( attrib, dpset, columnj+1, 0 );
}


void FaultDisplay::setRandomPosDataInternal( int attrib,
    const DataPointSet* dpset, int column, TaskRunner* )
{
    if ( attrib>=nrAttribs() || !dpset || dpset->nrCols()<3 ||
	 !explicitpanels_ )
    {
	validtexture_ = false;
	updateSingleColor();
	return;
    }

    const BinIDValueSet& bidvset = dpset->bivSet();
    RowCol sz = explicitpanels_->getTextureSize();
    mDeclareAndTryAlloc( PtrMan<Array2D<float> >, texturedata,
	    		 Array2DImpl<float>(sz.col,sz.row) );

    float* texturedataptr = texturedata->getData();
    for ( int idy=0; idy<texturedata->info().getTotalSz(); idy++ )
	(*texturedataptr++) = mUdf(float);

    const DataColDef texturei(Geometry::ExplFaultStickSurface::sKeyTextureI());
    const DataColDef texturej(Geometry::ExplFaultStickSurface::sKeyTextureJ());
    const int columni =
	dpset->dataSet().findColDef(texturei,PosVecDataSet::NameExact);
    const int columnj =
	dpset->dataSet().findColDef(texturej,PosVecDataSet::NameExact);

    const BinIDValueSet& vals = dpset->bivSet();
    BinIDValueSet::Pos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const float i = ptr[columni];
	const float j = ptr[columnj];
	texturedata->set( mNINT(j), mNINT(i), ptr[column] );
    }

    texture_->setData( attrib, 0, texturedata, true );
    validtexture_ = true;
    usestexture_ = true;
    updateSingleColor();
}


void FaultDisplay::setResolution( int res, TaskRunner* tr )
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


void FaultDisplay::displayIntersections( bool yn )
{
    if ( intersectiondisplay_ )
    {
	if ( yn ) intersectiondisplay_->touch( false );
	intersectiondisplay_->turnOn( yn );
    }
}


bool FaultDisplay::areIntersectionsDisplayed() const
{ return intersectiondisplay_ ? intersectiondisplay_->isOn() : false; }


void FaultDisplay::otherObjectsMoved( const ObjectSet<const SurveyObject>& objs,
				      int whichobj )
{
    if ( !explicitintersections_ ) return;

    ObjectSet<const SurveyObject> usedobjects;
    TypeSet<int> planeids;

    for ( int idx=0; idx<objs.size(); idx++ )
    {

	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	if ( !plane || !plane->isOn() )
	    continue;

	const CubeSampling cs = plane->getCubeSampling(true,true,-1);
	const BinID b00 = cs.hrg.start, b11 = cs.hrg.stop;
	BinID b01, b10;

	if ( plane->getOrientation()==PlaneDataDisplay::Timeslice )
	{
	    b01 = BinID( cs.hrg.start.inl, cs.hrg.stop.crl );
	    b10 = BinID( cs.hrg.stop.inl, cs.hrg.start.crl );
	}
	else
	{
	    b01 = b00;
	    b10 = b11;
	}

	const Coord3 c00( SI().transform(b00),cs.zrg.start );
	const Coord3 c01( SI().transform(b01),cs.zrg.stop );
	const Coord3 c11( SI().transform(b11),cs.zrg.stop );
	const Coord3 c10( SI().transform(b10),cs.zrg.start );

	const Coord3 normal = (c01-c00).cross(c10-c00).normalize();

	TypeSet<Coord3> positions;
	positions += c00;
	positions += c01;
	positions += c11;
	positions += c10;

	const int idy = intersectionobjs_.indexOf( objs[idx] );
	if ( idy==-1 )
	{
	    usedobjects += plane;
	    planeids += explicitintersections_->addPlane(normal,positions);
	}
	else
	{
	    usedobjects += plane;
	    explicitintersections_->setPlane( planeids_[idy],
					      normal, positions );
	    planeids += planeids_[idy];

	    intersectionobjs_.remove( idy );
	    planeids_.remove( idy );
	}
    }

    for ( int idx=planeids_.size()-1; idx>=0; idx-- )
	explicitintersections_->removePlane( planeids_[idx] );

    intersectionobjs_ = usedobjects;
    planeids_ = planeids;

    if ( areIntersectionsDisplayed() ) intersectiondisplay_->touch( false );
}


}; // namespace visSurvey
