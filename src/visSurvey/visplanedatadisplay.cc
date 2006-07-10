/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visplanedatadisplay.cc,v 1.143 2006-07-10 13:25:00 cvskris Exp $";

#include "visplanedatadisplay.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "attribdatacubes.h"
#include "arrayndslice.h"
#include "binidvalset.h"
#include "genericnumer.h"
#include "vistexturerect.h"
#include "survinfo.h"
#include "samplfunc.h"
#include "scaler.h"
#include "visdataman.h"
#include "visrectangle.h"
#include "vismaterial.h"
#include "vistexturecoords.h"
#include "viscolortab.h"
#include "viscoord.h"
#include "visdepthtabplanedragger.h"
#include "visdrawstyle.h"
#include "visgridlines.h"
#include "vispickstyle.h"
#include "visfaceset.h"
#include "vismultitexture2.h"
#include "vistransform.h"
#include "iopar.h"
#include "zaxistransform.h"



mCreateFactoryEntry( visSurvey::PlaneDataDisplay );


namespace visSurvey {

DefineEnumNames(PlaneDataDisplay,Orientation,1,"Orientation")
{ "Inline", "Crossline", "Timeslice", 0 };


PlaneDataDisplay::PlaneDataDisplay()
    : VisualObjectImpl(true)
    , texture_( visBase::MultiTexture2::create() )
    , rectangle_( visBase::FaceSet::create() )
    , rectanglepickstyle_( visBase::PickStyle::create() )
    , dragger_( visBase::DepthTabPlaneDragger::create() )
    , gridlines_( visBase::GridLines::create() )
    , curicstep_(SI().inlStep(),SI().crlStep())
    , curzstep_(SI().zStep())
    , datatransform_( 0 )
    , datatransformvoihandle_( -1 )
    , moving_(this)
    , movefinished_(this)
    , resolution_( 0 )
    , orientation_( Inline )
    , onoffstatus_( true )
{
    volumecache_.allowNull( true );
    rposcache_.allowNull( true );
    dragger_->ref();
    addChild( dragger_->getInventorNode() );
    dragger_->motion.notify( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.notify( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->notify(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    draggerrect_ = visBase::FaceSet::create();
    draggerrect_->ref();
    draggerrect_->removeSwitch();
    draggerrect_->setVertexOrdering(1);
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
    dragger_->setDim( (int) orientation_ );

    rectanglepickstyle_->ref();
    addChild( rectanglepickstyle_->getInventorNode() );

    texture_->ref();
    addChild( texture_->getInventorNode() );
    texture_->setTextureRenderQuality(1);

    rectangle_->ref();
    addChild( rectangle_->getInventorNode() );
    rectangle_->setCoordIndex( 0, 0 );
    rectangle_->setCoordIndex( 1, 1 );
    rectangle_->setCoordIndex( 2, 2 );
    rectangle_->setCoordIndex( 3, 3 );
    rectangle_->setCoordIndex( 4, -1 );
    rectangle_->setVertexOrdering(0);
    rectangle_->setShapeType(0);

    material->setColor( Color::White );
    material->setAmbience( 0.8 );
    material->setDiffIntensity( 0.8 );

    gridlines_->ref();
    addChild( gridlines_->getInventorNode() );

    updateRanges( true, true );

    as_ += new Attrib::SelSpec;
    volumecache_ += 0;
    rposcache_ += 0;
}


PlaneDataDisplay::~PlaneDataDisplay()
{
    dragger_->motion.notify( mCB(this,PlaneDataDisplay,draggerMotion) );
    dragger_->finished.remove( mCB(this,PlaneDataDisplay,draggerFinish) );
    dragger_->rightClicked()->remove(
	    		mCB(this,PlaneDataDisplay,draggerRightClick) );

    deepErase( as_ );
    deepUnRef( volumecache_ );
    deepErase( rposcache_ );

    setDataTransform( 0 );

    texture_->unRef();
    rectangle_->unRef();
    dragger_->unRef();
    rectanglepickstyle_->unRef();
    gridlines_->unRef();
    draggerrect_->unRef();
    draggerdrawstyle_->unRef();
    draggermaterial_->unRef();
}


void PlaneDataDisplay::turnOn( bool yn )
{
    onoffstatus_ = yn;
    updateMainSwitch();
}


void PlaneDataDisplay::updateMainSwitch()
{
    bool newstatus = onoffstatus_;
    if ( newstatus )
    {
	newstatus = false;
	for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	{
	    if ( isAttribEnabled(idx) )
	    {
		newstatus = true;
		break;
	    }
	}
    }

    VisualObjectImpl::turnOn( newstatus );
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
    CubeSampling survey( SI().sampling(true) );
    if ( datatransform_ )
	assign( survey.zrg, datatransform_->getZInterval(false) );
	
    const Interval<float> inlrg( survey.hrg.start.inl, survey.hrg.stop.inl );
    const Interval<float> crlrg( survey.hrg.start.crl, survey.hrg.stop.crl );

    dragger_->setSpaceLimits( inlrg, crlrg, survey.zrg );
    dragger_->setSize( Coord3(inlrg.width(), crlrg.width(),survey.zrg.width()));

    CubeSampling newpos = getCubeSampling(false,true);
    if ( !newpos.isEmpty() )
    {
	if ( !survey.includes( newpos ) )
	    newpos.limitTo( survey );
    }

    if ( !newpos.hrg.isEmpty() && !resetic && resetz )
	survey.hrg = newpos.hrg;

    newpos = snapPosition( resetic || resetz || newpos.isEmpty() 
	    						? survey : newpos );

    if ( newpos!=getCubeSampling(false,true) )
	setCubeSampling( newpos );
}


CubeSampling PlaneDataDisplay::snapPosition( const CubeSampling& cs ) const
{
    CubeSampling res( cs );
    const Interval<float> inlrg( res.hrg.start.inl, res.hrg.stop.inl );
    const Interval<float> crlrg( res.hrg.start.crl, res.hrg.stop.crl );
    const Interval<float> zrg( res.zrg );

    if ( datatransform_ )
	res.hrg.snapToSurvey();
    else
	res.snapToSurvey();

    if ( orientation_==Inline )
	res.hrg.start.inl = res.hrg.stop.inl =
	    SI().inlRange(true).snap( inlrg.center() );
    else if ( orientation_==Crossline )
	res.hrg.start.crl = res.hrg.stop.crl =
	    SI().crlRange(true).snap( crlrg.center() );
    else
	res.zrg.start = res.zrg.stop = SI().zRange(true).snap(zrg.center());

    return res;
}


float PlaneDataDisplay::calcDist( const Coord3& pos ) const
{
    const mVisTrans* utm2display = scene_->getUTM2DisplayTransform();
    const Coord3 xytpos = utm2display->transformBack( pos );
    const BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );

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
    zdiff = cs.zrg.includes(xytpos.z)
	? 0
	: mMIN(xytpos.z-cs.zrg.start,xytpos.z-cs.zrg.stop) *
	  SI().zFactor() * scene_->getZScale();

    const float inldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(1,0)));
    const float crldist =
	SI().transform( BinID(0,0)).distance( SI().transform(BinID(0,1)));
    float inldiff = inlcrldist.inl * inldist;
    float crldiff = inlcrldist.crl * crldist;

    return sqrt( inldiff*inldiff + crldiff*crldiff + zdiff*zdiff );
}


float PlaneDataDisplay::maxDist() const
{
    float maxzdist = SI().zFactor() * scene_->getZScale() * SI().zStep() / 2;
    return orientation_==Timeslice ? maxzdist : SurveyObject::sDefMaxDist;
}


bool PlaneDataDisplay::setDataTransform( ZAxisTransform* zat )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransformvoihandle_!=-1 )
	    datatransform_->removeVolumeOfInterest(datatransformvoihandle_);
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this, PlaneDataDisplay, dataTransformCB ));
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    datatransformvoihandle_ = -1;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this, PlaneDataDisplay, dataTransformCB ));
    }

    return true;
}


const ZAxisTransform* PlaneDataDisplay::getDataTransform() const
{ return datatransform_; }


void PlaneDataDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );

    for ( int idx=0; idx<volumecache_.size(); idx++ )
    {
	if ( volumecache_[idx] )
	    setData( idx, volumecache_[idx] );
	else if ( rposcache_[idx] )
	{
	    ObjectSet<BinIDValueSet> set;
	    set += rposcache_[idx];
	    setData( idx, &set );
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
    else if ( orientation_==Timeslice && dragcs.zrg.start!=oldcs.zrg.start )
	showplane = true;
   
    draggerdrawstyle_->setDrawStyle( showplane
	    ? visBase::DrawStyle::Filled
	    : visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( showplane ? 0.5 : 0 );
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


void PlaneDataDisplay::setDraggerPos( const CubeSampling& cs )
{
    const Coord3 center( (cs.hrg.start.inl+cs.hrg.stop.inl)/2.0,
		         (cs.hrg.start.crl+cs.hrg.stop.crl)/2.0,
		         cs.zrg.center() );
    Coord3 width( cs.hrg.stop.inl-cs.hrg.start.inl,
		  cs.hrg.stop.crl-cs.hrg.start.crl, cs.zrg.width() );

    const Coord3 oldwidth = dragger_->size();
    width[(int)orientation_] = oldwidth[(int)orientation_];

    dragger_->setCenter( center );
    dragger_->setSize( width );
    movefinished_.trigger();
}


void PlaneDataDisplay::coltabChanged( CallBacker* )
{
    // Hack for correct transparency display
    bool manipshown = isManipulatorShown();
    if ( manipshown ) return;
    showManipulator( true );
    showManipulator( false );
}

/*
SurveyObject* PlaneDataDisplay::duplicate() const
{
    PlaneDataDisplay* pdd = create();
    pdd->setOrientation( orientation_ );
    pdd->setCubeSampling( getCubeSampling() );
    pdd->setResolution( getResolution() );

    int ctid = pdd->getColTabID();
    visBase::DataObject* obj = ctid>=0 ? visBase::DM().getObject( ctid ) : 0;
    mDynamicCastGet(visBase::VisColorTab*,nct,obj);
    if ( nct )
    {
	const char* ctnm = texture_->getColorTab().colorSeq().colors().name();
	nct->colorSeq().loadFromStorage( ctnm );
    }
    return pdd;
}
*/


void PlaneDataDisplay::showManipulator( bool yn )
{
    dragger_->turnOn( yn );
    rectanglepickstyle_->setStyle( yn ? visBase::PickStyle::Unpickable
				      : visBase::PickStyle::Shape );
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
}


void PlaneDataDisplay::acceptManipulation()
{
    CubeSampling cs = getCubeSampling( true, true );
    setCubeSampling( cs );
    draggerdrawstyle_->setDrawStyle( visBase::DrawStyle::Lines );
    draggermaterial_->setTransparency( 0 );
    if ( gridlines_ ) gridlines_->setPlaneCubeSampling( cs );
}


BufferString PlaneDataDisplay::getManipulationString() const
{
    BufferString res;
    if ( orientation_==Inline )
    {
	res = "Inline: ";
	res += getCubeSampling(true,true).hrg.start.inl;
    }
    else if ( orientation_==Crossline )
    {
	res = "Crossline: ";
	res += getCubeSampling(true,true).hrg.start.crl;
    }
    else
    {
	res = SI().zIsTime() ? "Time: " : "Depth: ";
	float val = getCubeSampling(true,true).zrg.start;
	res += SI().zIsTime() ? mNINT(val * 1000) : val;
    }

    return res;
}


NotifierAccess* PlaneDataDisplay::getManipulationNotifier()
{ return &moving_; }


int PlaneDataDisplay::nrResolutions() const
{ return 3; }


int PlaneDataDisplay::getResolution() const
{ return resolution_; }


void PlaneDataDisplay::setResolution( int res )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    texture_->clearAll();

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( volumecache_[idx] )
	    setData( idx, volumecache_[idx] );
	else if ( rposcache_[idx] )
	{
	    ObjectSet<BinIDValueSet> set;
	    set += rposcache_[idx];
	    setData( idx, &set );
	}
    }
}


SurveyObject::AttribFormat PlaneDataDisplay::getAttributeFormat() const
{
    return datatransform_ && orientation_==Timeslice
	? SurveyObject::RandomPos
	: SurveyObject::Cube;
}


bool PlaneDataDisplay::canHaveMultipleAttribs() const
{ return true; }


int PlaneDataDisplay::nrAttribs() const
{ return as_.size(); }


bool PlaneDataDisplay::addAttrib()
{
    as_ += new Attrib::SelSpec;
    volumecache_ += 0;
    rposcache_ += 0;

    texture_->addTexture("");
    texture_->setOperation( as_.size()-1, visBase::MultiTexture::BLEND );

    updateMainSwitch();
    return true;
}


bool PlaneDataDisplay::removeAttrib( int attrib )
{
    if ( as_.size()<2 || attrib<0 || attrib>=as_.size() )
	return false;

    delete as_[attrib];
    as_.remove( attrib );
    if ( volumecache_[attrib] ) volumecache_[attrib]->unRef();
    volumecache_.remove( attrib );
    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.remove( attrib );

    texture_->removeTexture( attrib );

    updateMainSwitch();
    return true;
}


bool PlaneDataDisplay::swapAttribs( int a0, int a1 )
{
    if ( a0<0 || a1<0 || a0>=as_.size() || a1>=as_.size() )
	return false;

    texture_->swapTextures( a0, a1 );
    as_.swap( a0, a1 );
    volumecache_.swap( a0, a1 );
    rposcache_.swap( a0, a1 );

    return true;
}


void PlaneDataDisplay::setAttribTransparency( int attrib, unsigned char nt )
{
    texture_->setTextureTransparency( attrib, nt );
}


unsigned char PlaneDataDisplay::getAttribTransparency( int attrib ) const
{
    return texture_->getTextureTransparency( attrib );
}


const Attrib::SelSpec* PlaneDataDisplay::getSelSpec( int attrib ) const
{
    return attrib>=0 && attrib<as_.size() ? as_[attrib] : 0;
}


void PlaneDataDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( attrib>=0 && attrib<as_.size() )
    {
	*as_[attrib] = as;
    }

    if ( volumecache_[attrib] ) volumecache_[attrib]->unRef();
    volumecache_.replace( attrib, 0 );
    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.replace( attrib, 0 );

    const char* usrref = as.userRef();
    if ( !usrref || !*usrref )
	texture_->turnOn( false );
}


bool PlaneDataDisplay::isClassification( int attrib ) const
{
    return attrib>=0 && attrib<isclassification_.size()
	? isclassification_[attrib] : false;
}


void PlaneDataDisplay::setClassification( int attrib, bool yn )
{
    if ( attrib<0 || attrib>=as_.size() )
	return;

    if ( yn )
    {
	while ( attrib>=isclassification_.size() )
	    isclassification_ += false;
    }
    else if ( attrib>=isclassification_.size() )
	return;

    isclassification_[attrib] = yn;
}


bool PlaneDataDisplay::isAttribEnabled( int attrib ) const 
{
    return texture_->isTextureEnabled( attrib );
}


void PlaneDataDisplay::enableAttrib( int attrib, bool yn )
{
    texture_->enableTexture( attrib, yn );
    updateMainSwitch();
}


const TypeSet<float>* PlaneDataDisplay::getHistogram( int attrib ) const
{
    return texture_->getHistogram( attrib, texture_->currentVersion( attrib ) );
}


int PlaneDataDisplay::getColTabID( int attrib ) const
{
    return texture_->getColorTab( attrib ).id();
}


CubeSampling PlaneDataDisplay::getCubeSampling( int attrib ) const
{
    return getCubeSampling( true, false, attrib );
}


void PlaneDataDisplay::getRandomPos( ObjectSet<BinIDValueSet>& pos ) const
{
    if ( !datatransform_->loadDataIfMissing( datatransformvoihandle_ ) )
	return;

    const CubeSampling cs = getCubeSampling( true, true, 0 ); //attrib?
    HorSamplingIterator iter( cs.hrg );

    BinIDValueSet* res = new BinIDValueSet( 1, false );
    pos += res;
    BinIDValue curpos;
    curpos.value = cs.zrg.start;
    while ( iter.next(curpos.binid) )
    {
	const float depth = datatransform_->transformBack( curpos );
	if ( mIsUdf(depth) )
	    continue;

	res->add( curpos.binid, depth );
    }
}


void PlaneDataDisplay::setRandomPosData( int attrib,
					 const ObjectSet<BinIDValueSet>* data )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return;

    setData( attrib, data );

    if ( rposcache_[attrib] ) delete rposcache_[attrib];
    rposcache_.replace( attrib, data && data->size()
	    ? new BinIDValueSet(*(*data)[0]) : 0 );
}


void PlaneDataDisplay::setCubeSampling( CubeSampling cs )
{
    cs = snapPosition( cs );
    const HorSampling& hrg = cs.hrg;

    visBase::Coordinates* coords = rectangle_->getCoordinates();
    if ( orientation_==Inline || orientation_==Crossline )
    {
	coords->setPos( 0, Coord3(hrg.start.inl,hrg.start.crl,cs.zrg.start) );
	coords->setPos( 1, Coord3(hrg.start.inl,hrg.start.crl,cs.zrg.stop) );
	coords->setPos( 2, Coord3(hrg.stop.inl,hrg.stop.crl,cs.zrg.stop) );
	coords->setPos( 3, Coord3(hrg.stop.inl,hrg.stop.crl, cs.zrg.start) );
    }
    else 
    {
	coords->setPos( 0, Coord3(hrg.start.inl,hrg.start.crl,cs.zrg.start) );
	coords->setPos( 1, Coord3(hrg.start.inl,hrg.stop.crl,cs.zrg.stop) );
	coords->setPos( 2, Coord3(hrg.stop.inl,hrg.stop.crl,cs.zrg.stop) );
	coords->setPos( 3, Coord3(hrg.stop.inl,hrg.start.crl,cs.zrg.start) );
    }

    setDraggerPos( cs );

    curicstep_ = hrg.step;
    curzstep_ = cs.zrg.step;

    texture_->clearAll();
    movefinished_.trigger();

    if ( !datatransform_ )
	return;

    if ( datatransformvoihandle_==-1 )
	datatransformvoihandle_ =
	    datatransform_->addVolumeOfInterest( cs, true );
    else
	datatransform_->setVolumeOfInterest( datatransformvoihandle_,
					     cs, true );
}


CubeSampling PlaneDataDisplay::getCubeSampling( bool manippos,
						bool displayspace,
       						int attrib ) const
{
    CubeSampling res(false);
    if ( manippos || rectangle_->getCoordinates()->size()>=4 )
    {
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
	    c0 = rectangle_->getCoordinates()->getPos(0);
	    c1 = rectangle_->getCoordinates()->getPos(2);
	}

	res.hrg.start = res.hrg.stop = BinID(mNINT(c0.x),mNINT(c0.y) );
	res.zrg.start = res.zrg.stop = c0.z;
	res.hrg.include( BinID(mNINT(c1.x),mNINT(c1.y)) );
	res.zrg.include( c1.z );
	res.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
	res.zrg.step = SI().zRange(true).step;

	const char* depthdomain = attrib>=0 && attrib<as_.size() 
	    				? as_[attrib]->depthDomainKey() : 0;
	const bool alreadytransformed = depthdomain && *depthdomain;
	if ( alreadytransformed ) return res;

	if ( datatransform_ && !displayspace )
	{
	    assign( res.zrg, datatransform_->getZInterval(true) );
	    res.zrg.step = SI().zRange( true ).step;
	}
    }
    return res;
}


bool PlaneDataDisplay::setDataVolume( int attrib,
				      const Attrib::DataCubes* datacubes )
{
    if ( attrib<0 || attrib>=nrAttribs() )
	return false;

    setData( attrib, datacubes );

    if ( volumecache_[attrib] ) volumecache_[attrib]->unRef();
    volumecache_.replace( attrib, datacubes );
    datacubes->ref();
    return true;
}


void PlaneDataDisplay::setData( int attrib, const Attrib::DataCubes* datacubes )
{
    if ( !datacubes )
    {
	texture_->setData( attrib, 0, 0 );
	texture_->turnOn( false );
	return;
    }

    //Do subselection of input if input is too big

    int unuseddim, dim0, dim1;
    if ( orientation_==Inline )
    {
	unuseddim = Attrib::DataCubes::cInlDim();
	dim0 = Attrib::DataCubes::cZDim();
	dim1 = Attrib::DataCubes::cCrlDim();
    }
    else if ( orientation_==Crossline )
    {
	unuseddim = Attrib::DataCubes::cCrlDim();
	dim0 = Attrib::DataCubes::cZDim();
	dim1 = Attrib::DataCubes::cInlDim();
    }
    else
    {
	unuseddim = Attrib::DataCubes::cZDim();
	dim0 = Attrib::DataCubes::cCrlDim();
	dim1 = Attrib::DataCubes::cInlDim();
    }

    const char* depthdomain = as_[attrib]->depthDomainKey();
    const bool alreadytransformed = depthdomain && *depthdomain;

    const int nrcubes = datacubes->nrCubes();
    texture_->setNrVersions( attrib, nrcubes );
    for ( int idx=0; idx<nrcubes; idx++ )
    {
	PtrMan<Array3D<float> > tmparray = 0;
	const Array3D<float>* usedarray = 0;
	if ( alreadytransformed || !datatransform_ )
	    usedarray = &datacubes->getCube(idx);
	else
	{
	    const CubeSampling cs = getCubeSampling(true,true);
	    datatransform_->loadDataIfMissing( datatransformvoihandle_ );

	    ZAxisTransformSampler outpsampler( *datatransform_, true,BinID(0,0),
		    	SamplingData<double>(cs.zrg.start, cs.zrg.step));
	    const Array3D<float>& srcarray = datacubes->getCube( idx );
	    const Array3DInfo& info = srcarray.info();
	    const int inlsz = info.getSize( Attrib::DataCubes::cInlDim() );
	    const int crlsz = info.getSize( Attrib::DataCubes::cCrlDim() );
	    const int zsz = cs.zrg.nrSteps()+1;
	    tmparray = new Array3DImpl<float>( inlsz, crlsz, zsz );
	    usedarray = tmparray;

	    for ( int inlidx=0; inlidx<inlsz; inlidx++ )
	    {
		for ( int crlidx=0; crlidx<crlsz; crlidx++ )
		{
		    const BinID bid( datacubes->inlsampling.atIndex(inlidx),
			   	     datacubes->crlsampling.atIndex(crlidx) );
		    outpsampler.setBinID( bid );
		    outpsampler.computeCache( Interval<int>(0,zsz-1) );

		    const float* inputptr = srcarray.getData() +
					    info.getMemPos(inlidx,crlidx,0);
		    float* outputptr = tmparray->getData() +
				    tmparray->info().getMemPos(inlidx,crlidx,0);

		    SampledFunctionImpl<float,const float*>
			inputfunc( inputptr,
				   info.getSize(Attrib::DataCubes::cZDim()),
				   datacubes->z0*datacubes->zstep,
				   datacubes->zstep );
		    inputfunc.setHasUdfs( true );
		    inputfunc.setInterpolate( !isClassification(attrib) );

		    reSample( inputfunc, outpsampler, outputptr, zsz );
		}
	    }
	}

	Array2DSlice<float> slice(*usedarray);
	slice.setPos( unuseddim, 0 );
	slice.setDimMap( 0, dim0 );
	slice.setDimMap( 1, dim1 );

	if ( slice.init() )
	{
	    if ( resolution_ )
		texture_->setDataOversample( attrib, idx, resolution_, 
					     !isClassification( attrib ),
		       			     &slice, true );
	    else texture_->setData( attrib, idx, &slice, true );

	    if ( !idx )
	    {
		setTextureCoords( slice.info().getSize(0),
				  slice.info().getSize(1) );
	    }
	}
	else
	{
	    texture_->turnOn(false);
	    pErrMsg( "Could not init slice." );
	}
    }

    texture_->turnOn( true );
}


void PlaneDataDisplay::setData( int attrib,
				const ObjectSet<BinIDValueSet>* data )
{
    if ( !data || data->size()<1 || (*data)[0]->nrVals()<2 )
    {
	texture_->setData( attrib, 0, 0 );
	texture_->turnOn( false );
	return;
    }

    const CubeSampling cs = getCubeSampling( true, true, 0 ); //attrib?
    const BinIDValueSet& set( *(*data)[0] );
    Array2DImpl<float> texturedata( cs.hrg.nrCrl(), cs.hrg.nrInl() );

    texture_->setNrVersions( attrib, set.nrVals()-1 );
    for ( int idx=1; idx<set.nrVals(); idx++ )
    {
	const int nrvals = texturedata.info().getTotalSz();
	
	float* texturedataptr = texturedata.getData();
	for ( int idy=0; idy<nrvals; idy++ )
	    (*texturedataptr++) = mUdf(float);

	BinIDValueSet::Pos pos;
	BinID bid;
	while ( set.next(pos,true) )
	{
	    set.get( pos, bid );
	    BinID idxs = (bid-cs.hrg.start)/cs.hrg.step;
	    texturedata.set( idxs.crl, idxs.inl, set.getVals(pos)[idx]);
	}

	texture_->setData( attrib, idx-1, &texturedata, true );
	if ( resolution_ )
	    texture_->setDataOversample( attrib, idx-1, resolution_, 
					 !isClassification( attrib ),
					 &texturedata, true );
	else
	    texture_->setData( attrib, idx-1, &texturedata, true );
    }

    setTextureCoords( texturedata.info().getSize(1),
	    	      texturedata.info().getSize(0) );
    texture_->turnOn( true );
}


void PlaneDataDisplay::setTextureCoords( int sz0, int sz1 )
{
    visBase::TextureCoords* tcoords = rectangle_->getTextureCoords();
    if ( !tcoords )
    {
	tcoords = visBase::TextureCoords::create();
	rectangle_->setTextureCoords( tcoords );
    }

    const LinScaler dim0scale( -0.5, 0, sz0-0.5, 1);
    const LinScaler dim1scale( -0.5, 0, sz1-0.5, 1);
    const Interval<float> dim0rg( dim0scale.scale(0), dim0scale.scale(sz0-1) );
    const Interval<float> dim1rg( dim1scale.scale(0), dim1scale.scale(sz1-1) );

    tcoords->setCoord( 0, Coord3( dim1rg.start, dim0rg.start, 0 ) );
    tcoords->setCoord( 1, Coord3( dim1rg.start, dim0rg.stop, 0 ) );
    tcoords->setCoord( 2, Coord3( dim1rg.stop, dim0rg.stop, 0 ) );
    tcoords->setCoord( 3, Coord3( dim1rg.stop, dim0rg.start, 0 ) );
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
    return attrib>=0 && attrib<nrAttribs() ? volumecache_[attrib] : 0;
}


int PlaneDataDisplay::nrTextures( int attrib ) const
{
    return getCacheVolume( attrib ) ? getCacheVolume( attrib )->nrCubes() : 0;
}


void PlaneDataDisplay::selectTexture( int attrib, int idx )
{
    if ( attrib<0 || attrib>=nrAttribs() ||
	 idx<0 || idx>=texture_->nrVersions(attrib) ) return;

    texture_->setCurrentVersion( attrib, idx );
}


int PlaneDataDisplay::selectedTexture( int attrib ) const
{ 
    if ( attrib<0 || attrib>=nrAttribs() ) return 0;

    return texture_->currentVersion( attrib );
}

#define mIsValid(idx,sz) ( idx>=0 && idx<sz )

void PlaneDataDisplay::getMousePosInfo( const visBase::EventInfo&,
					const Coord3& pos,
					BufferString& val, 
					BufferString& info ) const
{
    info = getManipulationString();
    val = "undef";
    BufferString valname;

    const BinIDValue bidv( SI().transform(pos), pos.z );
    for ( int idx=as_.size()-1; idx>=0; idx-- )
    {
	if ( !isAttribEnabled(idx) ||
		texture_->getTextureTransparency(idx)==255 )
	    continue;

	const int version = texture_->currentVersion(idx);
	float fval = mUdf(float);

	if ( idx<volumecache_.size() && volumecache_[idx] )
	{
	    const Attrib::DataCubes* vc = volumecache_[idx];

	    if ( !vc->getValue(version,bidv,&fval,!isClassification(idx)) )
		continue;
	}

	if ( idx<rposcache_.size() && rposcache_[idx] )
	{
	    const BinIDValueSet& set = *rposcache_[idx];
	    const BinIDValueSet::Pos setpos = set.findFirst( bidv.binid );
	    if ( setpos.i==-1 || setpos.j==-1 )
		continue;

	    fval = set.getVals(setpos)[version+1];
	}

	if ( idx )
	{
	    const Color col = texture_->getColorTab(idx).color(fval);
	    if ( col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	{
	    val = fval;
	    if ( volumecache_.size()>1 )
	    {
		BufferString attribstr = "(";
		attribstr += as_[idx]->userRef();
		attribstr += ")";
		val.insertAt( cValNameOffset(), (const char*)attribstr);
	    }

	    return;
	}

    }

    return;
}


void PlaneDataDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );

    par.setYN( visBase::VisualObjectImpl::sKeyIsOn(), isOn() );
    par.set( sKeyOrientation(), OrientationRef(orientation_) );
    par.set( sKeyResolution(), resolution_ );
    getCubeSampling( false, true ).fillPar( par );

    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );
	const int coltabid = getColTabID(attrib);
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 texture_->isTextureEnabled(attrib) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
}


int PlaneDataDisplay::usePar( const IOPar& par )
{
    const int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    int nrattribs;
    if ( par.get(sKeyNrAttribs(),nrattribs) ) //current format
    {
	Orientation ori;
	EnumRef oriref = OrientationRef(ori);
	if ( par.get( sKeyOrientation(), oriref ) )
	    setOrientation( ori );

	par.get( sKeyResolution(), resolution_ );

	bool ison = true;
	par.getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	turnOn( ison );

	CubeSampling cs;
	if ( cs.usePar( par ) )
	    setCubeSampling( cs );

	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    int coltabid = -1;
	    if ( attribpar->get(sKeyColTabID(),coltabid) )
	    {
		visBase::DataObject* dataobj= visBase::DM().getObject(coltabid);
		if ( !dataobj ) return 0;

		mDynamicCastGet(const visBase::VisColorTab*,coltab,dataobj);
		if ( !coltab ) coltabid=-1;
	    }

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    as_[attribnr]->usePar( *attribpar );
	    if ( coltabid!=-1 )
	    {
		mDynamicCastGet( visBase::VisColorTab*, coltab, 
		       		 visBase::DM().getObject(coltabid) );
		texture_->setColorTab( attribnr, *coltab );
	    }

	    ison = true;
	    attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	    texture_->enableTexture( attribnr, ison );
	}
    }
    else
    {
	int trectid;

	if ( !par.get( sKeyTextureRect(), trectid ))
	    return -1;

	visBase::DataObject* dataobj = visBase::DM().getObject( trectid );
	if ( !dataobj ) return 0;

	mDynamicCastGet(visBase::TextureRect*,tr,dataobj)
	if ( !tr ) return -1;

	tr->ref();


	visBase::Rectangle& rect = tr->getRectangle();
	CubeSampling cubesampl;
	Orientation ori;
	cubesampl.hrg.start =
	    BinID( mNINT( rect.origo().x), mNINT( rect.origo().y) );
	cubesampl.hrg.stop = cubesampl.hrg.start;
	cubesampl.hrg.step = curicstep_;

	const float zrg0 = rect.origo().z;
	cubesampl.zrg.start = (float)(int)(1000*zrg0+.5) / 1000;
	cubesampl.zrg.stop = cubesampl.zrg.start;
	cubesampl.zrg.step = curzstep_;

	if ( rect.orientation()==visBase::Rectangle::XY )
	{
	    cubesampl.hrg.stop.inl += mNINT(rect.width(0,false));
	    cubesampl.hrg.stop.crl += mNINT(rect.width(1,false));
	    ori = Timeslice;
	}
	else if ( rect.orientation()==visBase::Rectangle::XZ )
	{
	    cubesampl.hrg.stop.inl += mNINT(rect.width(0,false));
	    cubesampl.zrg.stop += rect.width(1,false);
	    ori = Crossline;
	}
	else
	{
	    ori = Inline;
	    cubesampl.hrg.stop.crl += mNINT(rect.width(1,false));
	    cubesampl.zrg.stop += rect.width(0,false);
	}

	setOrientation( ori );
	setCubeSampling( cubesampl );
	resolution_ = tr->getResolution();

	if ( resolution_>=2 ) resolution_ = 2;
	else if ( resolution_<0 ) resolution_ = 0;
	
	tr->unRef();

	as_[0]->usePar( par );
    }

    return 1;
}


}; // namespace visSurvey
