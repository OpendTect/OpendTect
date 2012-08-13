/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUnusedVar = "$Id: visplanedatadisplay.cc,v 1.274 2012-08-13 04:04:40 cvsaneesh Exp $";

#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "array2dresample.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "basemap.h"
#include "coltabsequence.h"
#include "cubesampling.h"
#include "datapointset.h"
#include "flatposdata.h"
#include "iopar.h"
#include "keyenum.h"
#include "settings.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "zdomain.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdepthtabplanedragger.h"
#include "visdrawstyle.h"
#include "visfaceset.h"
#include "visevent.h"
#include "visgridlines.h"
#include "vismaterial.h"
#include "vismultitexture2.h"
#include "vistexturechannels.h"
#include "vispickstyle.h"
#include "vissplittexture2rectangle.h"
#include "vistexturecoords.h"
#include "vistexturerect.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "zaxistransform.h"
#include "zaxistransformdatapack.h"


mCreateFactoryEntry( visSurvey::PlaneDataDisplay );

namespace visSurvey {

class PlaneDataDisplayBaseMapObject : public BaseMapObject
{
public:
		PlaneDataDisplayBaseMapObject(PlaneDataDisplay* pdd);

    const char*		getType() const;
    void		updateGeometry();
    int			nrShapes() const;
    const char*		getShapeName(int) const;
    void		getPoints(int,TypeSet<Coord>& res) const;
    const LineStyle*	getLineStyle(int) const { return &lst_; }
    bool		close(int) const;

protected:
    LineStyle			lst_;
    PlaneDataDisplay*		pdd_;
};


PlaneDataDisplayBaseMapObject::PlaneDataDisplayBaseMapObject(
							PlaneDataDisplay* pdd)
    : BaseMapObject( pdd->name() )
    , pdd_( pdd )
{}


const char* PlaneDataDisplayBaseMapObject::getType() const
{ return PlaneDataDisplay::getOrientationString(pdd_->getOrientation()); }


void PlaneDataDisplayBaseMapObject::updateGeometry()
{
    changed.trigger();
}


int PlaneDataDisplayBaseMapObject::nrShapes() const
{ return 1; }


const char* PlaneDataDisplayBaseMapObject::getShapeName(int) const
{ return pdd_->name(); }


void PlaneDataDisplayBaseMapObject::getPoints(int,TypeSet<Coord>& res) const
{
    const HorSampling hrg = pdd_->getCubeSampling(true,false).hrg;
    const InlCrlSystem* survinfo = pdd_->getInlCrlSystem();
    if ( pdd_->getOrientation()==PlaneDataDisplay::Zslice )
    {
	res += survinfo->transform(hrg.start);
	res += survinfo->transform(BinID(hrg.start.inl, hrg.stop.crl) );
	res += survinfo->transform(hrg.stop);
	res += survinfo->transform(BinID(hrg.stop.inl, hrg.start.crl) );
    }
    else
    {
	res += survinfo->transform(hrg.start);
	res += survinfo->transform(hrg.stop);
    }
}


bool PlaneDataDisplayBaseMapObject::close(int) const
{
    return pdd_->getOrientation()==PlaneDataDisplay::Zslice;
}



DefineEnumNames(PlaneDataDisplay,Orientation,1,"Orientation")
{ "Inline", "Crossline", "Z-slice", 0 };

PlaneDataDisplay::PlaneDataDisplay()
    : MultiTextureSurveyObject( true )
    , rectangle_( visBase::SplitTexture2Rectangle::create() )
    , rectanglepickstyle_( visBase::PickStyle::create() )
    , dragger_( visBase::DepthTabPlaneDragger::create() )
    , gridlines_( visBase::GridLines::create() )
    , curicstep_(inlcrlsystem_->inlStep(),inlcrlsystem_->crlStep())
    , datatransform_( 0 )
    , voiidx_(-1)
    , moving_(this)
    , movefinished_(this)
    , orientation_( Inline )
    , csfromsession_( false )
    , eventcatcher_( 0 )
    , minx0step_( -1 )
    , minx1step_( -1 )
    , inl2displaytrans_( 0 )
    , texturerect_( 0 )
{
    if ( doOsg() )
    {
	inl2displaytrans_ = mVisTrans::create();
	inl2displaytrans_->ref();
	addChild( inl2displaytrans_->osgNode() );

	texturerect_ = visBase::TextureRectangle::create();
	inl2displaytrans_->addObject( texturerect_ );
	texturerect_->setTextureChannels( channels_ );

	inl2displaytrans_->addObject( dragger_ );
    }

    volumecache_.allowNull( true );
    rposcache_.allowNull( true );
    dragger_->ref();

    int channelidx = childIndex( channels_->getInventorNode() );

    insertChild( channelidx++, dragger_->getInventorNode() );
    dragger_->motion.notify( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.notify( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->notify(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    draggerrect_ = visBase::FaceSet::create();
    draggerrect_->ref();
    draggerrect_->removeSwitch();
    draggerrect_->setVertexOrdering(
	    visBase::VertexShape::cCounterClockWiseVertexOrdering() );
    draggerrect_->getCoordinates()->addPos( Coord3(-1,-1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(1,-1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(1,1,0) );
    draggerrect_->getCoordinates()->addPos( Coord3(-1,1,0) );
    draggerrect_->setCoordIndex( 0, 0 );
    draggerrect_->setCoordIndex( 1, 1 );
    draggerrect_->setCoordIndex( 2, 2 );
    draggerrect_->setCoordIndex( 3, 3 );
    draggerrect_->setCoordIndex( 4, -1 );

    draggermaterial_ = visBase::Material::create();
    draggermaterial_->ref();
    draggerrect_->setMaterial( draggermaterial_ );

    draggerdrawstyle_ = visBase::DrawStyle::create();
    draggerdrawstyle_->ref();
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggerrect_->insertNode( draggerdrawstyle_->getInventorNode() );

    dragger_->setOwnShape( draggerrect_->getInventorNode() );
    dragger_->setDim( (int) 0 );

    if ( (int) orientation_ )
	dragger_->setDim( (int) orientation_ );

    rectanglepickstyle_->ref();
    addChild( rectanglepickstyle_->getInventorNode() );

    rectangle_->ref();
    rectangle_->removeSwitch();
    rectangle_->setMaterial( 0 );
    addChild( rectangle_->getInventorNode() );
    material_->setColor( Color::White() );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.2 );

    gridlines_->ref();
    insertChild( channelidx, gridlines_->getInventorNode() );

    updateRanges( true, true );

    int buttonkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyDepthKey(), buttonkey );
    dragger_->setTransDragKeys( true, buttonkey );
    buttonkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyPlaneKey(), buttonkey );
    dragger_->setTransDragKeys( false, buttonkey );
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    setSceneEventCatcher( 0 );
    dragger_->motion.remove( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.remove( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->remove(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    deepErase( rposcache_ );
    setZAxisTransform( 0,0 );

    for ( int idx=volumecache_.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( volumecache_[idx] );

    for ( int idy=0; idy<displaycache_.size(); idy++ )
    {
	const TypeSet<DataPack::ID>& dpids = *displaycache_[idy];
	for ( int idx=dpids.size()-1; idx>=0; idx-- )
	    DPM(DataPackMgr::FlatID()).release( dpids[idx] );
    }

    deepErase( displaycache_ );

    rectangle_->unRef();
    dragger_->unRef();
    rectanglepickstyle_->unRef();
    gridlines_->unRef();
    draggerrect_->unRef();
    draggerdrawstyle_->unRef();
    draggermaterial_->unRef();

    setBaseMap( 0 );

    if ( inl2displaytrans_ ) inl2displaytrans_->unRef();
}


void PlaneDataDisplay::setInlCrlSystem(const InlCrlSystem* ics )
{
    SurveyObject::setInlCrlSystem( ics );
    if ( inl2displaytrans_ )
    {
	STM().setIC2DispayTransform(inlcrlsystem_->sampling().hrg,
				    inl2displaytrans_);
    }
}


void PlaneDataDisplay::setOrientation( Orientation nt )
{
    if ( orientation_==nt )
	return;

    orientation_ = nt;

    dragger_->setDim( (int) nt );
    updateRanges( true, true );
}


void PlaneDataDisplay::updateRanges( bool resetic, bool resetz )
{
    if ( !scene_ )
	return;

    CubeSampling survey = scene_->getCubeSampling();
    const Interval<float> inlrg( survey.hrg.start.inl, survey.hrg.stop.inl );
    const Interval<float> crlrg( survey.hrg.start.crl, survey.hrg.stop.crl );

    dragger_->setSpaceLimits( inlrg, crlrg, survey.zrg );
    dragger_->setWidthLimits(
	    Interval<float>( 4*survey.hrg.step.inl, mUdf(float) ),
	    Interval<float>( 4*survey.hrg.step.crl, mUdf(float) ),
	    Interval<float>( 4*survey.zrg.step, mUdf(float) ) );

    CubeSampling newpos = getCubeSampling(false,true);
    if ( !newpos.isEmpty() )
    {
	if ( !survey.includes( newpos ) )
	    newpos.limitTo( survey );
    }

    if ( !newpos.hrg.isEmpty() && !resetic && resetz )
	survey.hrg = newpos.hrg;

    if ( resetic || resetz || newpos.isEmpty() )
    {
	newpos = survey;
	if ( orientation_==Zslice && datatransform_ && resetz )
	{
	    const float center = survey.zrg.snappedCenter();
	    if ( !mIsUdf(center) )
		newpos.zrg.start = newpos.zrg.stop = center;
	}
    }

    newpos = snapPosition( newpos );

    if ( newpos!=getCubeSampling(false,true) )
	setCubeSampling( newpos );
}


CubeSampling PlaneDataDisplay::snapPosition( const CubeSampling& cs ) const
{
    CubeSampling res( cs );
    const Interval<float> inlrg( res.hrg.start.inl, res.hrg.stop.inl );
    const Interval<float> crlrg( res.hrg.start.crl, res.hrg.stop.crl );
    const Interval<float> zrg( res.zrg );

    res.hrg.snapToSurvey();
    if ( scene_ )
    {
    	const StepInterval<float>& scenezrg = scene_->getCubeSampling().zrg;
    	res.zrg.limitTo( scenezrg );
    	res.zrg.start = scenezrg.snap( res.zrg.start );
    	res.zrg.stop = scenezrg.snap( res.zrg.stop );

	if ( orientation_!=Inline && orientation_!=Crossline )
	    res.zrg.start = res.zrg.stop = scenezrg.snap(zrg.center());
    }

    if ( orientation_==Inline )
	res.hrg.start.inl = res.hrg.stop.inl =
	    inlcrlsystem_->inlRange().snap( inlrg.center() );
    else if ( orientation_==Crossline )
	res.hrg.start.crl = res.hrg.stop.crl =
	    inlcrlsystem_->crlRange().snap( crlrg.center() );

    return res;
}


Coord3 PlaneDataDisplay::getNormal( const Coord3& pos ) const
{
    if ( orientation_==Zslice )
	return Coord3(0,0,1);
    
    return Coord3( orientation_==Inline
		  ? inlcrlsystem_->binID2Coord().rowDir()
		  : inlcrlsystem_->binID2Coord().colDir(), 0 );
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = inlcrlsystem_->transform( Coord(xytpos.x,xytpos.y) );

    const CubeSampling cs = getCubeSampling(false,true);
    
    BinID inlcrldist( 0, 0 );
    float zdiff = 0;

    inlcrldist.inl =
	binid.inl>=cs.hrg.start.inl && binid.inl<=cs.hrg.stop.inl 
	     ? 0
	     : mMIN( abs(binid.inl-cs.hrg.start.inl),
		     abs( binid.inl-cs.hrg.stop.inl) );
    inlcrldist.crl =
	binid.crl>=cs.hrg.start.crl && binid.crl<=cs.hrg.stop.crl 
	     ? 0
	     : mMIN( abs(binid.crl-cs.hrg.start.crl),
		     abs( binid.crl-cs.hrg.stop.crl) );
    const float zfactor = scene_
    	? scene_->getZScale()
        : inlcrlsystem_->zScale();
    zdiff = cs.zrg.includes(xytpos.z,false)
	? 0
	: (float)(mMIN(fabs(xytpos.z-cs.zrg.start), fabs(xytpos.z-cs.zrg.stop))  
	               * zfactor  * scene_->getZStretch() );

    const float inldist = inlcrlsystem_->inlDistance();
    const float crldist = inlcrlsystem_->crlDistance();
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return Math::Sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


float PlaneDataDisplay::maxDist() const
{
    const float zfactor = scene_ ? scene_->getZScale() : inlcrlsystem_->zScale();
    float maxzdist = zfactor * scene_->getZStretch() * inlcrlsystem_->zStep() / 2;
    return orientation_==Zslice ? maxzdist : SurveyObject::sDefMaxDist();
}


bool PlaneDataDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,PlaneDataDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* PlaneDataDisplay::getZAxisTransform() const
{ return datatransform_; }


void PlaneDataDisplay::setTranslationDragKeys( bool depth, int ns )
{ dragger_->setTransDragKeys( depth, ns ); }


int PlaneDataDisplay::getTranslationDragKeys(bool depth) const
{ return dragger_->getTransDragKeys( depth ); }


void PlaneDataDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int idx=0; idx<volumecache_.size(); idx++ )
    {
	if ( volumecache_[idx] )
	{
	    setVolumeDataPackNoCache( idx, volumecache_[idx] );
	}
	else if ( rposcache_[idx] )
	{
	    BinIDValueSet set(*rposcache_[idx]);
	    setRandomPosDataNoCache( idx, &set, 0 );
	}
    }
}


void PlaneDataDisplay::draggerMotion( CallBacker* )
{
    moving_.trigger();

    const CubeSampling dragcs = getCubeSampling(true,true);
    const CubeSampling snappedcs = snapPosition( dragcs );
    const CubeSampling oldcs = getCubeSampling(false,true);

    bool showplane = false;
    if ( orientation_==Inline && dragcs.hrg.start.inl!=oldcs.hrg.start.inl )
	showplane = true;
    else if ( orientation_==Crossline &&
	      dragcs.hrg.start.crl!=oldcs.hrg.start.crl )
	showplane = true;
    else if ( orientation_==Zslice && dragcs.zrg.start!=oldcs.zrg.start )
	showplane = true;
   
    draggerdrawstyle_->setDrawStyle( showplane
	    ? visBase::DrawStyle::Filled
	    : visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( showplane ? 0.5f : 0 );

    if ( doOsg() )
    {
	dragger_->showPlane( showplane );
	dragger_->showDraggerBorder( !showplane );
    }
}


void PlaneDataDisplay::draggerFinish( CallBacker* )
{
    const CubeSampling cs = getCubeSampling(true,true);
    const CubeSampling snappedcs = snapPosition( cs );

    if ( cs!=snappedcs )
	setDraggerPos( snappedcs );
}


void PlaneDataDisplay::draggerRightClick( CallBacker* cb )
{
    triggerRightClick( dragger_->rightClickedEventInfo() );
}
 
#define mDefineCenterAndWidth( thecs ) \
    const Coord3 center( (thecs.hrg.start.inl+thecs.hrg.stop.inl)/2.0, \
		         (thecs.hrg.start.crl+thecs.hrg.stop.crl)/2.0, \
		         thecs.zrg.center() ); \
    Coord3 width( thecs.hrg.stop.inl-thecs.hrg.start.inl, \
		  thecs.hrg.stop.crl-thecs.hrg.start.crl, thecs.zrg.width() ); \
    if ( width.x < 1 ) width.x = 1; \
    if ( width.y < 1 ) width.y = 1; \
    if ( width.z < thecs.zrg.step * 0.5 ) width.z = 1; \
 \
    const Coord3 oldwidth = dragger_->size(); \
    width[(int)orientation_] = oldwidth[(int)orientation_]

void PlaneDataDisplay::setDraggerPos( const CubeSampling& cs )
{
    mDefineCenterAndWidth( cs );
    dragger_->setCenter( center );
    dragger_->setSize( width );
}


void PlaneDataDisplay::coltabChanged( CallBacker* )
{
    // Hack for correct transparency display
    bool manipshown = isManipulatorShown();
    if ( manipshown ) return;
    showManipulator( true );
    showManipulator( false );
}


void PlaneDataDisplay::showManipulator( bool yn )
{
    dragger_->turnOn( yn );
    rectanglepickstyle_->setStyle( yn ? visBase::PickStyle::Unpickable
				      : visBase::PickStyle::Shape );
    if ( doOsg() )
	texturerect_->enableTraversal( visBase::IntersectionTraversal, !yn );
}


bool PlaneDataDisplay::isManipulatorShown() const
{
    return dragger_->isOn();
}


bool PlaneDataDisplay::isManipulated() const
{ return getCubeSampling(true,true)!=getCubeSampling(false,true); }


void PlaneDataDisplay::resetManipulation()
{
    CubeSampling cs = getCubeSampling( false, true );
    setDraggerPos( cs );
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( 0 );

    if ( doOsg() )
    {
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
}


void PlaneDataDisplay::acceptManipulation()
{
    CubeSampling cs = getCubeSampling( true, true );
    setCubeSampling( cs );
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( 0 );

    if ( doOsg() )
    {
	dragger_->showPlane( false );
	dragger_->showDraggerBorder( true );
    }
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    getObjectInfo( res );
    return res;
}


NotifierAccess* PlaneDataDisplay::getManipulationNotifier()
{ return &moving_; }


int PlaneDataDisplay::nrResolutions() const
{
    return 3;
}


void PlaneDataDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;

    for ( int idx=0; idx<nrAttribs(); idx++ )
	updateFromDisplayIDs( idx, tr );
}


SurveyObject::AttribFormat
    PlaneDataDisplay::getAttributeFormat( int attrib ) const
{
    if ( alreadyTransformed(attrib) )
	return SurveyObject::Cube;

    return datatransform_ && orientation_==Zslice
	? SurveyObject::RandomPos
	: SurveyObject::Cube;
}


void PlaneDataDisplay::addCache()
{
    volumecache_ += 0;
    rposcache_ += 0;
    displaycache_ += new TypeSet<DataPack::ID>;
}


void PlaneDataDisplay::removeCache( int attrib )
{
    DPM(DataPackMgr::FlatID()).release( volumecache_[attrib] );
    volumecache_.remove( attrib );

    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.remove( attrib );

    const TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    delete displaycache_.remove( attrib );
    
    for ( int idx=0; idx<displaycache_.size(); idx++ )
	updateFromDisplayIDs( idx, 0 );
}


void PlaneDataDisplay::swapCache( int a0, int a1 )
{
    volumecache_.swap( a0, a1 );
    rposcache_.swap( a0, a1 );
    displaycache_.swap( a0, a1 );
}


void PlaneDataDisplay::emptyCache( int attrib )
{
    DPM(DataPackMgr::FlatID()).release( volumecache_[attrib] );
    volumecache_.replace( attrib, 0 );

    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.replace( attrib, 0 );
    
    if ( displaycache_[attrib] )
    {
	TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    	for ( int idx=dpids.size()-1; idx>=0; idx-- )
	    DPM(DataPackMgr::FlatID()).release( dpids[idx] );

	dpids.erase();
    }

    channels_->setNrVersions( attrib, 1 );
    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
}


bool PlaneDataDisplay::hasCache( int attrib ) const
{
    return volumecache_[attrib] || rposcache_[attrib];
}


void PlaneDataDisplay::triggerSel()
{
    updateMouseCursorCB( 0 );
    visBase::VisualObject::triggerSel();
}


void PlaneDataDisplay::triggerDeSel()
{
    updateMouseCursorCB( 0 );
    visBase::VisualObject::triggerDeSel();
}


CubeSampling PlaneDataDisplay::getCubeSampling( int attrib ) const
{
    return getCubeSampling( true, false, attrib );
}


void PlaneDataDisplay::getRandomPos( DataPointSet& pos, TaskRunner* ) const
{
    if ( !datatransform_ ) return;

    const CubeSampling cs = getCubeSampling( true, true, 0 ); //attrib?
    if ( datatransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_<0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	datatransform_->loadDataIfMissing( voiidx_ );
    }

    HorSamplingIterator iter( cs.hrg );
    BinIDValue curpos;
    curpos.value = cs.zrg.start;
    while ( iter.next(curpos.binid) )
    {
	const float depth = datatransform_->transformBack( curpos );
	if ( mIsUdf(depth) )
	    continue;

	DataPointSet::Pos newpos( curpos.binid, depth );
	DataPointSet::DataRow dtrow( newpos );
	pos.addRow( dtrow );
    }
    pos.dataChanged();
}


void PlaneDataDisplay::setRandomPosData( int attrib, const DataPointSet* data,
					 TaskRunner* tr )
{
    if ( attrib>=nrAttribs() )
	return;

    setRandomPosDataNoCache( attrib, &data->bivSet(), tr );

    if ( rposcache_[attrib] ) 
	delete rposcache_[attrib];

    rposcache_.replace( attrib, data ? new BinIDValueSet(data->bivSet()) : 0 );
}


void PlaneDataDisplay::setCubeSampling( const CubeSampling& wantedcs )
{
    CubeSampling cs = snapPosition( wantedcs );
    const HorSampling& hrg = cs.hrg;

    if ( orientation_==Inline || orientation_==Crossline )
    {
	rectangle_->setPosition(
		Coord3( hrg.start.inl, hrg.start.crl, cs.zrg.start ),
		Coord3( hrg.start.inl,  hrg.start.crl,  cs.zrg.stop ),
		Coord3( hrg.stop.inl, hrg.stop.crl, cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.stop.crl,  cs.zrg.stop ) );
    }
    else 
    {
	rectangle_->setPosition(
		Coord3( hrg.start.inl, hrg.start.crl, cs.zrg.stop ),
		Coord3( hrg.start.inl, hrg.stop.crl,  cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.start.crl, cs.zrg.start ),
		Coord3( hrg.stop.inl,  hrg.stop.crl,  cs.zrg.stop ) );
    }

    if ( texturerect_ )
    {
	mDefineCenterAndWidth( cs );
	width[(int)orientation_] = 0;
	texturerect_->setCenter( center );
	texturerect_->setWidth( width );
	texturerect_->swapTextureAxes();
    }

    setDraggerPos( cs );
    if ( gridlines_ ) gridlines_->setPlaneCubeSampling( cs );

    curicstep_ = hrg.step;

    //channels_->clearAll();
    movefinished_.trigger();
    if ( basemapobj_ )
	basemapobj_->updateGeometry();
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos,
						bool displayspace,
       						int attrib ) const
{
    CubeSampling res(false);
    Coord3 c0, c1;

    if ( manippos )
    {
	const Coord3 center = dragger_->center();
	Coord3 halfsize = dragger_->size()/2;
	halfsize[orientation_] = 0;

	c0 = center + halfsize;
	c1 = center - halfsize;
    }
    else
    {
	if ( texturerect_ ) 
	{
	    const Coord3 center = texturerect_->getCenter();
	    Coord3 halfsize = texturerect_->getWidth()/2;
	    halfsize[orientation_] = 0;

	    c0 = center + halfsize;
	    c1 = center - halfsize;
	}
	else
	{
	    c0 = rectangle_->getPosition( false, false );
	    c1 = rectangle_->getPosition( true, true );
	}
    }

    res.hrg.start = res.hrg.stop = BinID(mNINT32(c0.x),mNINT32(c0.y) );
    res.zrg.start = res.zrg.stop = (float) c0.z;
    res.hrg.include( BinID(mNINT32(c1.x),mNINT32(c1.y)) );
    res.zrg.include( (float) c1.z );
    res.hrg.step = BinID( inlcrlsystem_->inlStep(), inlcrlsystem_->crlStep() );
    res.zrg.step = inlcrlsystem_->zRange().step;

    const bool alreadytf = alreadyTransformed( attrib );
    if ( alreadytf )
    {
	if ( scene_ )
	    res.zrg.step = scene_->getCubeSampling().zrg.step;
	else if ( datatransform_ )
	    res.zrg.step = datatransform_->getGoodZStep();
	return res;
    }

    if ( datatransform_ )
    {
	if ( !displayspace )
	{
	    res.zrg.setFrom( datatransform_->getZInterval(true) );
	    res.zrg.step = SI().zRange(true).step;
	}
	else
	{
	    if ( scene_ )
		res.zrg.step = scene_->getCubeSampling().zrg.step;
	    else
		res.zrg.step = datatransform_->getGoodZStep();
	}
    }

    return res;
}


bool PlaneDataDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				      TaskRunner* )
{
    if ( attrib>=nrAttribs() )
	return false;

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet( const Attrib::Flat3DDataPack*, f3ddp, datapack );
    if ( !f3ddp )
    {
	dpman.release( dpid );
	return false;
    }

    setVolumeDataPackNoCache( attrib, f3ddp );
    if ( volumecache_[attrib] )
	dpman.release( volumecache_[attrib] );

    volumecache_.replace( attrib, f3ddp );
    return true;
}


float PlaneDataDisplay::getDisplayMinDataStep( bool x0 ) const
{
    return x0 ? minx0step_ : minx1step_;
}


void PlaneDataDisplay::setVolumeDataPackNoCache( int attrib, 
			const Attrib::Flat3DDataPack* f3ddp )
{
    if ( !f3ddp ) return;
    
    //set display datapack.
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );

#define mLoadFDPs( f3dp, pids, packs ) \
    for ( int cidx=0; cidx<f3dp->cube().nrCubes(); cidx++ ) \
    { \
	if ( f3dp->getCubeIdx()==cidx ) \
	{ \
	    dpman.obtain( f3dp->id() ); \
	    packs += f3dp; \
	    pids += f3dp->id(); \
	} \
	else \
	{ \
	    mDeclareAndTryAlloc( Attrib::Flat3DDataPack*, ndp, \
		    Attrib::Flat3DDataPack(f3dp->descID(),f3dp->cube(),cidx)); \
	    if ( userrefs_[attrib]->size()>cidx ) \
		ndp->setName( userrefs_[attrib]->get(cidx)) ; \
	    dpman.addAndObtain( ndp ); \
	    packs += ndp; \
	    pids += ndp->id();\
	} \
    }

    const bool alreadytransformed = alreadyTransformed( attrib );
    if ( minx0step_<0 || minx1step_<0 )
    {
	minx0step_ = (float) f3ddp->posData().range(true).step;

	if ( alreadytransformed || getOrientation()==Zslice )
	    minx1step_ = (float) f3ddp->posData().range(false).step;
	else if ( scene_ )
	    minx1step_ = scene_->getCubeSampling().zrg.step;
    }

    TypeSet<DataPack::ID> attridpids;
    ObjectSet<const FlatDataPack> displaypacks;
    ObjectSet<const FlatDataPack> tfpacks;
    mLoadFDPs( f3ddp, attridpids, displaypacks );

    //transform data if necessary.
    if ( !alreadytransformed && datatransform_ )
    {
	attridpids.erase();
	for ( int idx=0; idx<displaypacks.size(); idx++ )
	{
	    mDeclareAndTryAlloc( ZAxisTransformDataPack*, ztransformdp,
		ZAxisTransformDataPack( *displaypacks[idx], 
		f3ddp->cube().cubeSampling(), *datatransform_ ) );

	    ztransformdp->setInterpolate( textureInterpolationEnabled() );
	    
	    CubeSampling outputcs = getCubeSampling( true, true );
	    outputcs.hrg.step = f3ddp->cube().cubeSampling().hrg.step;
	    if ( scene_ )
 		outputcs.zrg.step = scene_->getCubeSampling().zrg.step;

	    ztransformdp->setOutputCS( outputcs );
	    if ( !ztransformdp->transform() )
		return;

	    dpman.addAndObtain( ztransformdp );
	    attridpids += ztransformdp->id();
	    tfpacks += ztransformdp;
	    dpman.release( displaypacks[idx] );
	}
    }
    
    const bool usetf = tfpacks.size();
    if ( nrAttribs()>1 )
    {
	const int oldchannelsz0 = channels_->getSize(1) / (resolution_+1);
	const int oldchannelsz1 = channels_->getSize(2) / (resolution_+1);
	
	//check current attribe sizes
	int newsz0 = 0, newsz1 = 0;
	bool hassamesz = true;
	if ( oldchannelsz0 && oldchannelsz1 )
	{
	    for ( int idx=0; idx<attridpids.size(); idx++ )
	    {
		const int sz0 = usetf ? tfpacks[idx]->data().info().getSize(0) 
		    : displaypacks[idx]->data().info().getSize(0);
		const int sz1 = usetf ? tfpacks[idx]->data().info().getSize(1)
		    : displaypacks[idx]->data().info().getSize(1);

		if ( idx && (sz0!=newsz0 || sz1!=newsz1) )
		    hassamesz = false;

		if ( newsz0<sz0 ) newsz0 = sz0;
		if ( newsz1<sz1 ) newsz1 = sz1;
	    }
	}

	const bool onlycurrent = newsz0<=oldchannelsz0 && newsz1<=oldchannelsz1;
	const int attribsz = (newsz0<2 || newsz1<2) || (newsz0==oldchannelsz0
		&& newsz1==oldchannelsz1 && hassamesz) ? 0 : nrAttribs();
	if ( newsz0<oldchannelsz0 ) newsz0 = oldchannelsz0;
	if ( newsz1<oldchannelsz1 ) newsz1 = oldchannelsz1;
	for ( int idx=0; idx<attribsz; idx++ )
	{
	    if ( onlycurrent && idx!=attrib )
		continue;

	    TypeSet<DataPack::ID> pids;
	    ObjectSet<const FlatDataPack> packs;
	    if ( idx!=attrib &&  volumecache_[idx] )
	    {
		mLoadFDPs( volumecache_[idx], pids, packs );
	    }

	    bool needsupdate = false;
	    const int idsz = idx==attrib ? attridpids.size() : pids.size();
	    for ( int idy=0; idy<idsz; idy++ )
	    {
		const FlatDataPack* dp = idx!=attrib ? packs[idy] :
		    ( usetf ? tfpacks[idy] : displaypacks[idy] );
		StepInterval<double> rg0 = dp->posData().range(true);
		StepInterval<double> rg1 = dp->posData().range(false);
		const int sz0 = dp->data().info().getSize(0);
		const int sz1 = dp->data().info().getSize(1);
		if ( sz0==newsz0 && sz1==newsz1 )
		    continue;

		needsupdate = true;
		mDeclareAndTryAlloc( Array2DImpl<float>*, arr,
			Array2DImpl<float> (newsz0,newsz1) );
		interpolArray(idx,arr->getData(),newsz0,newsz1,dp->data(),0);
		mDeclareAndTryAlloc( FlatDataPack*, fdp,
			FlatDataPack( dp->category(), arr ) );

		rg0.step = newsz0!=1 ? rg0.width()/(newsz0-1) : rg0.width();
		rg1.step = newsz1!=1 ? rg1.width()/(newsz1-1) : rg1.width(); 
		if ( rg0.step < minx0step_ ) minx0step_ = (float) rg0.step;
		if ( rg1.step < minx1step_ ) minx1step_ = (float) rg1.step;

		fdp->posData().setRange( true, rg0 );
		fdp->posData().setRange( false, rg1 );

		dpman.addAndObtain( fdp );
		if ( idx==attrib )
		    attridpids[idy] = fdp->id();
		else
		    pids[idy] = fdp->id();

		dpman.release( dp );
	    }

	    if ( idx!=attrib )
	    {
		if ( needsupdate )
		    setDisplayDataPackIDs( idx, pids );

		for ( int idy=0; idy<pids.size(); idy++ )
		    dpman.release( pids[idy] );
	    }
	}
    }

    setDisplayDataPackIDs( attrib, attridpids );
    
    for ( int idx=0; idx<attridpids.size(); idx++ )
	dpman.release( attridpids[idx] );
}


DataPack::ID PlaneDataDisplay::getDataPackID( int attrib ) const
{
    return volumecache_.validIdx(attrib) &&  volumecache_[attrib] 
	? volumecache_[attrib]->id() : DataPack::cNoID();
}


void PlaneDataDisplay::setRandomPosDataNoCache( int attrib,
			const BinIDValueSet* bivset, TaskRunner* )
{
    if ( !bivset ) return;

    const CubeSampling cs = getCubeSampling( true, true, 0 );
    TypeSet<DataPack::ID> attridpids;
    for ( int idx=1; idx<bivset->nrVals(); idx++ )
    {
	mDeclareAndTryAlloc( Array2DImpl<float>*, arr,
			Array2DImpl<float> ( cs.hrg.nrInl(), cs.hrg.nrCrl() ) );
	mDeclareAndTryAlloc( FlatDataPack*, fdp,
	    FlatDataPack( Attrib::DataPackCommon::categoryStr(false), arr ) );
        DPM(DataPackMgr::FlatID()).addAndObtain( fdp );
        attridpids += fdp->id();

    	float* texturedataptr = arr->getData();    
    	for ( int idy=0; idy<arr->info().getTotalSz(); idy++ )
    	    (*texturedataptr++) = mUdf(float);
	
    	BinIDValueSet::Pos pos;
    	BinID bid;
    	while ( bivset->next(pos,true) )
    	{
    	    bivset->get( pos, bid );
    	    BinID idxs = (bid-cs.hrg.start)/cs.hrg.step;
    	    arr->set( idxs.inl, idxs.crl, bivset->getVals(pos)[idx]);
    	}
    }

    setDisplayDataPackIDs( attrib, attridpids );

    for ( int idx=0; idx<attridpids.size(); idx++ )
	DPM(DataPackMgr::FlatID()).release( attridpids[idx] );
}


void PlaneDataDisplay::setDisplayDataPackIDs( int attrib,
			const TypeSet<DataPack::ID>& newdpids )
{
    TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).release( dpids[idx] );

    dpids = newdpids;
    for ( int idx=dpids.size()-1; idx>=0; idx-- )
	DPM(DataPackMgr::FlatID()).obtain( dpids[idx] );

    updateFromDisplayIDs( attrib, 0 );
}


void PlaneDataDisplay::updateFromDisplayIDs( int attrib, TaskRunner* tr )
{
    const TypeSet<DataPack::ID>& dpids = *displaycache_[attrib];
    int sz = dpids.size();
    if ( sz<1 )
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, 0 );
	return;
    }

    channels_->setNrVersions( attrib, sz );

    for ( int idx=0; idx<sz; idx++ )
    {
	int dpid = dpids[idx];
	const DataPack* datapack = DPM(DataPackMgr::FlatID()).obtain( dpid );
	mDynamicCastGet( const FlatDataPack*, fdp, datapack );
	if ( !fdp )
	{
	    channels_->turnOn( false );
	    DPM(DataPackMgr::FlatID()).release( dpid );
	    continue;
	}

	const Array2D<float>& dparr = fdp->data();

	const float* arr = dparr.getData();
	OD::PtrPolicy cp = OD::UsePtr;

	int sz0 = dparr.info().getSize(0);
	int sz1 = dparr.info().getSize(1);

	if ( !arr || resolution_>0 )
	{
	    sz0 *= (resolution_+1);
	    sz1 *= (resolution_+1);

	    const od_int64 totalsz = sz0*sz1;
	    mDeclareAndTryAlloc( float*, tmparr, float[totalsz] );

	    if ( !tmparr )
	    {
		DPM(DataPackMgr::FlatID()).release( dpid );
		continue;
	    }

	    if ( resolution_<1 )
		dparr.getAll( tmparr );
	    else
	    	interpolArray( attrib, tmparr, sz0, sz1, dparr, tr );

	    arr = tmparr;
	    cp = OD::TakeOverPtr;
	}

	channels_->setSize( 1, sz0, sz1 );
	channels_->setUnMappedData( attrib, idx, arr, cp, tr );

	rectangle_->setOriginalTextureSize( sz0, sz1 );
	DPM(DataPackMgr::FlatID()).release( dpid );
    }
   
    channels_->turnOn( true );
}


void PlaneDataDisplay::interpolArray( int attrib, float* res, int sz0, int sz1,
			      const Array2D<float>& inp, TaskRunner* tr ) const
{
    Array2DReSampler<float,float> resampler( inp, res, sz0, sz1, true );
    resampler.setInterpolate( textureInterpolationEnabled() );
    if ( tr ) tr->execute( resampler );
    else resampler.execute();
}


const TypeSet<DataPack::ID>* PlaneDataDisplay::getDisplayDataPackIDs(int attrib)
{ 
    return displaycache_.validIdx(attrib) ? displaycache_[attrib] : 0; 
} 


inline int getPow2Sz( int actsz, bool above=true, int minsz=1,
		      int maxsz=INT_MAX )
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz>1 )
    {
	if ( above && !npowextra && sz % 2 )
	npowextra = 1;
	sz /= 2; npow++;
    }

    sz = intpow( 2, npow + npowextra );
    if ( sz < minsz ) sz = minsz;
    if ( sz > maxsz ) sz = maxsz;
    return sz;
}


const Attrib::DataCubes* PlaneDataDisplay::getCacheVolume( int attrib ) const
{
    return attrib<volumecache_.size() && volumecache_[attrib]
	? &volumecache_[attrib]->cube() : 0;
}


#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					Coord3& pos,
					BufferString& val, 
					BufferString& info ) const
{
    info = getManipulationString();
    getValueString( pos, val );
}


void PlaneDataDisplay::getObjectInfo( BufferString& info ) const
{
    if ( orientation_==Inline )
    {
	info = "Inline: ";
	info += getCubeSampling(true,true).hrg.start.inl;
    }
    else if ( orientation_==Crossline )
    {
	info = "Crossline: ";
	info += getCubeSampling(true,true).hrg.start.crl;
    }
    else
    {
	float val = getCubeSampling(true,true).zrg.start;
	if ( !scene_ ) { info = val; return; }

	const ZDomain::Info& zdinf = scene_->zDomainInfo();
	info = zdinf.userName(); info += ": ";
	info += val*zdinf.userFactor();
    }
}


bool PlaneDataDisplay::getCacheValue( int attrib, int version,
				      const Coord3& pos, float& res ) const
{
    if ( !volumecache_.validIdx(attrib) ||
	 (!volumecache_[attrib] && !rposcache_[attrib]) )
	return false;

    const BinIDValue bidv( inlcrlsystem_->transform(pos), (float) pos.z );
    if ( attrib<volumecache_.size() && volumecache_[attrib] )
    {
	const int ver = channels_->currentVersion(attrib);

	const Attrib::DataCubes& vc = volumecache_[attrib]->cube();
	return vc.getValue( ver, bidv, &res, false );
    }
    else if ( attrib<rposcache_.size() && rposcache_[attrib] )
    {
	const BinIDValueSet& set = *rposcache_[attrib];
	const BinIDValueSet::Pos setpos = set.findFirst( bidv.binid );
	if ( setpos.i==-1 || setpos.j==-1 )
	    return false;

	res = set.getVals(setpos)[version+1];
	return true;
    }

    return false;
}


bool PlaneDataDisplay::isVerticalPlane() const
{
    return orientation_ != PlaneDataDisplay::Zslice;
}


void PlaneDataDisplay::setScene( Scene* sc )
{
    SurveyObject::setScene( sc );
    if ( sc ) updateRanges( false, false );
}


BaseMapObject* PlaneDataDisplay::createBaseMapObject()
{ return new PlaneDataDisplayBaseMapObject( this ); }


void PlaneDataDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	eventcatcher_->eventhappened.notify(
		mCB(this,PlaneDataDisplay,updateMouseCursorCB) );
    }
}


void PlaneDataDisplay::updateMouseCursorCB( CallBacker* cb )
{
    char newstatus = 1; // 1= zdrag, 2=pan
    if ( cb )
    {
	mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
	if ( eventinfo.pickedobjids.indexOf(id())==-1 )
	    newstatus = 0;
	else
	{
	    const unsigned int buttonstate =
		(unsigned int) eventinfo.buttonstate_;

	    if ( buttonstate==dragger_->getTransDragKeys(false) )
		newstatus = 2;
	}
    }

    if ( !isManipulatorShown() || !isOn() || isLocked() )
	newstatus = 0;

    if ( !newstatus ) mousecursor_.shape_ = MouseCursor::NotSet;
    else if ( newstatus==1 ) mousecursor_.shape_ = MouseCursor::PointingHand;
    else mousecursor_.shape_ = MouseCursor::SizeAll;
}


SurveyObject* PlaneDataDisplay::duplicate( TaskRunner* tr ) const
{
    PlaneDataDisplay* pdd = create();
    pdd->setOrientation( orientation_ );
    pdd->setCubeSampling( getCubeSampling(false,true,0) );

    while ( nrAttribs() > pdd->nrAttribs() )
	pdd->addAttrib();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( !getSelSpec(idx) ) continue;

	pdd->setSelSpec( idx, *getSelSpec(idx) );
	pdd->setDataPackID( idx, getDataPackID(idx), tr );
	if ( getColTabMapperSetup( idx ) )
	    pdd->setColTabMapperSetup( idx, *getColTabMapperSetup( idx ), tr );
	if ( getColTabSequence( idx ) )
	    pdd->setColTabSequence( idx, *getColTabSequence( idx ), tr );
    }

    return pdd;
}


void PlaneDataDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    MultiTextureSurveyObject::fillPar( par, saveids );

    par.set( sKeyOrientation(), getOrientationString( orientation_) );
    getCubeSampling( false, true ).fillPar( par );

    const int gridlinesid = gridlines_->id();
    par.set( sKeyGridLinesID(), gridlinesid );
    if ( saveids.indexOf(gridlinesid) == -1 ) saveids += gridlinesid;
}


int PlaneDataDisplay::usePar( const IOPar& par )
{
    const int res =  MultiTextureSurveyObject::usePar( par );
    if ( res!=1 ) return res;

    Orientation orientation = Inline;
    FixedString orstr = par.find( sKeyOrientation() );
    if ( !parseEnumOrientation(orstr,orientation) && orstr == "Timeslice" )
	orientation = Zslice;		// Backward compatibilty with 4.0

    setOrientation( orientation );
    CubeSampling cs;
    if ( cs.usePar( par ) )
    {
	csfromsession_ = cs;
	setCubeSampling( cs );
    }

    int gridlinesid;
    if ( par.get(sKeyGridLinesID(),gridlinesid) )
    { 
        DataObject* dataobj = visBase::DM().getObject( gridlinesid );
        if ( !dataobj ) return 0;
        mDynamicCastGet(visBase::GridLines*,gl,dataobj)
        if ( !gl ) return -1;
	removeChild( gridlines_->getInventorNode() );
	gridlines_->unRef();
	gridlines_ = gl;
	gridlines_->ref();
	gridlines_->setPlaneCubeSampling( cs );
	int childidx = childIndex( channels_->getInventorNode() );
	insertChild( childidx, gridlines_->getInventorNode() );
    }

    return 1;
}


} // namespace visSurvey
