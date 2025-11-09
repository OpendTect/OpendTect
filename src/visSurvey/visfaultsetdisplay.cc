/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visfaultsetdisplay.h"

#include "arrayndimpl.h"
#include "binidsurface.h"
#include "datacoldef.h"
#include "emmanager.h"
#include "executor.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "iopar.h"
#include "keystrs.h"
#include "posvecdataset.h"
#include "settings.h"
#include "uistrings.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "visseis2ddisplay.h"


namespace visSurvey
{

#define mDefaultMarkerSize 3
#define mSceneIdx (scene_ ? scene_->fixedIdx() : -1)

const char* FaultSetDisplay::sKeyEarthModelID() { return "EM ID"; }
const char* FaultSetDisplay::sKeyTriProjection() { return "TriangulateProj"; }
const char* FaultSetDisplay::sKeyDisplayPanels() { return "Display panels"; }
const char* FaultSetDisplay::sKeyDisplayIntersections()
				{ return "Display intersections"; }
const char* FaultSetDisplay::sKeyDisplayHorIntersections()
				{ return "Display horizon intersections"; }
const char* FaultSetDisplay::sKeyUseTexture()	{ return "Use texture"; }
const char* FaultSetDisplay::sKeyLineStyle()	{ return "Linestyle"; }
const char* FaultSetDisplay::sKeyZValues()		{ return "Z values"; }

FaultSetDisplay::FaultSetDisplay()
    : MultiTextureSurveyObject()
    , colorchange(this)
    , displaymodechange(this)
{
    ref();
    datapacks_.setNullAllowed();

    drawstyle_ = visBase::DrawStyle::create();
    addNodeState( drawstyle_.ptr() );
    drawstyle_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,2) );

    getMaterial()->setAmbience( 0.8 );
    mAttachCB( getMaterial()->change, FaultSetDisplay::matChangeCB );

    init();
    unRefNoDelete();
}


FaultSetDisplay::~FaultSetDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( nullptr );

    deepErase( horshapes_ );
    deepErase( explicitpanels_ );
    deepErase( explicitintersections_ );
    for ( auto* texturedataset : texturedataset_ )
	deepErase( *texturedataset );
    deepErase( texturedataset_ );
}


void FaultSetDisplay::setSceneEventCatcher( visBase::EventCatcher* vec )
{
    if ( eventcatcher_ )
	mDetachCB( eventcatcher_->eventhappened,FaultSetDisplay::mouseCB );

    eventcatcher_ = vec;

    if ( eventcatcher_ )
	mAttachCB( eventcatcher_->eventhappened,FaultSetDisplay::mouseCB );
}


void FaultSetDisplay::setScene( Scene* scene )
{
    SurveyObject::setScene( scene );
    for ( auto* explicitpanel : explicitpanels_ )
	explicitpanel->setSceneIdx( mSceneIdx );
}


EM::ObjectID FaultSetDisplay::getEMObjectID() const
{
    return faultset_ ? faultset_->id() : EM::ObjectID::udf();
}


#define mErrRet(s) { errmsg_ = s; return false; }

EM::FaultSet3D* FaultSetDisplay::emFaultSet()
{
    mDynamicCastGet(EM::FaultSet3D*,flt,faultset_.ptr());
    return flt;
}


bool FaultSetDisplay::setEMObjectID( const EM::ObjectID& emid )
{
    if ( faultset_ )
	mDetachCB( faultset_->change,FaultSetDisplay::emChangeCB );

    faultset_ = nullptr;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultSet3D*,emfaultset,emobject.ptr());
    if ( !emfaultset )
    {
	for ( int idx=0; idx<paneldisplays_.size(); idx++ )
	{
	    paneldisplays_[idx]->turnOn( false );
	    intersectiondisplays_[idx]->turnOn( false );
	}

	for ( auto* horintersections : horintersections_ )
	    horintersections->turnOn( false );

	return false;
    }

    faultset_ = emfaultset;
    if ( !emfaultset->name().isEmpty() )
	setName( emfaultset->name() );

    if ( paneldisplays_.isEmpty() )
    {
	const float zscale = scene_
	    ? scene_->getZScale() *scene_->getFixedZStretch()
	    : s3dgeom_->zScale();

	for ( int idx=0; idx<faultset_->nrFaults(); idx++ )
	{
	    RefMan<visBase::TextureChannels> channels =
					visBase::TextureChannels::create();
	    RefMan<visBase::ColTabTextureChannel2RGBA> channelsrgba =
				visBase::ColTabTextureChannel2RGBA::create();
	    channels->setChannels2RGBA( channelsrgba.ptr() );
	    channels->enableTextureInterpolation( true );
	    channelset_.add( channels.ptr() );
	    if ( idx==0 )
		channels_ = channels;

	    RefMan<visBase::GeomIndexedShape> paneldisplay =
					visBase::GeomIndexedShape::create();
	    paneldisplay->setDisplayTransformation( displaytransform_.ptr() );
	    paneldisplay->setMaterial( nullptr );
	    paneldisplay->setSelectable( false );
	    paneldisplay->setGeometryShapeType(
					visBase::GeomIndexedShape::Triangle );
	    paneldisplay->useOsgNormal( true );
	    paneldisplay->setRenderMode( visBase::RenderBothSides );
	    paneldisplay->setTextureChannels( channels.ptr() );

	    addChild( paneldisplay->osgNode() );
	    paneldisplays_ += paneldisplay.ptr();

	    Geometry::ExplFaultStickSurface* explicitpanel = nullptr;
	    mTryAlloc( explicitpanel,Geometry::ExplFaultStickSurface(0,zscale));
	    explicitpanel->setSceneIdx( mSceneIdx );
	    explicitpanel->display( false, true );
	    explicitpanel->setTexturePowerOfTwo( true );
	    explicitpanel->setTextureSampling(
			BinIDValue( BinID(s3dgeom_->inlRange().step_,
					  s3dgeom_->crlRange().step_),
				      s3dgeom_->zStep() ) );
	    explicitpanels_ += explicitpanel;

	    auto* arrs = new ObjectSet<Array2D<float> >;
	    arrs->setNullAllowed();
	    texturedataset_ += arrs;

	    RefMan<visBase::GeomIndexedShape> intersectiondisplay =
					visBase::GeomIndexedShape::create();
	    intersectiondisplay->setDisplayTransformation(
						displaytransform_.ptr() );
	    intersectiondisplay->setMaterial( nullptr );
	    intersectiondisplay->setSelectable( false );
	    intersectiondisplay->setGeometryShapeType(
					visBase::GeomIndexedShape::PolyLine3D,
					Geometry::PrimitiveSet::Lines );
	    addChild( intersectiondisplay->osgNode() );
	    intersectiondisplay->turnOn( false );
	    intersectiondisplays_ += intersectiondisplay.ptr();

	    Geometry::ExplPlaneIntersection* explicitintersection = nullptr;
	    mTryAlloc( explicitintersection, Geometry::ExplPlaneIntersection );
	    explicitintersections_ += explicitintersection;
	}
    }

    for ( int idx=0; idx<faultset_->nrFaults(); idx++ )
    {
	RefMan<EM::Fault3D> flt3d =
		faultset_->getFault3D( faultset_->getFaultID(idx) );
	mDynamicCastGet(Geometry::FaultStickSurface*,fss,
			flt3d->geometryElement())

	paneldisplays_[idx]->setSurface( explicitpanels_[idx] );
	explicitpanels_[idx]->setSurface( fss );
	paneldisplays_[idx]->touch( true );

	explicitintersections_[idx]->setShape( *explicitpanels_[idx] );
	intersectiondisplays_[idx]->setSurface( explicitintersections_[idx] );
    }

    nontexturecol_ = faultset_->preferredColor();
    updateSingleColor();
    updateDisplay();
    mAttachCB( faultset_->change,FaultSetDisplay::emChangeCB );

    return true;
}


MultiID FaultSetDisplay::getMultiID() const
{
    return faultset_ ? faultset_->multiID() : MultiID();
}


void FaultSetDisplay::setColor( OD::Color nc )
{
    if ( faultset_ )
	faultset_->setPreferredColor( nc );
    else
    {
	nontexturecol_ = nc;
	updateSingleColor();
    }
}


void FaultSetDisplay::updateSingleColor()
{
    const bool usesinglecolor = !showsTexture();

    for ( int idx=0; idx<channelset_.size(); idx++ )
	channelset_[idx]->turnOn( !usesinglecolor );

    const float transparency = getMaterial()->getTransparency();
    const OD::Color prevcol = getMaterial()->getColor();
    const OD::Color newcol = usesinglecolor ? nontexturecol_.darker(0.3)
					: OD::Color::White();
    if ( newcol != prevcol )
    {
	getMaterial()->setColor( newcol );
	getMaterial()->setTransparency( transparency );
	colorchange.trigger();
    }
    else if ( !usesinglecolor )			// To update color column in
	getMaterial()->change.trigger();	// tree if texture is shown

    for ( int idx = 0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->getMaterial()->setColor( nontexturecol_ );

    for ( int idx=0; idx<intersectiondisplays_.size(); idx++ )
	if ( intersectiondisplays_[idx]->getMaterial() )
	    intersectiondisplays_[idx]->updateMaterialFrom( getMaterial() );
}


bool FaultSetDisplay::usesColor() const
{
    return !showsTexture() ||
	   areIntersectionsDisplayed() || areHorizonIntersectionsDisplayed();
}


void FaultSetDisplay::useTexture( bool yn, bool trigger )
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


void FaultSetDisplay::setDepthAsAttrib( int attrib )
{
    const bool attribwasdepth = getSelSpec(attrib) &&
		    StringView(getSelSpec(attrib)->userRef())==sKeyZValues();

    const Attrib::SelSpec as( sKeyZValues(), Attrib::SelSpec::cNoAttrib(),
			      false, "" );
    setSelSpec( attrib, as );

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    RefMan<DataPointSet> dps = new DataPointSet( false, true );
    getRandomPos( *dps.ptr(), nullptr );
    dps->setName( sKeyZValues() );
    auto* zvalsdef = new DataColDef( sKeyZValues() );
    dps->dataSet().add( zvalsdef );
    BinIDValueSet& bivs = dps->bivSet();
    if ( dps->size() && bivs.nrVals()==5 )
    {
	int zcol = dps->dataSet().findColDef( *zvalsdef,
					       PosVecDataSet::NameExact );
	if ( zcol==-1 ) zcol = 3;

	BinIDValueSet::SPos pos;
	while ( bivs.next(pos) )
	{
	    float* vals = bivs.getVals(pos);
	    if ( zaxistransform_ )
	    {
		vals[zcol] = zaxistransform_->transform(
				    BinIDValue(bivs.getBinID(pos), vals[0]) );
	    }
	    else
		vals[zcol] = vals[0];
	}

	setRandomPosData( attrib, dps.ptr(), nullptr );
    }

    if ( !attribwasdepth )
    {
	BufferString seqnm;
	Settings::common().get( "dTect.Horizon.Color table", seqnm );
	ColTab::Sequence seq( seqnm );
	setColTabSequence( attrib, seq, nullptr );
	setColTabMapperSetup( attrib, ColTab::MapperSetup(), nullptr );
    }
}


bool FaultSetDisplay::canShowTexture() const
{
    return validtexture_ && isAnyAttribEnabled() && arePanelsDisplayedInFull();
}


OD::Color FaultSetDisplay::getColor() const
{
    return nontexturecol_;
}


void FaultSetDisplay::updatePanelDisplay()
{
    for ( int idx=0; idx<paneldisplays_.size(); idx++ )
    {
	const bool dodisplay = arePanelsDisplayed() &&
			       !areIntersectionsDisplayed() &&
			       !areHorizonIntersectionsDisplayed();
	if ( dodisplay )
	    paneldisplays_[idx]->touch( false,false );

	paneldisplays_[idx]->turnOn( dodisplay );
    }
}


void FaultSetDisplay::updateIntersectionDisplay()
{
    for ( int idx=0; idx<intersectiondisplays_.size(); idx++ )
    {
	setLineRadius( intersectiondisplays_[idx] );

	const bool dodisplay = areIntersectionsDisplayed() &&
			       arePanelsDisplayed() &&
			       isOn() && otherobjects_;
	if ( dodisplay )
	    intersectiondisplays_[idx]->touch( false );

	intersectiondisplays_[idx]->turnOn( dodisplay );
    }
}


void FaultSetDisplay::updateHorizonIntersectionDisplay()
{
    for ( int idx=0; idx<horintersections_.size(); idx++ )
    {
	setLineRadius( horintersections_[idx] );

	const bool dodisplay = areHorizonIntersectionsDisplayed() &&
			       arePanelsDisplayed() && isOn();
	if ( dodisplay )
	    horintersections_[idx]->touch( false );

	horintersections_[idx]->turnOn( dodisplay );
    }
}


void FaultSetDisplay::updateDisplay()
{
    updateIntersectionDisplay();
    updateHorizonIntersectionDisplay();
    updatePanelDisplay();
}


void FaultSetDisplay::triangulateAlg( mFltTriProj projplane )
{
    if ( explicitpanels_.isEmpty() ||
	    projplane==explicitpanels_.first()->triangulateAlg() )
	return;

    for ( int idx=0; idx<paneldisplays_.size(); idx++ )
    {
	explicitpanels_[idx]->triangulateAlg( projplane );
	paneldisplays_[idx]->touch( true );
    }

    updateIntersectionDisplay();
}


mFltTriProj FaultSetDisplay::triangulateAlg() const
{
    return !explicitpanels_.isEmpty() ?
		  explicitpanels_.first()->triangulateAlg()
		: Geometry::ExplFaultStickSurface::None;
}


void FaultSetDisplay::displayPanels( bool yn )
{
    displaypanels_ = yn;
    updateDisplay();
    displaymodechange.trigger();
}


bool FaultSetDisplay::arePanelsDisplayed() const
{ return displaypanels_; }


bool FaultSetDisplay::arePanelsDisplayedInFull() const
{
    return paneldisplays_.isEmpty() ? false : paneldisplays_.first()->isOn();
}


void FaultSetDisplay::fillPar( IOPar& par ) const
{
    MultiTextureSurveyObject::fillPar( par );

    par.set( sKeyEarthModelID(), getMultiID() );
    par.set( sKeyTriProjection(), triangulateAlg() );

    par.setYN( sKeyDisplayPanels(), displaypanels_ );
    par.setYN( sKeyDisplayIntersections(), displayintersections_ );
    par.setYN( sKeyDisplayHorIntersections(), displayhorintersections_ );
    par.setYN( sKeyUseTexture(), usestexture_ );
    par.set( sKey::Color(), (int) getColor().rgb() );

    if ( lineStyle() )
    {
	BufferString str;
	lineStyle()->toString( str );
	par.set( sKeyLineStyle(), str );
    }
}


bool FaultSetDisplay::usePar( const IOPar& par )
{
    if ( !MultiTextureSurveyObject::usePar(par) )
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

	if ( emobject ) setEMObjectID( emobject->id() );
    }

    int tp;
    par.get( sKeyTriProjection(), tp );
    triangulateAlg( (Geometry::ExplFaultStickSurface::TriProjection)tp );

    par.getYN( sKeyDisplayPanels(), displaypanels_ );
    par.getYN( sKeyDisplayIntersections(), displayintersections_ );
    par.getYN( sKeyDisplayHorIntersections(), displayhorintersections_ );
    par.getYN( sKeyUseTexture(), usestexture_ );

    par.get( sKey::Color(), (int&) nontexturecol_.rgb() );
    updateSingleColor();

    BufferString linestyle;
    if ( par.get(sKeyLineStyle(),linestyle) )
    {
	OD::LineStyle ls;
	ls.fromString( linestyle );
	setLineStyle( ls );
    }

    return true;
}


void FaultSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    for ( int idx=0; idx<paneldisplays_.size(); idx++ )
    {
	paneldisplays_[idx]->setDisplayTransformation( nt );
	intersectiondisplays_[idx]->setDisplayTransformation( nt );
    }

    for ( int idx=0; idx<horintersections_.size(); idx++ )
	horintersections_[idx]->setDisplayTransformation( nt );
}


const mVisTrans* FaultSetDisplay::getDisplayTransformation() const
{
    return paneldisplays_.isEmpty() ? nullptr
			: paneldisplays_.first()->getDisplayTransformation();
}


bool FaultSetDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->remove(
		mCB(this,FaultSetDisplay,dataTransformCB) );
	if ( voiid_>0 )
	{
	    zaxistransform_->removeVolumeOfInterest( voiid_ );
	    voiid_ = -1;
	}
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	if ( zaxistransform_->changeNotifier() )
	    zaxistransform_->changeNotifier()->notify(
		    mCB(this,FaultSetDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* FaultSetDisplay::getZAxisTransform() const
{
    return zaxistransform_.ptr();
}


void FaultSetDisplay::dataTransformCB( CallBacker* )
{
    // TODO: implement
}


Coord3 FaultSetDisplay::disp2world( const Coord3& displaypos ) const
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


#define mZScale() \
    ( scene_ ? scene_->getZScale()*scene_->getFixedZStretch() \
	     : s3dgeom_->zScale() )


void FaultSetDisplay::mouseCB( CallBacker* cb )
{
    if ( !faultset_ || !isOn() || eventcatcher_->isHandled() || !isSelected() )
	return;

    //mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    eventcatcher_->setHandled();
}


void FaultSetDisplay::emChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);

    if ( !faultset_ ) return;

    if ( cbdata.event==EM::EMObjectCallbackData::PrefColorChange )
    {
	nontexturecol_ = faultset_->preferredColor();
	updateSingleColor();
    }
}


int FaultSetDisplay::nrResolutions() const
{ return 1; }


class RandomPosExtractor : public Executor
{ mODTextTranslationClass(RandomPosExtractor)
public:
RandomPosExtractor( const ObjectSet<Geometry::ExplFaultStickSurface>& panels,
		    const ObjectSet<visBase::GeomIndexedShape>& disps,
		    DataPointSet& dpset )
    : Executor("Extracting Fault Positions")
    , panels_(panels),disps_(disps),dpset_(dpset)
    , curidx_(0)
{}

od_int64 nrDone() const override
{ return curidx_; }

od_int64 totalNr() const override
{ return panels_.size(); }

uiString uiNrDoneText() const override
{ return tr("Nr Surfaces Done"); }

int nextStep() override
{
    if ( curidx_ >= panels_.size() )
	return Finished();

    Geometry::ExplFaultStickSurface* panel =
	    const_cast<Geometry::ExplFaultStickSurface*>( panels_[curidx_] );
    if ( !panel->getTexturePositions(dpset_,curidx_,0) )
	return ErrorOccurred();

    visBase::GeomIndexedShape* disp =
	    const_cast<visBase::GeomIndexedShape*>( disps_[curidx_] );
    disp->touch( false, false );
    curidx_++;
    return MoreToDo();
}

    const ObjectSet<Geometry::ExplFaultStickSurface>&	panels_;
    const ObjectSet<visBase::GeomIndexedShape>&		disps_;
    DataPointSet&					dpset_;
    int							curidx_;

};

bool FaultSetDisplay::getRandomPos(
				DataPointSet& dpset, TaskRunner* taskr ) const
{
    for ( int idx=0; idx<datapacks_.size(); idx++ )
    {
	ConstRefMan<DataPointSet> dps = datapacks_[idx];
	if ( !dps || dps->size() < 1 )
	    continue;

	dpset = *dps.ptr();
	dpset.dataSet().removeColumn( dps->dataSet().nrCols()-1 );
	return true;
    }

    if ( !faultset_ || explicitpanels_.isEmpty() )
	return false;

    RandomPosExtractor exec( explicitpanels_, paneldisplays_, dpset );
    return TaskRunner::execute( taskr, exec );
}


bool FaultSetDisplay::getRandomPosCache( int attrib, DataPointSet& data ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return false;

    ConstRefMan<PointDataPack> pointdp = getPointDataPack( attrib );
    mDynamicCastGet(const DataPointSet*,dps,pointdp.ptr());
    if ( dps )
	data  = *dps;

    return true;
}


bool FaultSetDisplay::setRandomPosData( int attrib, const DataPointSet* dpset,
					TaskRunner* taskr )
{
    const DataColDef iddef( sKey::ID() );
    const int idcol =
	dpset->dataSet().findColDef(iddef,PosVecDataSet::NameExact);

    auto* dps = const_cast<DataPointSet*>( dpset );
    return setRandomPosDataInternal( attrib, dps, idcol+1, nullptr );
}


bool FaultSetDisplay::setRandomPosDataInternal( int attrib,
		    const DataPointSet* dpset, int column, TaskRunner* taskr )
{
    if ( attrib>=nrAttribs() || !dpset || dpset->nrCols()<3 ||
	 explicitpanels_.isEmpty() )
    {
	validtexture_ = false;
	updateSingleColor();
	return false;
    }

    setPointDataPack( attrib, const_cast<DataPointSet*>( dpset ), taskr );
    const DataColDef texturei(Geometry::ExplFaultStickSurface::sKeyTextureI());
    const DataColDef texturej(Geometry::ExplFaultStickSurface::sKeyTextureJ());
    const DataColDef iddef(sKey::ID());
    const int columni =
	dpset->dataSet().findColDef(texturei,PosVecDataSet::NameExact);
    const int columnj =
	dpset->dataSet().findColDef(texturej,PosVecDataSet::NameExact);
    const int columnid =
	dpset->dataSet().findColDef(iddef,PosVecDataSet::NameExact);

    const int idxi = columni - dpset->nrFixedCols();
    const int idxj = columnj - dpset->nrFixedCols();
    const int idxval = column - dpset->nrFixedCols();
    const int idxid = columnid - dpset->nrFixedCols();

    for ( int fidx=0; fidx<faultset_->nrFaults(); fidx++ )
    {
	const RowCol sz = explicitpanels_[fidx]->getTextureSize();
	ObjectSet<Array2D<float> >* texturedatas = texturedataset_[fidx];

	while ( texturedatas->size()-1 < attrib )
	    (*texturedatas) += nullptr;

	mDeclareAndTryAlloc( Array2D<float>*, texturedata,
			     Array2DImpl<float>(sz.col(),sz.row()) );
	if ( texturedata && texturedata->isOK() )
	{
	    texturedata->setAll( mUdf(float) );
	    delete texturedatas->replace( attrib, texturedata );
	}
    }

    for ( int idx=0; idx<dpset->size(); idx++ )
    {
	const DataPointSet::DataRow dr = dpset->dataRow( idx );
	const float i = dr.data_[idxi];
	const float j = dr.data_[idxj];
	const int fidx = mNINT32( dr.data_[idxid] );
	ObjectSet<Array2D<float> >* texturedatas = texturedataset_[fidx];
	Array2D<float>* texturedata = (*texturedatas)[attrib];
	texturedata->set( mNINT32(j), mNINT32(i), dr.data_[idxval] );
    }

    for ( int fidx=0; fidx<faultset_->nrFaults(); fidx++ )
    {
	ObjectSet<Array2D<float> >* texturedatas = texturedataset_[fidx];
	Array2D<float>* texturedata = (*texturedatas)[attrib];
	channelset_[fidx]->setSize( attrib, 1, texturedata->info().getSize(0),
			    texturedata->info().getSize(1) );
	channelset_[fidx]->setUnMappedData( attrib, 0, texturedata->getData(),
				    OD::UsePtr, taskr );
    }

    validtexture_ = true;
    updateSingleColor();
    return true;
}


void FaultSetDisplay::setResolution( int res, TaskRunner* taskr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
}


bool FaultSetDisplay::getCacheValue( int attrib, int version, const Coord3& crd,
				  float& value ) const
{ return true; }


void FaultSetDisplay::addCache()
{
    datapacks_ += nullptr;
}

void FaultSetDisplay::removeCache( int attrib )
{
    if ( !datapacks_.validIdx(attrib) )
	return;

    datapacks_.removeSingle( attrib );
}

void FaultSetDisplay::swapCache( int attr0, int attr1 )
{}

void FaultSetDisplay::emptyCache( int attrib )
{}

bool FaultSetDisplay::hasCache( int attrib ) const
{ return false; }


void FaultSetDisplay::setOnlyAtSectionsDisplay( bool yn )
{
    displayIntersections( yn );
}


bool FaultSetDisplay::displayedOnlyAtSections() const
{ return areIntersectionsDisplayed(); }


void FaultSetDisplay::displayIntersections( bool yn )
{
    displayintersections_ = yn;
    updateDisplay();
    displaymodechange.trigger();
}


bool FaultSetDisplay::areIntersectionsDisplayed() const
{ return displayintersections_; }


bool FaultSetDisplay::canDisplayIntersections() const
{
    if ( scene_ )
    {
	for ( int idx=0; idx<scene_->size(); idx++ )
	{
	    visBase::DataObject* dataobj = scene_->getObject( idx );
	    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	    mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	    mDynamicCastGet( RandomTrackDisplay*,rdtd,dataobj);
	    if ( (plane && plane->isOn()) ||
		 (s2dd && s2dd->isOn())   ||
		 (rdtd && rdtd->isOn()) )
		return true;
	}
    }

    return displayintersections_;
}


void FaultSetDisplay::displayHorizonIntersections( bool yn )
{
    displayhorintersections_ = yn;
    updateDisplay();
    displaymodechange.trigger();
}


bool FaultSetDisplay::areHorizonIntersectionsDisplayed() const
{  return displayhorintersections_; }


bool FaultSetDisplay::canDisplayHorizonIntersections() const
{
    if ( scene_ )
    {
	for ( int idx=0; idx<scene_->size(); idx++ )
	{
	    visBase::DataObject* dataobj = scene_->getObject( idx );
	    mDynamicCastGet( HorizonDisplay*, hor, dataobj );
	    if ( hor && hor->isOn() )
		return true;
	}
    }

    return displayhorintersections_;
}


static int getValidIntersectionObjectIdx( bool horizonintersection,
				    const ObjectSet<const SurveyObject>& objs,
				    const VisID& objid )
{
    for ( int idx=0; objid.isValid() && idx<objs.size(); idx++ )
    {
	if ( horizonintersection )
	{
	    mDynamicCastGet( const HorizonDisplay*, hor, objs[idx] );
	    if ( hor && hor->id()==objid )
		return idx;
	}
	else
	{
	    mDynamicCastGet( const RandomTrackDisplay*, rdtd, objs[idx] );
	    if ( rdtd && rdtd->id()==objid )
		return idx;

	    mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	    if ( plane && plane->id()==objid )
		return idx;
	}
    }
    return -1;
}


void FaultSetDisplay::updateHorizonIntersections( const VisID& whichobj,
	const ObjectSet<const SurveyObject>& objs )
{
    if ( !faultset_ )
	return;
/*
    const bool doall = whichobj==-1 || whichobj==id();
    const int onlyidx = getValidIntersectionObjectIdx( true, objs, whichobj );
    if ( !doall && onlyidx<0 )
	return;

    ObjectSet<HorizonDisplay> activehordisps;
    TypeSet<int> activehorids;
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	if ( !doall && idx!=onlyidx )
	    continue;

	mDynamicCastGet( const HorizonDisplay*, hor, objs[idx] );
	if ( !hor || !hor->isOn() || !hor->getSectionIDs().size() )
	    continue;
	if ( hor->displayedOnlyAtSections() )
	    continue;

	activehordisps += const_cast<HorizonDisplay*>( hor );
	activehorids += hor->id();
    }

    for ( int idx=horintersections_.size()-1; idx>=0; idx-- )
    {

	if ( whichobj>=0 && horintersectids_[idx]!=whichobj )
	    continue;
	if ( whichobj<0 && activehorids.isPresent(horintersectids_[idx]) )
	    continue;

	horintersections_[idx]->turnOn( false );
	horintersections_.removeSingle( idx );
	delete horshapes_.removeSingle( idx );
	horintersectids_.removeSingle( idx );
    }

    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     faultset_->geometryElement())

    for ( int idx=0; idx<activehordisps.size(); idx++ )
    {
	if ( horintersectids_.isPresent(activehorids[idx]) )
	    continue;

	EM::SectionID sid = activehordisps[idx]->getSectionIDs()[0];
	Geometry::BinIDSurface* surf =
	    activehordisps[idx]->getHorizonSection(sid)->getSurface();

	RefMan<visBase::GeomIndexedShape> line =
					visBase::GeomIndexedShape::create();
	if ( !line->getMaterial() )
	{
	    RefMan<visBase::Material> newmat = visBase::Material::create();
	    line->setMaterial( newmat.ptr() );
	}

	line->getMaterial()->setColor( nontexturecol_ );
	line->setSelectable( false );
	line->setGeometryShapeType( visBase::GeomIndexedShape::PolyLine3D,
	    Geometry::PrimitiveSet::LineStrips );
	line->setDisplayTransformation( displaytransform_.ptr() );
	addChild( line->osgNode() );
	line->turnOn( false );
	Geometry::ExplFaultStickSurface* shape = nullptr;
	mTryAlloc( shape, Geometry::ExplFaultStickSurface(0,mZScale()) );
	shape->setSceneIdx( mSceneIdx );
	line->setSurface( shape );
	shape->display( false, false );
	shape->setSurface( fss );
	const float zshift = (float) activehordisps[idx]->getTranslation().z;
	Geometry::FaultBinIDSurfaceIntersector it( zshift, *surf,
		*explicitpanels_, *shape->coordList() );
	it.setShape( *shape );
	it.compute();

	line->touch( true, false );
	horintersections_ += line;
	horshapes_ += shape;
	horintersectids_ += activehorids[idx];
    }

    updateHorizonIntersectionDisplay(); */
}


void FaultSetDisplay::otherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    const VisID& whichobj )
{
    updateHorizonIntersections( whichobj, objs );

    if ( explicitintersections_.isEmpty() ) return;

    const bool doall = !whichobj.isValid() || whichobj==id();
    const int onlyidx = getValidIntersectionObjectIdx( false, objs, whichobj );
    if ( !doall && onlyidx<0 )
	return;

    ObjectSet<const SurveyObject> usedobjects;
    TypeSet<int> planeids;
    otherobjects_ = false;

    for ( int idx=0; idx<objs.size(); idx++ )
    {
	if ( !doall && idx!=onlyidx )
	    continue;

	mDynamicCastGet( const RandomTrackDisplay*, rdtd, objs[idx] );
	mDynamicCastGet( const PlaneDataDisplay*, plane, objs[idx] );
	if ( !plane || !plane->isOn() )
	{
	    if ( !rdtd || !rdtd->isOn() )
		continue;
	}

	otherobjects_ = true;

	const TrcKeyZSampling cs = plane ?
				   plane->getTrcKeyZSampling(true,true,-1) :
				   rdtd->getTrcKeyZSampling(false,-1);

	const BinID b00 = cs.hsamp_.start_, b11 = cs.hsamp_.stop_;
	BinID b01, b10;

	if ( plane && plane->getOrientation()==OD::SliceType::Z )
	{
	    b01 = BinID( cs.hsamp_.start_.inl(), cs.hsamp_.stop_.crl() );
	    b10 = BinID( cs.hsamp_.stop_.inl(), cs.hsamp_.start_.crl() );
	}
	else
	{
	    b01 = b00;
	    b10 = b11;
	}

	const Coord3 c00( s3dgeom_->transform(b00),cs.zsamp_.start_ );
	const Coord3 c01( s3dgeom_->transform(b01),cs.zsamp_.stop_ );
	const Coord3 c11( s3dgeom_->transform(b11),cs.zsamp_.stop_ );
	const Coord3 c10( s3dgeom_->transform(b10),cs.zsamp_.start_ );

	const Coord3 normal = (c01-c00).cross(c10-c00).normalize();

	TypeSet<Coord3> positions;
	positions += c00;
	positions += c01;
	positions += c11;
	positions += c10;

	const int idy = intersectionobjs_.indexOf( objs[idx] );
	if ( idy==-1 )
	{
	    usedobjects += objs[idx];
	    int planeid = -1;
	    for ( int fidx=0; fidx<explicitintersections_.size(); fidx++ )
	    {
		const int pid =
		    explicitintersections_[fidx]->addPlane( normal, positions );
		if ( pid >= 0 )
		    planeid = pid;
	    }

	    planeids.add( planeid );
	}
	else
	{
	    usedobjects += objs[idx];
	    for ( int fidx=0; fidx<explicitintersections_.size(); fidx++ )
		explicitintersections_[fidx]->setPlane( planeids_[idy],
							normal, positions );

	    planeids += planeids_[idy];

	    intersectionobjs_.removeSingle( idy );
	    planeids_.removeSingle( idy );
	}
    }

    for ( int idx=planeids_.size()-1; idx>=0; idx-- )
    {
	for ( int fidx=0; fidx<explicitintersections_.size(); fidx++ )
	    explicitintersections_[fidx]->removePlane( planeids_[idx] );
    }

    intersectionobjs_ = usedobjects;
    planeids_ = planeids;
    updateIntersectionDisplay();
}


const OD::LineStyle* FaultSetDisplay::lineStyle() const
{ return &drawstyle_->lineStyle(); }


void FaultSetDisplay::setLineStyle( const OD::LineStyle& lst )
{
    if ( lineStyle()->width_<0 || lst.width_<0 )
    {
	drawstyle_->setLineStyle( lst );
	scene_->objectMoved( 0 );
    }
    else
	drawstyle_->setLineStyle( lst );

    updateDisplay();
}


void FaultSetDisplay::setLineRadius( visBase::GeomIndexedShape* shape )
{
    const bool islinesolid = lineStyle()->type_ == OD::LineStyle::Solid;
    const float linewidth = islinesolid ? 0.5f*lineStyle()->width_ : -1.0f;

    OD::LineStyle lnstyle( *lineStyle() ) ;
    lnstyle.width_ = (int)( 2*linewidth );

    if ( shape )
	shape->setLineStyle( lnstyle );
}


EM::FaultID FaultSetDisplay::getFaultID(
			const visBase::EventInfo& eventinfo ) const
{
    if ( !faultset_ )
	return EM::FaultID::udf();

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	const VisID visid = eventinfo.pickedobjids[idx];
	for ( int fidx=0; fidx<paneldisplays_.size(); fidx++ )
	{
	    if ( paneldisplays_[fidx]->id() == visid )
		return faultset_->getFaultID( fidx );
	}

	for ( int fidx=0; fidx<intersectiondisplays_.size(); fidx++ )
	{
	    if ( intersectiondisplays_[fidx]->id() == visid )
		return faultset_->getFaultID( fidx );
	}
    }

    return EM::FaultID::udf();
}


void FaultSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
		    Coord3& pos, BufferString& val, uiString& info ) const
{
    info.setEmpty();
    val.setEmpty();
    if ( !faultset_ )
	return;

    const EM::FaultID selid = getFaultID( eventinfo );
    uiString faultmsg = uiStrings::sFaultSet();
    faultmsg.addMoreInfo( faultset_->name() );
    uiString idmsg = uiStrings::sID();
    idmsg.addMoreInfo( selid.asInt() );
    info = ::toUiString( "%1	%2" ).arg( faultmsg ).arg( idmsg );
}


bool FaultSetDisplay::setPointDataPack( int attrib, PointDataPack* dp,
					TaskRunner* /* taskr */ )
{
    mDynamicCastGet(DataPointSet*,dps,dp);
    if ( !dps || !datapacks_.validIdx(attrib) )
	return false;

    datapacks_.replace( attrib, dps );
    return true;
}


ConstRefMan<DataPack> FaultSetDisplay::getDataPack( int attrib ) const
{
    return getPointDataPack( attrib );
}


ConstRefMan<PointDataPack> FaultSetDisplay::getPointDataPack( int attrib ) const
{
    if ( !datapacks_.validIdx(attrib) )
	return nullptr;

    ConstRefMan<DataPointSet> datapack = datapacks_.get( attrib );
    return datapack ? datapack.ptr() : nullptr;
}


void FaultSetDisplay::matChangeCB(CallBacker*)
{
    for ( int idx=0; idx<paneldisplays_.size(); idx++ )
	paneldisplays_[idx]->updateMaterialFrom( getMaterial() );
}


void FaultSetDisplay::setPixelDensity( float dpi )
{
    MultiTextureSurveyObject::setPixelDensity( dpi );

    for ( int idx=0; idx<paneldisplays_.size(); idx++ )
    {
	paneldisplays_[idx]->setPixelDensity( dpi );
	intersectiondisplays_[idx]->setPixelDensity( dpi );
    }
}


void FaultSetDisplay::setAttribTransparency( int attrib, unsigned char nt )
{
    for ( int idx=0; idx<channelset_.size(); idx++ )
    {
	mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*, cttc2rgba,
			 channelset_[idx]->getChannels2RGBA() );
	if ( cttc2rgba )
	    cttc2rgba->setTransparency( attrib, nt );
    }
}


void FaultSetDisplay::enableAttrib( int attrib, bool yn )
{
    for ( int idx=0; idx<channelset_.size(); idx++ )
	channelset_[idx]->getChannels2RGBA()->setEnabled( attrib, yn );

    updateMainSwitch();
    updateSingleColor();
}


bool FaultSetDisplay::addAttrib()
{
    if ( !MultiTextureSurveyObject::addAttrib() )
	return false;

    for ( int idx=1; idx<channelset_.size(); idx++ )
    {
	while ( channelset_[idx]->nrChannels()<channels_->nrChannels() )
	    channelset_[idx]->addChannel();
    }

    updateMainSwitch();
    return true;
}


bool FaultSetDisplay::removeAttrib( int attrib )
{
    if ( !MultiTextureSurveyObject::removeAttrib(attrib) )
	return false;

    for ( int idx=1; idx<channelset_.size(); idx++ )
	channelset_[idx]->removeChannel( attrib );

    updateMainSwitch();
    return true;
}


bool FaultSetDisplay::swapAttribs( int a0, int a1 )
{
    if ( !MultiTextureSurveyObject::swapAttribs(a0,a1) )
	return false;

    for ( int idx=1; idx<channelset_.size(); idx++ )
	channelset_[idx]->swapChannels( a0, a1 );

    return true;
}


const visBase::GeomIndexedShape* FaultSetDisplay::getFaultDisplayedPlane(
								int idx) const
{
    return paneldisplays_.validIdx(idx) ? paneldisplays_[idx] : nullptr;
}


const TypeSet<float>* FaultSetDisplay::getHistogram( int attrib ) const
{
    static TypeSet<float> ret;
    for ( int idx=0; idx<channelset_.size(); idx++ )
    {
	const TypeSet<float>* hist = channelset_[idx]->getHistogram( attrib );
	if ( hist )
	    ret.append( *hist );
    }

    return &ret;
}


void FaultSetDisplay::setColTabSequence( int attrib,
			      ColTab::Sequence const& seq, TaskRunner* )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    for ( int idx=0; idx<channelset_.size(); idx++ )
	channelset_[idx]->getChannels2RGBA()->setSequence( attrib, seq );
}


bool FaultSetDisplay::canSetColTabSequence() const
{
    return channels_->getChannels2RGBA()->canSetSequence();
}


void FaultSetDisplay::setColTabMapperSetup( int attrib,
			      const ColTab::MapperSetup& mapper, TaskRunner* )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    const ColTab::MapperSetup& old = channels_->getColTabMapperSetup(attrib,0);
    if ( old!=mapper )
    {
	const bool needsreclip = old.needsReClip( mapper );
	for ( int idx=0; idx<channelset_.size(); idx++ )
	{
	    channelset_[idx]->setColTabMapperSetup( attrib, mapper );
	    channelset_[idx]->reMapData( attrib, !needsreclip, 0 );
	}
    }
}


const ColTab::MapperSetup*
FaultSetDisplay::getColTabMapperSetup( int attrib, int version ) const
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return nullptr;

    static ColTab::MapperSetup ms;
    if ( mIsUdf(version) || version<0
			 || version >= channels_->nrVersions(attrib) )
	version = channels_->currentVersion( attrib );

    ms = channels_->getColTabMapperSetup( attrib, version );
    for ( int idx=1; idx<channelset_.size(); idx++ )
	ms.range_.include( channelset_[idx]->getColTabMapperSetup(
					    attrib,version).range_, false );

    return &ms;
}

} // namespace visSurvey
