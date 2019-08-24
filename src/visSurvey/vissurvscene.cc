/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "vissurvscene.h"

#include "binidvalue.h"
#include "trckeyzsampling.h"
#include "envvars.h"
#include "fontdata.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "trckey.h"
#include "uistring.h"
#include "visannot.h"
#include "viscamera.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "visselman.h"
#include "vistext.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vissurvobj.h"
#include "visvolumedisplay.h"
#include "vistopbotimage.h"
#include "zaxistransform.h"
#include "zdomain.h"
#include "uistrings.h"


mCreateFactoryEntry( visSurvey::Scene )

namespace visSurvey {

const char* Scene::sKeyShowAnnot()	{ return "Show text"; }
const char* Scene::sKeyShowScale()	{ return "Show scale"; }
const char* Scene::sKeyShowGrid()	{ return "Show grid"; }
const char* Scene::sKeyAnnotFont()	{ return "Annotation font"; }
const char* Scene::sKeyAnnotColor()	{ return "Annotation color"; }
const char* Scene::sKeyMarkerColor()	{ return "Marker color"; }
const char* Scene::sKeyShowCube()	{ return "Show cube"; }
const char* Scene::sKeyZStretch()	{ return "Z Stretch"; }
const char* Scene::sKeyZAxisTransform()	{ return "ZTransform"; }
const char* Scene::sKeyTopImageID()	{ return "TopImage.ID"; }
const char* Scene::sKeyBotImageID()	{ return "BotImage.ID"; }

static const char* sKeydTectScene()	{ return "dTect.Scene"; }
static const char* sKeyShowColTab()	{ return "Show ColTab"; }

static const char* sKeyChildID()	{ return "ID"; }
static const char* sKeyNrChild()	{ return "Number of Child"; }
static const char* childfix()		{ return "Child"; }
static const char* sKeyTopImage()	{ return "TopImage"; }
static const char* sKeyBottomImage()	{ return "BottomImage"; }
static const char* sKeyZScale()		{ return "Z Scale"; }

Scene::Scene()
    : tempzstretchtrans_(0)
    , annot_(0)
    , markerset_(0)
    , mouseposchange(this)
    , mousecursorchange(this)
    , keypressed(this)
    , mouseclicked(this)
    , sceneboundingboxupdated(this)
    , mouseposval_(0)
    , mouseposstr_("")
    , curzstretch_( 2 )
    , datatransform_( 0 )
    , mousecursor_( 0 )
    , polyselector_( 0 )
    , coordselector_( 0 )
    , zscale_( SI().zScale() )
    , infopar_(*new IOPar)
    , zdomaininfo_(new ZDomain::Info(ZDomain::SI()))
    , ctshownusepar_( false )
    , usepar_( false )
    , scenecoltab_(0)
    , topimg_( 0 )
    , botimg_( 0 )
    , posmodemanipdeselobjid_( -1 )
    , spacebarwaspressed_( false )
    , moreobjectstodo_( false )
{
    mAttachCB( events_.eventhappened, Scene::mouseCB );
    mAttachCB( events_.eventhappened, Scene::mouseCursorCB );
    mAttachCB( events_.eventhappened, Scene::keyPressCB );
    mAttachCB( events_.nothandled, Scene::mouseCursorCB );
    mAttachCB( visBase::DM().selMan().selnotifier, Scene::selChangeCB );

    setCameraAmbientLight( 1 );
    setup();
}


void Scene::setEventHandled()
{ events_.setHandled(); }


void Scene::updateAnnotationText()
{
    if ( !annot_ )
	return;

    if ( SI().inlRange(OD::UsrWork).width() )
	annot_->setText( 0, uiStrings::sInline() );

    if ( SI().crlRange(OD::UsrWork).width() )
	annot_->setText( 1, uiStrings::sCrossline() );

    if ( SI().zRange(OD::UsrWork).width() )
	annot_->setText( 2, zDomainUserName() );

    annot_->setScaleFactor( 2,
	    zdomaininfo_ ? zdomaininfo_->userFactor() : 1 );
}


void Scene::setup()
{
    annot_ = visBase::Annotation::create();
    annot_->setScene( this );

    const Settings& setts = Settings::common();
    BufferString font;
    const char* key = FontData::key( FontData::Graphics3D );
    setts.get( IOPar::compKey("Font.def",key), font );
    if ( font.isEmpty() )
	setts.get( IOPar::compKey(sKeydTectScene(),sKeyAnnotFont()), font );

    FontData fd;
    if ( !font.isEmpty() )
	fd.getFrom( font.buf() );
    annot_->setFont( fd );
    annot_->allowShading( SettingsAccess().doesUserWantShading(false) );

   if ( !SI().getDefaultPars().get(
	IOPar::compKey(sKeyZScale(), zDomainInfo().key()), curzstretch_ ) )
    {
	if ( !SI().getDefaultPars().get(sKeyZStretch(),curzstretch_) )
	    SI().getDefaultPars().get( sKeyZScale(), curzstretch_ );
    }

    const TrcKeyZSampling tkzs( OD::UsrWork );
    updateTransforms( tkzs );
    setTrcKeyZSampling( tkzs );

    addInlCrlZObject( annot_ );
    updateAnnotationText();

#define mGetProp(type,var,defval,get,str,func) \
    type var = defval; \
    setts.get( IOPar::compKey(sKeydTectScene(),str), var ); \
    func( var );

    mGetProp( bool, showan, true, getYN, sKeyShowAnnot(), showAnnotText );
    mGetProp( bool, showsc, true, getYN, sKeyShowScale(), showAnnotScale );
    mGetProp( bool, showgr, true, getYN, sKeyShowGrid(), showAnnotGrid );

    mGetProp( Color, mcol, getMarkerColor(), get,
	      sKeyMarkerColor(), setMarkerColor );
    mGetProp( Color, acol, getAnnotColor(), get,
	      sKeyAnnotColor(), setAnnotColor );
}



#define mRemoveSelector \
unRefAndZeroPtr( polyselector_ ); \
deleteAndZeroPtr( coordselector_ )

Scene::~Scene()
{
    detachAllNotifiers();

    if ( datatransform_ ) datatransform_->unRef();

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx));
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( vo ) vo->setSceneEventCatcher( 0 );
	if ( so ) so->setScene( 0 );
    }

    mRemoveSelector;
    delete zdomaininfo_;
}


#define mZFactor(stretch) (zscale_*stretch / 2)


void Scene::updateTransforms( const TrcKeyZSampling& cs )
{
    if ( !tempzstretchtrans_ )
    {
	tempzstretchtrans_ = mVisTrans::create();
	visBase::DataObjectGroup::addObject( tempzstretchtrans_ );
    }

    RefMan<mVisTrans> newinlcrlrotation = mVisTrans::create();
    RefMan<mVisTrans> newinlcrlscale = mVisTrans::create();

    const float zfactor = -1 * mZFactor(curzstretch_);
    // -1 to compensate for that we want z to increase with depth

    SceneTransformManager::computeICRotationTransform(
	    *SI().get3DGeometry(OD::UsrWork), zfactor, cs.zsamp_.center(),
	    newinlcrlrotation, newinlcrlscale );

    tempzstretchtrans_->addObject( newinlcrlrotation );

    RefMan<mVisTrans> oldinlcrlrotation = inlcrlrotation_;
    inlcrlrotation_ = newinlcrlrotation;
    inlcrlscale_ = newinlcrlscale;

    if ( oldinlcrlrotation )
    {
	tempzstretchtrans_->removeObject(
			tempzstretchtrans_->getFirstIdx( oldinlcrlrotation ));

	for ( int idx=0; idx<oldinlcrlrotation->size(); idx++ )
	{
	    RefMan<visBase::DataObject> dobj =
		oldinlcrlrotation->getObject(idx);
	    inlcrlrotation_->addObject( dobj );
	    dobj->setDisplayTransformation( inlcrlscale_ );
	}

	oldinlcrlrotation->removeAll();
    }

    RefMan<mVisTrans> newutm2disptransform = mVisTrans::create();
    SceneTransformManager::computeUTM2DisplayTransform(
		    *SI().get3DGeometry(OD::UsrWork), zfactor,
		    cs.zsamp_.center(), newutm2disptransform );

    if ( utm2disptransform_ )
    {
	for ( int idx=0; idx<tempzstretchtrans_->size(); idx++ )
	{
	    visBase::DataObject* dobj = tempzstretchtrans_->getObject( idx );
	    if ( dobj->getDisplayTransformation()==utm2disptransform_ )
	    {
		dobj->setDisplayTransformation( newutm2disptransform );
	    }
	}
    }

    utm2disptransform_ = newutm2disptransform;

    ObjectSet<visBase::Transformation> utm2display;
    utm2display += utm2disptransform_;
    utm2display += tempzstretchtrans_;
    events_.setUtm2Display( utm2display );
}


bool Scene::isRightHandSystem() const
{
    return SI().isRightHandSystem();
}


const ZDomain::Info& Scene::zDomainInfo() const
{ return *zdomaininfo_; }


void Scene::setZDomainInfo( const ZDomain::Info& zdinf )
{
    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zdinf );
    updateAnnotationText();
}


const char* Scene::zDomainKey() const
{ return zdomaininfo_->key(); }

uiString Scene::zDomainUserName() const
{ return toUiString(zdomaininfo_->userName()); }

const char* Scene::zDomainUnitStr( bool withparens ) const
{ return zdomaininfo_->fileUnitStr( withparens ); }

int Scene::zDomainUserFactor() const
{ return zdomaininfo_->userFactor(); }

const char* Scene::zDomainID() const
{ return zdomaininfo_->getID(); }


void Scene::getAllowedZDomains( BufferString& dms ) const
{
    FileMultiString fms( zDomainKey() );
    if ( datatransform_ )
	fms.add( datatransform_->toZDomainKey() );
    dms = fms.buf();
}


void Scene::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    tkzs_ = cs;
    if ( !annot_ ) return;

    annot_->setTrcKeyZSampling( cs );
}


void Scene::setAnnotScale( const TrcKeyZSampling& cs )
{
    annotscale_ = cs;
    if ( !annot_ ) return;

    annot_->setScale( cs );
}


const TrcKeyZSampling& Scene::getAnnotScale() const
{ return annot_ ? annot_->getScale() : annotscale_; }


int Scene::size() const
{
    return tempzstretchtrans_->size()+inlcrlrotation_->size();
}


int Scene::getFirstIdx( const visBase::DataObject* dobj ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( getObject( idx )==dobj )
	    return idx;
    }

    return -1;
}


visBase::DataObject* Scene::getObject( int idx )
{
    if ( idx<tempzstretchtrans_->size() )
	return tempzstretchtrans_->getObject( idx );

    return inlcrlrotation_->getObject( idx-tempzstretchtrans_->size() );
}


const visBase::DataObject* Scene::getObject( int idx ) const
{
    return const_cast<Scene*>( this )->getObject( idx );
}


void Scene::addUTMObject( visBase::VisualObject* obj )
{
    obj->setDisplayTransformation( utm2disptransform_ );
    tempzstretchtrans_->addObject( obj );
}


void Scene::addInlCrlZObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj);
    if ( so )
	so->set3DSurvGeom( SI().get3DGeometry(OD::UsrWork) );

    obj->setDisplayTransformation( inlcrlscale_ );
    inlcrlrotation_->addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    mDynamicCastGet(visBase::VisualObject*,vo,obj)

    if ( so )
    {
	so->set3DSurvGeom( SI().get3DGeometry(OD::UsrWork) );
	mAttachCB( so->getMovementNotifier(), Scene::objectMoved );

	so->setScene( this );
	STM().setCurrentScene( this );
	mDynamicCastGet( const visSurvey::VolumeDisplay*, voldisplay, so );
	so->allowShading( SettingsAccess().doesUserWantShading(voldisplay) );
    }

    if ( vo )
	vo->setSceneEventCatcher( &events_ );

    if ( so && so->isInlCrl() )
	addInlCrlZObject( obj );
    else if ( vo )
	addUTMObject( vo );

    if ( so && datatransform_ )
	so->setZAxisTransform( datatransform_,0 );

    if ( so )
	objectMoved( obj );

    sceneboundingboxupdated.trigger();
}


void Scene::removeObject( int idx )
{
    mousecursor_ = 0;
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so )
    {
	mDetachCB( so->getMovementNotifier(), Scene::objectMoved );
	hoveredposmodemanipobjids_ -= obj->id();
    }

    if ( idx<tempzstretchtrans_->size() )
	tempzstretchtrans_->removeObject( idx );
    else
	inlcrlrotation_->removeObject( idx-tempzstretchtrans_->size() );

    if ( so && !getMoreObjectsToDoHint() )
	objectMoved(0);
}


void Scene::setFixedZStretch( float zstretch )
{
    if ( mIsEqual(zstretch,curzstretch_,mDefEps) ) return;

    curzstretch_ = zstretch;
    updateTransforms( tkzs_ );
}


float Scene::getFixedZStretch() const
{ return curzstretch_; }


float Scene::getTempZStretch() const
{
    return tempzstretchtrans_
	? mCast(float,tempzstretchtrans_->getScale().z_)
	: 1.f;
}


void Scene::setTempZStretch( float zstretch )
{
    if ( !tempzstretchtrans_ )
    {
	pErrMsg("No z-transform");
	return;
    }

    tempzstretchtrans_->setScale( Coord3(1,1,zstretch) );
}


void Scene::setZScale( float zscale )
{
    zscale_ = zscale;
    updateTransforms( tkzs_ );
}


float Scene::getZScale() const
{ return zscale_; }


float Scene::getApparentVelocity(float zstretch) const
{
    if ( !zDomainInfo().def_.isTime() )
	return zstretch;

    //The depth when t=1 s
    //in xy units
    float depthat1sec = mZFactor(zstretch);

    //In meters
    if ( SI().xyInFeet() )
	depthat1sec *= mFromFeetFactorF;

    //in depth units
    if ( SI().depthsInFeet() )
	depthat1sec *= mToFeetFactorF;

    //compensate for twt
    const float travelleddistance = 2*depthat1sec;
    return travelleddistance; //distance travelled at 1 second
}


const mVisTrans* Scene::getTempZStretchTransform() const
{ return tempzstretchtrans_; }


const mVisTrans* Scene::getInlCrl2DisplayTransform() const
{ return inlcrlrotation_; }


const mVisTrans* Scene::getUTM2DisplayTransform() const
{ return utm2disptransform_; }


void Scene::showAnnotText( bool yn )
{ annot_->showText( yn ); }


bool Scene::isAnnotTextShown() const
{ return annot_->isTextShown(); }


void Scene::showAnnotScale( bool yn )
{ annot_->showScale( yn ); }


bool Scene::isAnnotScaleShown() const
{ return annot_->isScaleShown(); }


void Scene::showAnnotGrid( bool yn )
{ annot_->showGridLines( yn ); }


bool Scene::isAnnotGridShown() const
{ return annot_->isGridLinesShown(); }


void Scene::showAnnot( bool yn )
{ annot_->turnOn( yn ); }


bool Scene::isAnnotShown() const
{ return annot_->isOn(); }


void Scene::setAnnotText( int dim, const uiString& txt )
{
    if ( (dim==0 && SI().inlRange(OD::UsrWork).width()) ||
	 (dim==1 && SI().crlRange(OD::UsrWork).width()) ||
	 (dim==2 && SI().zRange(OD::UsrWork).width()) )
	annot_->setText( dim, txt );
}


const FontData& Scene::getAnnotFont() const
{
    return annot_->getFont();
}


void Scene::setAnnotFont( const FontData& nf )
{
    if ( scenecoltab_ ) scenecoltab_->setAnnotFont( nf );
    return annot_->setFont( nf );
}


void Scene::setAnnotColor( const Color& col )
{
    annot_->getMaterial()->setColor( col );

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( so )
	    so->setAnnotColor( col );
    }

    if ( scenecoltab_ )
	scenecoltab_->setLegendColor( col );
}


const Color Scene::getAnnotColor() const
{
    return annot_->getMaterial()->getColor();
}


const Selector<Coord3>* Scene::getSelector() const
{
    if ( !coordselector_ ) return 0;

    mDynamicCastGet(const visBase::PolygonCoord3Selector*,sel,coordselector_);
    if ( !sel ) return 0;

    return coordselector_;
}


Coord3 Scene::getMousePos( bool displayspace ) const
{
    if ( displayspace ) return xytmousepos_;

    Coord3 res = xytmousepos_;
    if ( datatransform_ && !mousetrckey_.isUdf() )
	res.z_ = datatransform_->transformTrcBack(
	mousetrckey_, (float)xytmousepos_.z_ );
    return res;
}


TrcKeyValue Scene::getMousePos() const
{ return TrcKeyValue( mousetrckey_, (float)getMousePos(false).z_ ); }


BufferString Scene::getMousePosValue() const
{ return BufferString(mouseposval_); }


BufferString Scene::getMousePosString() const
{ return mouseposstr_; }


void Scene::objectMoved( CallBacker* cb )
{
    Threads::Locker locker( updatelock_ );
    ObjectSet<const SurveyObject> activeobjects;
    int movedid = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so || !so->getMovementNotifier()
		 || !so->isAnyAttribEnabled() )
	    continue;

	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx))
	if ( !vo ) continue;
	if ( !vo->isOn() ) continue;

	if ( cb==vo ) movedid = vo->id();

	activeobjects += so;
    }

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));

	if ( so ) so->otherObjectsMoved( activeobjects, movedid );
    }
}


#define mGetPosModeManipObjInfo( idx, dataobj, so, canmove ) \
\
    const visBase::DataObject* dataobj = getObject( idx ); \
    mDynamicCastGet( const SurveyObject*, so, dataobj ); \
\
    const bool canmove = so && so->hasPosModeManipulator() \
			    && !so->isLocked() && dataobj->isOn();


void Scene::selChangeCB( CallBacker* cber )
{
    mCBCapsuleUnpack( int, selid, cber );

    for ( int idx=size()-1; idx>=0; idx-- )
    {
	mGetPosModeManipObjInfo( idx, dataobj, so, canmoveposmodemanip );
	if ( canmoveposmodemanip && selid==dataobj->id() )
	    return;
    }

    // reset if new selection via tree/scene/spacebar is not a posmodemanipobj
    posmodemanipdeselobjid_ = -1;
}


void Scene::togglePosModeManipObjSel()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();

    TypeSet<int> movableposmodemanipobjids;
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	mGetPosModeManipObjInfo( idx, dataobj, so, canmoveposmodemanip );
	if ( !so )
	    continue;

	if ( canmoveposmodemanip )
	    movableposmodemanipobjids += dataobj->id();

	if ( !selectedids.isPresent(dataobj->id()) )
	    continue;

	if ( canmoveposmodemanip && so->isManipulatorShown() )
	{
	    visBase::DM().selMan().select( posmodemanipdeselobjid_ );
	    posmodemanipdeselobjid_ = -1;
	    return;
	}

	if ( !so->hasPosModeManipulator() )
	    posmodemanipdeselobjid_ = dataobj->id();
    }

    if ( movableposmodemanipobjids.isEmpty() )
    {
	posmodemanipdeselobjid_ = -1;
	return;
    }

    while ( movableposmodemanipobjids.size() > 1 )
    {
	const int idx0 = hoveredposmodemanipobjids_.indexOf(
					    movableposmodemanipobjids[0] );
	const int idx1 = hoveredposmodemanipobjids_.indexOf(
					    movableposmodemanipobjids[1] );
	movableposmodemanipobjids.removeSingle( idx0<idx1 ? 0 : 1 );
    }

    visBase::DM().selMan().select( movableposmodemanipobjids[0] );
}


void Scene::selectPosModeManipObj( int selid )
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.isPresent(selid) )
	return;

    for ( int idx=size()-1; idx>=0; idx-- )
    {
	mGetPosModeManipObjInfo( idx, dataobj, so, canmoveposmodemanip );
	if ( selid == dataobj->id() )
	{
	    if ( canmoveposmodemanip )
	    {
		if ( posmodemanipdeselobjid_ < 0 )
		{
		    hoveredposmodemanipobjids_ -= selid;
		    hoveredposmodemanipobjids_ += selid;
		    togglePosModeManipObjSel();
		}
		else
		    visBase::DM().selMan().select( selid );
	    }

	    return;
	}
    }
}


void Scene::keyPressCB( CallBacker* cb )
{
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::Keyboard ) return;

    if ( eventinfo.key_ == OD::KB_Space )
    {
	if ( !spacebarwaspressed_ )
	    togglePosModeManipObjSel();

	spacebarwaspressed_ = eventinfo.pressed;
	events_.setHandled();
	return;
    }

    if ( eventinfo.pressed ) return;

    kbevent_.key_ = eventinfo.key_;
    kbevent_.modifier_ = eventinfo.buttonstate_;
    kbevent_.isrepeat_ = false;
    keypressed.trigger();
}


void Scene::mouseCB( CallBacker* cb )
{
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type == visBase::MouseClick )
    {
	mouseevent_ = MouseEvent( eventinfo.buttonstate_ );
	mouseevent_.setPressed( eventinfo.pressed );
	xytmousepos_ = eventinfo.worldpickedpos;
	mouseclicked.trigger();
	return;
    }

    if ( eventinfo.type != visBase::MouseMovement ) return;

    mouseposval_.setEmpty(); mouseposstr_.setEmpty();
    xytmousepos_.setUdf(); mousetrckey_.setUdf();

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    for ( int idx=0; idx<selectedids.size(); idx++ )
    {
	const visBase::DataObject* dataobj =
				visBase::DM().getObject( selectedids[idx] );
	mDynamicCastGet( const SurveyObject*, so, dataobj );

	if ( so && so->getSelMousePosInfo(eventinfo, xytmousepos_,
					  mouseposval_, mouseposstr_) )
	{
	    if ( !xytmousepos_.isUdf() )
		mousetrckey_ = TrcKey( SI().transform(
				    Coord(xytmousepos_.x_,xytmousepos_.y_)) );

	    mouseposchange.trigger();
	    return;
	}
    }

    const int sz = eventinfo.pickedobjids.size();
    if ( sz )
    {
	xytmousepos_ = eventinfo.worldpickedpos;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	    mDynamicCastGet(const SurveyObject*,so,pickedobj);
	    if ( so )
	    {
		if ( so->hasPosModeManipulator() )
		{
		    hoveredposmodemanipobjids_ -= pickedobj->id();
		    hoveredposmodemanipobjids_ += pickedobj->id();
		}

		if ( !mouseposval_[0] )
		{
		    BufferString newmouseposval;
		    BufferString newstr;
		    so->getMousePosInfo( eventinfo, xytmousepos_,
					 newmouseposval, newstr );
		    IOPar infopar;
		    so->getMousePosInfo( eventinfo, infopar );

		    if ( !infopar.get(sKey::TraceKey(),mousetrckey_) )
			mousetrckey_.setUdf();

		    if ( !newstr.isEmpty() )
			mouseposstr_ = newstr;

		    if ( newmouseposval[0] )
			mouseposval_ = newmouseposval;
		}

		break;
	    }
	}
	if ( mousetrckey_.isUdf() )
	    mousetrckey_ = TrcKey( SI().transform(
				Coord(xytmousepos_.x_,xytmousepos_.y_) ) );
    }

    mouseposchange.trigger();
}


void Scene::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );
    mousecursor_ = 0;

    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	DataObject* pickedobj =
			visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	mDynamicCastGet( SurveyObject*, so, pickedobj );
	if ( so )
	{
	    so->updateMouseCursorCB( cb );
	    mousecursor_ = so->getMouseCursor();
	    break;
	}
    }

    bool needmousecursorcall = false;
    mDefineStaticLocalObject( MouseCursor, polycursor, = MouseCursor::Bitmap );
    polycursor.filename_ = "cursor_polygonselect";
    mDefineStaticLocalObject( MouseCursor, rectcursor, = MouseCursor::Bitmap );
    rectcursor.filename_ = "cursor_rectangleselect";
    const visBase::PolygonSelection* sel = getPolySelection();
    if ( sel )
    {
	visBase::PolygonSelection::SelectionType type = sel->getSelectionType();
	if ( type==visBase::PolygonSelection::Polygon )
	    mousecursor_ = &polycursor;
	else if ( type==visBase::PolygonSelection::Rectangle )
	    mousecursor_ = &rectcursor;
    }

    mDefineStaticLocalObject( MouseCursor, pickcursor, = MouseCursor::Cross );

    if ( !mousecursor_ || mousecursor_->shape_==MouseCursor::NotSet )
    {
	const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
	for ( int idx=size()-1; idx>=0; idx-- )
	{
	    const visBase::DataObject* dataobj = getObject( idx );
	    mDynamicCastGet( const visSurvey::SurveyObject*, so, dataobj );
	    if ( !so || !selectedids.isPresent(dataobj->id()) )
		continue;

	    needmousecursorcall = true;

	    if ( so->isPicking() )
	    {
		mousecursor_ = &pickcursor;
		needmousecursorcall = false;
		break;
	    }
	}
    }

    if ( needmousecursorcall )
    {
	STM().setCurrentScene( this );
	STM().mouseCursorCall.trigger();
    }

    mousecursorchange.trigger();
}


void Scene::passMouseCursor( const MouseCursor& mc )
{ mousecursor_ = &mc; }


const MouseCursor* Scene::getMouseCursor() const
{ return mousecursor_; }


void Scene::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( datatransform_==zat ) return;

    if ( datatransform_ ) datatransform_->unRef();
    datatransform_ = zat;
    if ( datatransform_ ) datatransform_->ref();

    bool usedefaultzstretch = false;
    TrcKeyZSampling cs( OD::UsrWork );
    if ( !zat )
    {
	setZDomainInfo( ZDomain::Info(ZDomain::SI()) );
	setZScale( SI().zScale() );
    }
    else
    {
	const Interval<float> zrg = zat->getZInterval( false );
	if ( !zrg.isUdf() )
	{
	    cs.zsamp_.start = zrg.start;
	    cs.zsamp_.stop = zrg.stop;
	    cs.zsamp_.step = zat->getGoodZStep();
	}

	setZDomainInfo( zat->toZDomainInfo() );
	setZScale( zat->toZScale() );
	usedefaultzstretch = SI().getDefaultPars().get(
	    IOPar::compKey(sKeyZScale(), zDomainInfo().key()), curzstretch_ );

    }

    setAnnotScale( TrcKeyZSampling(false) );
    setTrcKeyZSampling( cs );

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;

	so->setZAxisTransform( zat,0 );
    }
    if ( usedefaultzstretch )
	updateTransforms( tkzs_ );

    updateAnnotationText();
}


ZAxisTransform* Scene::getZAxisTransform()
{ return datatransform_; }

const ZAxisTransform* Scene::getZAxisTransform() const
{ return datatransform_; }


void Scene::setMarkerPos( const TrcKeyValue& trkv, int sceneid )
{
    Coord3 displaypos( trkv.tk_.getCoord(), trkv.val_ );
    if ( sceneid==id() )
	displaypos.setUdf();

    if ( datatransform_ && !trkv.tk_.isUdf() && !mIsUdf(trkv.val_) )
	displaypos.z_ = datatransform_->transformTrc( trkv.tk_, trkv.val_ );

    const bool defined = displaypos.isDefined();
    if ( !defined )
    {
	if ( markerset_ )
	    markerset_->turnOn( false );
	return;
    }

    if ( !markerset_ )
    {
	markerset_ = createMarkerSet();
	addUTMObject( markerset_ );
    }

    markerset_->clearMarkers();
    markerset_->addPos( displaypos );
    markerset_->turnOn( true );
}


visBase::MarkerSet* Scene::createMarkerSet() const
{
    visBase::MarkerSet* markerset = visBase::MarkerSet::create();
    markerset->ref();
    markerset->setType( OD::MarkerStyle3D::Cross );
    markerset->setMarkersSingleColor( cDefaultMarkerColor() );
    markerset->setScreenSize( 6 );
    markerset->unRefNoDelete();
    return markerset;
}


void Scene::setMarkerSize( float nv )
{
    if ( !markerset_ )
    {
	markerset_ = createMarkerSet();
	addUTMObject( markerset_ );
	markerset_->turnOn( false );
    }

    markerset_->setScreenSize( nv );
}


float Scene::getMarkerSize() const
{
    if ( !markerset_ )
	return visBase::MarkerSet::cDefaultScreenSize();

    return markerset_->getScreenSize();
}


void Scene::setMarkerColor( const Color& nc )
{
    if ( !markerset_ )
    {
	markerset_ = createMarkerSet();
	addUTMObject( markerset_ );
	markerset_->turnOn( false );
    }

    markerset_->setMarkersSingleColor( nc );
}


const Color& Scene::getMarkerColor() const
{
    if ( !markerset_ )
	return cDefaultMarkerColor();

    mDefineStaticLocalObject( Color, singlecolor, );
    singlecolor = markerset_->getMarkersSingleColor();
    return singlecolor;
}


const Color& Scene::cDefaultMarkerColor()
{
    mDefineStaticLocalObject( Color, res, (255,255,255) );
    return res;
}


void Scene::fillPar( IOPar& par ) const
{
    par.setYN( sKeyShowAnnot(), isAnnotTextShown() );
    par.setYN( sKeyShowScale(), isAnnotScaleShown() );
    par.setYN( sKeyShowGrid(), isAnnotGridShown() );
    par.setYN( sKeyShowCube(), isAnnotShown() );
    par.set( sKeyZStretch(), curzstretch_ );
    par.setYN( sKeyShowColTab(), scenecoltab_->isOn() );
    par.set( sKeyMarkerColor(), getMarkerColor() );

    par.set( sKeyAnnotColor(), getAnnotColor() );

    if ( datatransform_ )
    {
	IOPar transpar;
	transpar.set( sKey::Name(), datatransform_->factoryKeyword() );
	datatransform_->fillPar( transpar );
	par.mergeComp( transpar, sKeyZAxisTransform() );
    }
    else
    {
	zdomaininfo_->def_.set( par );
	par.mergeComp( zdomaininfo_->pars_, ZDomain::sKey() );
	tkzs_.fillPar( par );
    }

    par.set( sKey::Scale(), zscale_ );

    if ( topimg_ )
    {
	IOPar topimgpar;
	topimgpar.set( sKeyTopImageID(), topimg_->id() );
	topimg_->fillPar( topimgpar );
	par.mergeComp( topimgpar, sKeyTopImage() );
    }

    if ( botimg_ )
    {
	IOPar botimgpar;
	botimgpar.set( sKeyBotImageID(), botimg_->id() );
	botimg_->fillPar( botimgpar );
	par.mergeComp( botimgpar, sKeyBottomImage() );
    }

    int nrchilds( 0 );
    for ( int idx=0;  idx<size(); idx++ )
    {
	BufferString childkey( childfix(), nrchilds );
	mDynamicCastGet(const visSurvey::SurveyObject*,survobj, getObject(idx));

	if ( !survobj || survobj->getSaveInSessionsFlag() == false )
	    continue;

	IOPar childpar;
	nrchilds++;

	mDynamicCastGet( const visBase::DataObject*, dobj, survobj );
	const int objid = dobj ? dobj->id() : -1;

	childpar.set( sKeyChildID(), objid );
	survobj->fillPar( childpar );
	par.mergeComp(childpar, childkey.buf() );
    }

    par.set( sKeyNrChild(), nrchilds );

}


void Scene::removeAll()
{
    visBase::DataObjectGroup::removeAll();
    const int idx = visBase::DataObjectGroup::getFirstIdx( tempzstretchtrans_ );
    if ( idx!=-1 )
	visBase::DataObjectGroup::removeObject( idx );

    tempzstretchtrans_ = 0; inlcrlrotation_ = 0; annot_ = 0;
    markerset_ = 0;

    mRemoveSelector;

    curzstretch_ = -1;

    hoveredposmodemanipobjids_.erase();
}


bool Scene::usePar( const IOPar& par )
{
    removeAll();
    setup();

    ZDomain::Info zdomaininfo( ZDomain::SI() );
    PtrMan<IOPar> transpar = par.subselect( sKeyZAxisTransform() );
    if ( transpar )
    {
	const char* nm = transpar->find( sKey::Name() );
	RefMan<ZAxisTransform> transform =
	    ZAxisTransform::factory().create( nm );
	if ( transform && transform->usePar( *transpar ) )
	{
	    float zscale;
	    setZAxisTransform( transform,0 );
	    if ( !par.get(sKey::Scale(),zscale) )
		zscale = transform->zScale();

	    setZScale( zscale );
	}
    }
    else
    {
	TrcKeyZSampling cs; float zscale=0.0f;
	if ( cs.usePar( par ) && par.get( sKey::Scale(), zscale ) )
	{
	    setTrcKeyZSampling( cs );
	    setZScale( zscale );
	    delete zdomaininfo_; zdomaininfo_ = new ZDomain::Info( par );
	}
    }

    int nrchilds( 0 );
    par.get( sKeyNrChild(), nrchilds );

    TypeSet<int> childids;
    for ( int idx=0; idx<nrchilds; idx++ )
    {
	BufferString key( childfix(), idx );
	PtrMan<IOPar> chldpar = par.subselect( key.buf() );

	int sessionobjid = -1;
	if ( chldpar->get( sKeyChildID(), sessionobjid ) )
	{
	    childids += sessionobjid;
	}
    }

    sort( childids );

    for ( int chld=0; chld<childids.size(); chld++ )
    {
	BufferString key( childfix(), chld );
	PtrMan<IOPar> chldpar = par.subselect( key.buf() );

	BufferString surobjtype;
	if ( !chldpar->get( sKey::Type(), surobjtype ) )
	    continue;

	if ( visBase::DM().getObject(childids[chld]) )
	{
	    pErrMsg("Hmmm");
		continue;
	}

	visSurvey::SurveyObject* survobj =
	    SurveyObject::factory().create( surobjtype );

	if ( !survobj )
	    continue;

	survobj->doRef();

	RefMan<visBase::VisualObject> dobj = 0;
	mDynamicCast( visBase::VisualObject*, dobj, survobj );
	if ( !dobj )
	{
	    survobj->doUnRef();
	    continue;
	}

	dobj->setID( childids[chld] );
	if ( !survobj->usePar( *chldpar ) )
	{
	    survobj->doUnRef();
	    continue;
	}

	addObject( dobj );

	survobj->doUnRef();
    }

    PtrMan<IOPar> topimgpar = par.subselect( sKeyTopImage() );
    if ( topimgpar )
    {
	createTopBotImage( true );
	topimg_->usePar( *topimgpar );
    }

    PtrMan<IOPar> botimgpar = par.subselect( sKeyBottomImage() );
    if ( botimgpar )
    {
	createTopBotImage( false );
	botimg_->usePar( *botimgpar );
    }

    par.getYN( sKeyShowColTab(), ctshownusepar_ );

    Color markercol = getMarkerColor();
    par.get( sKeyMarkerColor(), markercol );
    setMarkerColor( markercol );

    bool txtshown = true;
    par.getYN( sKeyShowAnnot(), txtshown );
    showAnnotText( txtshown );

    Color annotcolor;
    if ( par.get(sKeyAnnotColor(),annotcolor) )
	setAnnotColor( annotcolor );

    bool scaleshown = true;
    par.getYN( sKeyShowScale(), scaleshown );
    showAnnotScale( scaleshown );

    bool gridshown = true;
    par.getYN( sKeyShowGrid(), gridshown );
    showAnnotGrid( gridshown );

    bool cubeshown = true;
    par.getYN( sKeyShowCube(), cubeshown );
    showAnnot( cubeshown );

    float zstretch = curzstretch_;
    par.get( sKeyZStretch(), zstretch );

    if ( zstretch != curzstretch_ )
    {
	setTempZStretch( 1.0f );
	setFixedZStretch( zstretch );
    }

    usepar_ = true;
    return true;
}


int Scene::getImageFromPar( const IOPar& par, const char* key,
			    visBase::TopBotImage*& image )
{
    int imgid;
    if ( par.get(key,imgid) )
    {
	RefMan<DataObject> dataobj = visBase::DM().getObject( imgid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::TopBotImage*,im,dataobj.ptr())
	if ( !im ) return -1;
	int objidx = getFirstIdx( image );
	if ( objidx>=0 ) removeObject( objidx );
	image = im;
    }

    return 1;
}


void Scene::createTopBotImage( bool istop )
{
    if ( istop && !topimg_ )
    {
	topimg_ = visBase::TopBotImage::create();
	topimg_->setUiName( tr("Top Image") );
	addUTMObject( topimg_ );
	topimg_->turnOn( false );
    }

    if ( !istop && !botimg_ )
    {
	botimg_ = visBase::TopBotImage::create();
	botimg_->setUiName( tr("Bottom Image") );
	addUTMObject( botimg_ );
	botimg_->turnOn( false );
    }
}


visBase::TopBotImage* Scene::getTopBotImage( bool istop )
{
    return istop ? topimg_ : botimg_;
}


void Scene::setPolygonSelector( visBase::PolygonSelection* ps )
{
    mRemoveSelector;

    if ( ps )
    {
	polyselector_ = ps;
	polyselector_->ref();
	polyselector_->setUTMCoordinateTransform( utm2disptransform_ );
	mTryAlloc(coordselector_,
		  visBase::PolygonCoord3Selector(*polyselector_));
    }
}


void Scene::setSceneColTab( visBase::SceneColTab* sct )
{
    scenecoltab_ = sct;
    if ( usepar_ && scenecoltab_ )
    {
	scenecoltab_->turnOn( ctshownusepar_ );
	usepar_ = false;
    }

}


void Scene::savePropertySettings()
{
    Settings& setts = Settings::common();

#define mSaveProp(set,str,val) \
    setts.set( IOPar::compKey(sKeydTectScene(),str), val );

    mSaveProp( setYN, sKeyShowAnnot(), isAnnotTextShown() );
    mSaveProp( setYN, sKeyShowScale(), isAnnotScaleShown() );
    mSaveProp( setYN, sKeyShowGrid(), isAnnotGridShown() );
    mSaveProp( set, sKeyMarkerColor(), getMarkerColor() );

    setts.removeWithKey( sKeyAnnotFont() );
    setts.removeWithKey( sKeyAnnotColor() );

    mSaveProp( set, sKeyAnnotColor(), getAnnotColor() );
    setts.write( false );
}


Coord3 Scene::getTopBottomIntersection( const visBase::EventInfo& eventinfo,
				bool outerside, bool ignoreocclusion ) const
{
    ConstRefMan<SurvGeom3D> s3dgeom( SI().get3DGeometry(OD::UsrWork) );
    if ( !s3dgeom || !utm2disptransform_ || !tempzstretchtrans_ )
	return Coord3::udf();

    const StepInterval<int> inlrg = s3dgeom->inlRange();
    const StepInterval<int> crlrg = s3dgeom->crlRange();

    for ( int top=0; top<=1; top++ )
    {
	const double z = top ? s3dgeom->zRange().start : s3dgeom->zRange().stop;

	Coord3 p0( s3dgeom->getCoord(BinID(inlrg.start,crlrg.start)), z );
	utm2disptransform_->transform( p0 );
	tempzstretchtrans_->transform( p0 );

	Coord3 p1( s3dgeom->getCoord(BinID(inlrg.start,crlrg.stop)), z );
	utm2disptransform_->transform( p1 );
	tempzstretchtrans_->transform( p1 );

	Coord3 p2( s3dgeom->getCoord(BinID(inlrg.stop,crlrg.start)), z );
	utm2disptransform_->transform( p2 );
	tempzstretchtrans_->transform( p2 );

	if ( mCast(bool,top) == (outerside == s3dgeom->isRightHandSystem()) )
	    std::swap( p1, p2 );

	const Plane3 plane( p0, p1, p2 );
	double t;

	if ( !eventinfo.mouseline.intersectWith(plane,t) )
	    continue;

	if ( !ignoreocclusion && !mIsUdf(eventinfo.pickdepth) &&
	     t>eventinfo.pickdepth )
	    continue;

	const Coord3 viewpoint = eventinfo.mouseline.getPoint( t-0.001 );

	if ( plane.distanceToPoint(viewpoint,true) < 0.0 )
	{
	    Coord3 pos = eventinfo.mouseline.getPoint( t );
	    tempzstretchtrans_->transformBack( pos );
	    utm2disptransform_->transformBack( pos );
	    return pos;
	}
    }

    return Coord3::udf();
}


#define mMapMarginToSurvey( bid, ic, relativesurveymargin ) \
{ \
    const StepInterval<int> rg = s3dgeom->ic##Range(); \
    const float margin = fabs( (relativesurveymargin) * rg.width() ); \
    if ( bid.ic()>=rg.start-margin && bid.ic()<rg.start ) \
	bid.ic() = rg.start; \
    if ( bid.ic()>rg.stop && bid.ic()<=rg.stop+margin ) \
	bid.ic() = rg.stop; \
}

Coord3 Scene::getTopBottomSurveyPos( const visBase::EventInfo& eventinfo,
			    bool outerside, bool ignoreocclusion,
			    bool inlcrlspace, BufferString* topbotstr ) const
{
    ConstRefMan<SurvGeom3D> s3dgeom( SI().get3DGeometry(OD::UsrWork) );
    const Coord3 pos =
	    getTopBottomIntersection( eventinfo, outerside, ignoreocclusion );

    if ( mIsUdf(pos) )
	return pos;

    BinID bid = s3dgeom->transform( pos.getXY() );

    const float relativesurveymargin = 0.05;
    mMapMarginToSurvey( bid, inl, relativesurveymargin );
    mMapMarginToSurvey( bid, crl, relativesurveymargin );

    if ( !s3dgeom->includes(bid) )
	return Coord3::udf();

    if ( topbotstr )
    {
	const uiString topbotname = pos.z_<s3dgeom->zRange().center() ?
				    tr("Survey Top") : tr("Survey Bottom");
	*topbotstr = mFromUiStringTodo( topbotname );
    }

    if ( inlcrlspace )
	return Coord3( bid.inl(), bid.crl(), pos.z_);

    return Coord3( s3dgeom->transform(bid), pos.z_ );
}


void Scene::setMoreObjectsToDoHint( bool yn )
{ moreobjectstodo_ = yn; }


bool Scene::getMoreObjectsToDoHint() const
{ return moreobjectstodo_; }


void Scene::runUpdateQueueCB( CallBacker* cb )
{
    mDynamicCastGet(const visBase::Camera*,cam,cb)
    if ( cam )
	updateAnnotationOrientation( *cam );

    visBase::Scene::runUpdateQueueCB( cb );
}


void Scene::updateAnnotationOrientation( const visBase::Camera& cam )
{
    if ( !annot_ )
	return;

    Coord3 eye, center, up;
    cam.getLookAtMatrix( eye, center, up );

    const mVisTrans* displaytrans = annot_->getDisplayTransformation();
    mVisTrans::transformBack( displaytrans, eye );
    RefObjectSet<visBase::Text> axisnames = annot_->getAxisNames();

    TypeSet<Coord3> annotnmspos;
    for ( int idx=0; idx<axisnames.size(); idx++ )
    {
	visBase::TextDrawable* txt = axisnames[idx]->text( 0 );
	annotnmspos += txt->getPosition();
	mVisTrans::transformBack( displaytrans, annotnmspos[idx] );
    }

    const bool firstinlineon = annot_->isFaceGridShown( 0, true );
    const bool lastinlon = annot_->isFaceGridShown( 0, false );
    const bool firstcrlon = annot_->isFaceGridShown( 1, true );
    const bool lastcrlon = annot_->isFaceGridShown( 1, false );
    const bool firstzon = annot_->isFaceGridShown( 2, true );
    const bool lastzon = annot_->isFaceGridShown( 2, false );
    BoolTypeSet verticeshown;
    verticeshown += firstcrlon	  != firstzon;	// Inline	@ min/min
    verticeshown += firstinlineon != firstzon;	// Crossline	@ min/min
    verticeshown += firstinlineon != firstcrlon;// Vertical	@ min/min
    verticeshown += lastcrlon	  != firstzon;	// Inline	@ max/min
    verticeshown += lastinlon	  != firstzon;	// Crossline	@ max/min
    verticeshown += lastinlon	  != firstcrlon;// Vertical	@ max/min
    verticeshown += firstcrlon	  != lastzon;	// Inline	@ min/max
    verticeshown += firstinlineon != lastzon;	// Crossline	@ min/max
    verticeshown += firstinlineon != lastcrlon; // Vertical	@ min/max
    verticeshown += lastcrlon	  != lastzon;	// Inline	@ max/max
    verticeshown += lastinlon	  != lastzon;	// Crossline	@ max/max
    verticeshown += lastinlon	  != lastcrlon; // Vertical	@ max/max

    RefObjectSet<visBase::Text> axisvals = annot_->getAnnotTexts();
    for ( int idx=0; idx<axisnames.size(); idx++ )
    {
	if ( verticeshown[idx] )
	{
	    axisnames[idx]->turnOn( true );
	    axisvals[idx]->turnOn( true );
	    //TODO: set justification
	}
	else
	{
	    axisnames[idx]->turnOn( false );
	    axisvals[idx]->turnOn( false );
	}
    }
}

} // namespace visSurvey
