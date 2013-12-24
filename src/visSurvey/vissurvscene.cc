/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vissurvscene.h"

#include "basemapimpl.h"
#include "binidvalue.h"
#include "cubesampling.h"
#include "envvars.h"
#include "fontdata.h"
#include "iopar.h"
#include "keystrs.h"
#include "separstr.h"
#include "settings.h"
#include "survinfo.h"
#include "visannot.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "vissurvobj.h"
#include "vistopbotimage.h"
#include "zaxistransform.h"
#include "zdomain.h"


mCreateFactoryEntry( visSurvey::Scene );


namespace visSurvey {


const char* Scene::sKeyShowAnnot()	{ return "Show text"; }
const char* Scene::sKeyShowScale()	{ return "Show scale"; }
const char* Scene::sKeyShowGrid()	{ return "Show grid"; }
const char* Scene::sKeyAnnotFont()	{ return "Annotation font"; }
const char* Scene::sKeyShowCube()	{ return "Show cube"; }
const char* Scene::sKeyZStretch()	{ return "Z Stretch"; }
const char* Scene::sKeyZAxisTransform()	{ return "ZTransform"; }
const char* Scene::sKeyAppAllowShading(){ return "Allow shading";}
const char* Scene::sKeyTopImageID()	{ return "TopImage.ID"; }
const char* Scene::sKeyBotImageID()	{ return "BotImage.ID"; }

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyShowColTab()	{ return "Show ColTab"; }

static const char* sKeyChildID()	{ return "ID"; }
static const char* sKeyNrChild()	{ return "Number of Child"; }
static const char* childfix()		{ return "Child"; }


Scene::Scene()
    : tempzstretchtrans_(0)
    , annot_(0)
    , markerset_(0)
    , mouseposchange(this)
    , mouseposval_(0)
    , mouseposstr_("")
    , curzstretch_( 2 )
    , datatransform_( 0 )
    , appallowshad_(true)
    , userwantsshading_(true)
    , mousecursor_( 0 )
    , polyselector_( 0 )
    , coordselector_( 0 )
    , zscale_( SI().zScale() )
    , infopar_(*new IOPar)
    , basemap_( 0 )
    , basemapcursor_( 0 )
    , zdomaininfo_(new ZDomain::Info(ZDomain::SI()))
    , ctshownusepar_( false )
    , usepar_( false )
{
    events_.eventhappened.notify( mCB(this,Scene,mouseMoveCB) );
    setAmbientLight( 1 );
    setup();

    if ( GetEnvVarYN("DTECT_MULTITEXTURE_NO_SHADING" ) )
	userwantsshading_ = false;

    if ( userwantsshading_ )
    {
	bool noshading = false;
	Settings::common().getYN( "dTect.No shading", noshading );
	userwantsshading_ = !noshading;
    }
}


void Scene::updateAnnotationText()
{
    if ( !annot_ )
	return;

    if ( SI().inlRange(true).width() )
    	annot_->setText( 0, "In-line" );

    if ( SI().crlRange(true).width() )
    	annot_->setText( 1, "Cross-line" );

    if ( SI().zRange(true).width() )
    	annot_->setText( 2, zDomainUserName() );

    annot_->setAnnotScale( 2,
	    zdomaininfo_ ? zdomaininfo_->userFactor() : 1 );
}


void Scene::setup()
{
    annot_ = visBase::Annotation::create();

    const CubeSampling& cs = SI().sampling(true);

    if ( !SI().pars().get( sKeyZStretch(), curzstretch_ ) )
        SI().pars().get( "Z Scale", curzstretch_ );
    
    updateTransforms( cs );

    setCubeSampling( cs );
    addInlCrlZObject( annot_ );
    updateAnnotationText();

    topimg_ = visBase::TopBotImage::create();
    topimg_->setName( "TopImage");
    addUTMObject( topimg_ );
    topimg_->turnOn( false );

    botimg_ = visBase::TopBotImage::create();
    botimg_->setName( "BottomImage");
    addUTMObject( botimg_ );
    botimg_->turnOn( false );

    bool doshow = true;
#define mShowAnnot(str,func) \
    doshow = true; \
    Settings::common().getYN( BufferString(sKeydTectScene(),str), doshow ); \
    func( doshow );

#define mGetFontFromPar( par ) \
BufferString font; \
if ( par.get( sKeyAnnotFont(), font ) ) \
{ \
    FontData fd; \
    if ( fd.getFrom( font.buf() ) ) \
	setAnnotFont( fd ); \
}


    mShowAnnot( sKeyShowAnnot(), showAnnotText );
    mShowAnnot( sKeyShowScale(), showAnnotScale );
    mShowAnnot( sKeyShowGrid(), showAnnotGrid );

    mGetFontFromPar( Settings::common() )
}



#define mRemoveSelector \
unRefAndZeroPtr( polyselector_ ); \
deleteAndZeroPtr( coordselector_ )

Scene::~Scene()
{
    if ( basemap_ && basemapcursor_ )
    {
	basemap_->removeObject( basemapcursor_ );
	delete basemapcursor_;
	basemapcursor_ = 0;
    }

    events_.eventhappened.remove( mCB(this,Scene,mouseMoveCB) );

    if ( datatransform_ ) datatransform_->unRef();

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,getObject(idx));
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( vo ) vo->setSceneEventCatcher( 0 );

	if ( !so ) continue;

	if ( so->getMovementNotifier() )
	    so->getMovementNotifier()->remove( mCB(this,Scene,objectMoved) );
	so->setScene( 0 );
   }

    mRemoveSelector;
    delete &infopar_;
    delete zdomaininfo_;
}


#define mZFactor(stretch) (zscale_*stretch / 2)


void Scene::updateTransforms( const CubeSampling& cs )
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

    SceneTransformManager::computeICRotationTransform(*SI().get3DGeometry(true),
	zfactor, cs.zrg.center(), newinlcrlrotation, newinlcrlscale );

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
		    *SI().get3DGeometry(true), zfactor, cs.zrg.center(),
                    newutm2disptransform );

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
    return SI().isClockWise();
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

const char* Scene::zDomainUserName() const
{ return zdomaininfo_->userName(); }

const char* Scene::zDomainUnitStr( bool withparens ) const
{ return zdomaininfo_->unitStr( withparens ); }

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


void Scene::setCubeSampling( const CubeSampling& cs )
{
    cs_ = cs;
    if ( !annot_ ) return;

    annot_->setCubeSampling( cs );
}


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
	so->set3DSurvGeom( SI().get3DGeometry(true) );

    obj->setDisplayTransformation( inlcrlscale_ );
    inlcrlrotation_->addObject( obj );
}


void Scene::addObject( visBase::DataObject* obj )
{
    mDynamicCastGet(SurveyObject*,so,obj)
    mDynamicCastGet(visBase::VisualObject*,vo,obj)

    if ( so )
    {
	so->set3DSurvGeom( SI().get3DGeometry(true) );
	if ( so->getMovementNotifier() )
	    so->getMovementNotifier()->notify( mCB(this,Scene,objectMoved));

	so->setScene( this );
	STM().setCurrentScene( this );
	so->allowShading( userwantsshading_ && appallowshad_ );
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
}


void Scene::removeObject( int idx )
{
    mousecursor_ = 0;
    DataObject* obj = getObject( idx );
    mDynamicCastGet(SurveyObject*,so,obj)
    if ( so && so->getMovementNotifier() )
	so->getMovementNotifier()->remove( mCB(this,Scene,objectMoved) );

    if ( idx<tempzstretchtrans_->size() )
	tempzstretchtrans_->removeObject( idx );
    else
	inlcrlrotation_->removeObject( idx-tempzstretchtrans_->size() );

    if ( so )
	objectMoved(0);
}


void Scene::setFixedZStretch( float zstretch )
{
    if ( mIsEqual(zstretch,curzstretch_,mDefEps) ) return;

    curzstretch_ = zstretch;
    updateTransforms( cs_ );
}


float Scene::getFixedZStretch() const
{ return curzstretch_; }


float Scene::getTempZStretch() const
{
    return tempzstretchtrans_
    	? mCast(float,tempzstretchtrans_->getScale().z)
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
    updateTransforms( cs_ );
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


void Scene::setAnnotText( int dim, const char* txt )
{
    if ( (dim==0 && SI().inlRange(true).width()) || 
	 (dim==1 && SI().crlRange(true).width()) ||
	 (dim==2 && SI().zRange(true).width()) )
    	annot_->setText( dim, txt );
}


const FontData& Scene::getAnnotFont() const
{
    return annot_->getFont();
}


void Scene::setAnnotFont( const FontData& nf )
{
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
}


const Color Scene::getAnnotColor() const
{
    return annot_->getMaterial()->getColor();
}


const Selector<Coord3>* Scene::getSelector() const
{
    if ( !coordselector_ ) return 0;

    mDynamicCastGet(const visBase::PolygonCoord3Selector*,sel,coordselector_);
    if ( !sel || !sel->hasPolygon() ) return 0;

    return coordselector_;
}


Coord3 Scene::getMousePos( bool xyt ) const
{
   if ( xyt ) return xytmousepos_;
   
   Coord3 res = xytmousepos_;
   BinID binid = SI().transform( Coord(res.x,res.y) );
   res.x = binid.inl();
   res.y = binid.crl();
   return res;
}


BufferString Scene::getMousePosValue() const
{ return BufferString(mouseposval_); }


BufferString Scene::getMousePosString() const
{ return mouseposstr_; }


const MouseCursor* Scene::getMouseCursor() const
{ return mousecursor_; }


void Scene::objectMoved( CallBacker* cb )
{
    ObjectSet<const SurveyObject> activeobjects;
    int movedid = -1;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;
	if ( !so->getMovementNotifier() ) continue;

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


void Scene::mouseMoveCB( CallBacker* cb )
{
    STM().setCurrentScene( this );

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseMovement ) return;

    mouseposval_ = "";
    mouseposstr_ = "";
    xytmousepos_ = Coord3::udf();

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
		if ( !mouseposval_[0] )
		{
		    BufferString newmouseposval;
		    BufferString newstr;
		    so->getMousePosInfo( eventinfo, xytmousepos_,
			    		 newmouseposval, newstr );
		    so->getMousePosInfo( eventinfo, infopar_ );
		    if ( !newstr.isEmpty() )
			mouseposstr_ = newstr;

		    if ( newmouseposval[0] )
			mouseposval_ = newmouseposval;
		}

		mousecursor_ = so->getMouseCursor();
		break;
	    }
	}
    }

    mouseposchange.trigger();
}


void Scene::setBaseMap( BaseMap* bm )
{
    if ( basemap_ && basemapcursor_ )
    {
	basemap_->removeObject( basemapcursor_ );
	delete basemapcursor_;
	basemapcursor_ = 0; 
    }

    basemap_ = bm;
    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx));
	if ( so ) so->setBaseMap( bm );
    }
}


BaseMap* Scene::getBaseMap()
{ return basemap_; }


void Scene::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat ) return;

    if ( datatransform_ ) datatransform_->unRef();
    datatransform_ = zat;
    if ( datatransform_ ) datatransform_->ref();

    CubeSampling cs = SI().sampling( true );
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
	    cs.zrg.start = zrg.start;
	    cs.zrg.stop = zrg.stop;
	    cs.zrg.step = zat->getGoodZStep();
	}

	setZDomainInfo( zat->toZDomainInfo() );
	setZScale( zat->toZScale() );
    }

    setCubeSampling( cs );

    for ( int idx=0; idx<size(); idx++ )
    {
	mDynamicCastGet(SurveyObject*,so,getObject(idx))
	if ( !so ) continue;

	so->setZAxisTransform( zat,0 );
    }

    updateAnnotationText();
}


ZAxisTransform* Scene::getZAxisTransform()
{ return datatransform_; }

const ZAxisTransform* Scene::getZAxisTransform() const
{ return datatransform_; }


void Scene::setMarkerPos( const Coord3& coord, int sceneid )
{
    updateBaseMapCursor( coord );

    Coord3 displaypos = coord;
    if ( sceneid==id() )
	displaypos = Coord3::udf();

    if ( datatransform_ && coord.isDefined() )
    {
	BufferString linenm; int trcnr = -1;
	infopar_.get( sKey::LineKey(), linenm );
	infopar_.get( sKey::TraceNr(), trcnr );
	if ( !linenm.isEmpty() && trcnr>=0 )
	{
	    BinID bid( datatransform_->lineIndex(linenm), trcnr );
	    displaypos.z = datatransform_->transform(
		    BinIDValue(bid,(float) coord.z) );
	}
	else
	    displaypos.z = datatransform_->transform( coord );
    }

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
    markerset_->turnMarkerOn( 0, true );
}


void Scene::updateBaseMapCursor( const Coord& coord )
{
    if ( !basemap_ )
	return;

    const bool defined = coord.isDefined();

    if ( defined && !basemapcursor_ )
    {
	basemapcursor_ = new BaseMapMarkers;
	basemapcursor_->setMarkerStyle(
		MarkerStyle2D(MarkerStyle2D::Target,5) );
    }

    if ( !basemapcursor_ )
	return;

    Threads::Locker lckr( basemapcursor_->lock_,
	    		  Threads::Locker::DontWaitForLock );

    if ( lckr.isLocked() )
    {
	if ( !defined )
	    basemapcursor_->positions().erase();
	else if ( basemapcursor_->positions().isEmpty() )
		basemapcursor_->positions() += coord;
	else
	    basemapcursor_->positions()[0] = coord;

	lckr.unlockNow();
	basemapcursor_->updateGeometry();
    }

	basemap_->addObject( basemapcursor_ );
}


visBase::MarkerSet* Scene::createMarkerSet() const
{
    visBase::MarkerSet* markerset = visBase::MarkerSet::create();
    markerset->ref();
    markerset->setType( MarkerStyle3D::Cross );
    markerset->setMarkersSingleColor( cDefaultMarkerColor() );
    markerset->setScreenSize( 9 );
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

    mDefineStaticLocalObject( const Color, singlecolor, 
			      = markerset_->getMarkersSingleColor() );
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
    par.setYN( sKeyAppAllowShading(), appallowshad_ );
    par.setYN( sKeyShowColTab(), scenecoltab_->isOn() );

    BufferString font;
    getAnnotFont().putTo( font );
    par.set( sKeyAnnotFont(), font );

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
	cs_.fillPar( par );
    }

    par.set( sKey::Scale(), zscale_ );
    par.set( sKeyTopImageID(), topimg_->id() );
    par.set( sKeyBotImageID(), botimg_->id() );

    int nrchilds( 0 );
    for ( int idx=0;  idx<size(); idx++ )
    {
	BufferString childkey( childfix(), nrchilds );
	mDynamicCastGet(const visSurvey::SurveyObject*,survobj, getObject(idx));
	if ( !survobj )
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

    mRemoveSelector;

    curzstretch_ = -1;
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
	CubeSampling cs; float zscale;
	if ( cs.usePar( par ) && par.get( sKey::Scale(), zscale ) )
	{
	    setCubeSampling( cs );
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

    par.getYN( sKeyShowColTab(), ctshownusepar_ );

    bool txtshown = true;
    par.getYN( sKeyShowAnnot(), txtshown );
    showAnnotText( txtshown );

    mGetFontFromPar( par );

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


visBase::TopBotImage* Scene::getTopBotImage( bool istop )
{ return istop ? topimg_ : botimg_; }


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
#define mSaveProp(str,func) \
    Settings::common().setYN( BufferString(sKeydTectScene(),str), func() );

    mSaveProp( sKeyShowAnnot(), isAnnotTextShown );
    mSaveProp( sKeyShowScale(), isAnnotScaleShown );
    mSaveProp( sKeyShowGrid(), isAnnotGridShown );

    BufferString font;
    getAnnotFont().putTo( font );
    Settings::common().set( sKeyAnnotFont(), font );
    Settings::common().write();
}

} // namespace visSurvey

