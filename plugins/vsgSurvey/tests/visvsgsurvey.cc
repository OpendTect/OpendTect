/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Scene transforms and the survey box annotation, compared with
 visSurvey and visBase. Also the status text and selection. No GPU.

-*/

#include "testprog.h"

#include "visvsgannotation.h"
#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "visvsgobject.h"
#include "visvsgsurvscene.h"
#include "visvsgsurvtransmgr.h"
#include "visvsgtransform.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keyenum.h"
#include "math2.h"
#include "moddepmgr.h"
#include "settings.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "thread.h"
#include "zdomain.h"

#include <vsg/maths/vec4.h>

#include "visannot.h"
#include "visdataman.h"
#include "vistext.h"
#include "vistransform.h"
#include "vistransmgr.h"

using VBTrans = visBase::Transformation;

namespace
{

struct RefTransforms
{
    RefMan<VBTrans>	rot_ = VBTrans::create();
    RefMan<VBTrans>	scale_ = VBTrans::create();
    RefMan<VBTrans>	utm_ = VBTrans::create();
};

} // namespace


static const BufferStringSet& survDirNames()
{
    static BufferStringSet ret;
    if ( ret.isEmpty() )
    {
	ret.add( "F3_Test_Survey" )
	   .add( "F3_Test_Survey_DisplayFT" )
	   .add( "F3_Test_Survey_XYinft" )
	   .add( "F3_Test_Survey_DepthM" )
	   .add( "F3_Test_Survey_DepthFT" );
    }

    return ret;
}


static bool isClose( const Coord3& a, const Coord3& b, double releps )
{
    const double scale = mMAX( 1., mMAX(a.abs(),b.abs()) );
    return a.distTo( b ) <= releps * scale;
}


static BufferString crdStr( const Coord3& crd )
{
    BufferString ret( "(" );
    ret.add( crd.x_ ).add( ", " ).add( crd.y_ ).add( ", " ).add( crd.z_ )
       .add( ")" );
    return ret;
}


static BufferString cmpMsg( const char* prefix, const Coord3& res,
			    const Coord3& ref )
{
    BufferString ret( prefix );
    ret.add( crdStr(res) ).add( " vs " ).add( crdStr(ref) );
    return ret;
}


static Coord3 getCornerIC( const TrcKeyZSampling& tkzs, int idx )
{
    const Interval<int> inlrg = tkzs.hsamp_.inlRange();
    const Interval<int> crlrg = tkzs.hsamp_.crlRange();
    const Interval<float>& zrg = tkzs.zsamp_;
    const bool inlstop = idx==1 || idx==2 || idx==5 || idx==6;
    const bool crlstop = idx==2 || idx==3 || idx==6 || idx==7;
    const bool zstop = idx>=4;
    return Coord3( inlstop ? inlrg.stop_ : inlrg.start_,
		   crlstop ? crlrg.stop_ : crlrg.start_,
		   zstop ? zrg.stop_ : zrg.start_ );
}


static float getZFactor( float zstretch )
{
    float zfactor = -1 * SI().zScale(false) * zstretch;
    if ( SI().zDomainInfo().isTime() )
	zfactor /= 2;

    return zfactor;
}


static void computeRef( float zstretch, float zmid, RefTransforms& ref )
{
    ConstRefMan<Survey::Geometry3D> geom = SI().get3DGeometry( true );
    const float zfactor = getZFactor( zstretch );
    visSurvey::SceneTransformManager::computeICRotationTransform(
		*geom.ptr(), zfactor, zmid, ref.rot_.ptr(), ref.scale_.ptr() );
    visSurvey::SceneTransformManager::computeUTM2DisplayTransform(
		*geom.ptr(), zfactor, zmid, ref.utm_.ptr() );
}


static void getScale( const TrcKeyZSampling& tkzs, TrcKeyZSampling& scalecs,
		      double* scalefac )
{
    // As visSurvey::Scene::updateAnnotationText()
    const ZDomain::Info& zdom = SI().zDomainInfo();
    scalecs = tkzs;
    scalefac[0] = scalefac[1] = scalefac[2] = 1.;
    if ( zdom.isDepth() &&
	 ((zdom.isDepthMeter() && SI().depthsInFeet()) ||
	  (zdom.isDepthFeet() && !SI().depthsInFeet())) )
    {
	const double fac = zdom.isDepthMeter() ? mToFeetFactorD
					       : mFromFeetFactorD;
	scalecs.zsamp_.scale( fac );
	scalefac[2] = fac;
    }
    else
    {
	scalecs.zsamp_.scale( sCast(float,zdom.userFactor()) );
	scalefac[2] = zdom.userFactor();
    }
}


static bool testTransforms( const char* survnm )
{
    const TrcKeyZSampling survtkzs = SI().sampling( true );
    const float zstretches[] = { 0.1f, 1.f, 2.f, 10.f };
    for ( const float zstretch : zstretches )
    {
	RefMan<ODVsg::Scene> scene = ODVsg::Scene::create();
	scene->setFixedZStretch( zstretch );

	RefTransforms ref;
	computeRef( zstretch, survtkzs.zsamp_.center(), ref );

	const mDGBVisTrans* rot = scene->getInlCrl2DisplayTransform();
	const mDGBVisTrans* utm = scene->getUTM2DisplayTransform();
	const ODVsg::Annotation* annot = scene->getAnnotation();
	const mDGBVisTrans* scale = annot->getDisplayTransformation();
	mRunStandardTest( rot && utm && scale, "Scene transforms exist" );

	for ( int idx=0; idx<8; idx++ )
	{
	    const Coord3 ic = getCornerIC( survtkzs, idx );
	    Coord3 icref = ic;
	    ref.scale_->transform( icref );
	    ref.rot_->transform( icref );
	    Coord3 icres = ic;
	    scale->transform( icres );
	    rot->transform( icres );

	    const Coord xy = SI().transform( BinID(int(ic.x_),int(ic.y_)) );
	    Coord3 utmref( xy, ic.z_ );
	    ref.utm_->transform( utmref );
	    Coord3 utmres( xy, ic.z_ );
	    utm->transform( utmres );

	    Coord3 annotref = ic;
	    ref.scale_->transform( annotref );
	    const Coord3 annotres = annot->getCorner( idx );

	    BufferString msg( survnm );
	    msg.add( ", Z stretch " ).add( zstretch )
	       .add( ", corner " ).add( idx ).add( ": " );
	    mRunStandardTestWithError( isClose(icres,icref,1e-9),
		"Inl/crl chain equals visSurvey",
		cmpMsg(msg,icres,icref) );
	    mRunStandardTestWithError( isClose(utmres,utmref,1e-9),
		"UTM chain equals visSurvey",
		cmpMsg(msg,utmres,utmref) );
	    mRunStandardTestWithError( isClose(utmres,icres,1e-6),
		"UTM and inl/crl chains agree",
		cmpMsg(msg,utmres,icres) );
	    mRunStandardTestWithError( isClose(annotres,annotref,1e-6),
		"Annotation corner equals visSurvey",
		cmpMsg(msg,annotres,annotref) );
	}
    }

    return true;
}


static bool compareAnnotations( const TrcKeyZSampling& tkzs,
				const char* desc )
{
    RefTransforms ref;
    computeRef( 2.f, SI().sampling(true).zsamp_.center(), ref );
    RefMan<mDGBVisTrans> scale = mDGBVisTrans::create();
    {
	ConstRefMan<Survey::Geometry3D> geom = SI().get3DGeometry( true );
	RefMan<mDGBVisTrans> rot = mDGBVisTrans::create();
	ODVsg::SceneTransformManager::computeICRotationTransform(
		*geom.ptr(), getZFactor(2.f),
		SI().sampling(true).zsamp_.center(), rot.ptr(), scale.ptr() );
    }

    RefMan<visBase::Annotation> vbannot = visBase::Annotation::create();
    RefMan<ODVsg::Annotation> annot = ODVsg::Annotation::create();
    vbannot->setDisplayTransformation( ref.scale_.ptr() );
    annot->setDisplayTransformation( scale.ptr() );
    vbannot->setTrcKeyZSampling( tkzs );
    annot->setTrcKeyZSampling( tkzs );

    TrcKeyZSampling scalecs;
    double scalefac[3];
    getScale( tkzs, scalecs, scalefac );
    vbannot->setScale( scalecs, scalefac, 3, true );
    annot->setScale( scalecs, scalefac, 3, true );

    BufferString survnm( SI().name() );
    survnm.add( ", " ).add( desc ).add( ": " );
    IOPar vbpar, par;
    vbannot->fillPar( vbpar );
    annot->fillPar( par );
    for ( int idx=0; idx<8; idx++ )
    {
	const BufferString key( "Corner ", idx );
	Coord3 vbpos, pos;
	vbpar.get( key, vbpos.x_, vbpos.y_, vbpos.z_ );
	par.get( key, pos.x_, pos.y_, pos.z_ );
	mRunStandardTestWithError( isClose(pos,vbpos,1e-6),
		"Box corner equals visBase::Annotation",
		cmpMsg(survnm,pos,vbpos) );
    }

    const visBase::Text2* vbtexts = vbannot->getAnnotTexts();
    const TypeSet<ODVsg::Annotation::Tick>& ticks = annot->getTicks();
    BufferString msg( survnm );
    msg.add( int(ticks.size()) ).add( " ticks vs " )
       .add( vbtexts->nrTexts() );
    mRunStandardTestWithError( int(ticks.size())==vbtexts->nrTexts(),
			       "Number of ticks equals visBase", msg );

    for ( int idx=0; idx<vbtexts->nrTexts(); idx++ )
    {
	const visBase::Text* vbtext = vbtexts->text( idx );
	const ODVsg::Annotation::Tick& tick = ticks[idx];
	const Coord3 vbpos = vbtext->getPosition();
	const BufferString vbval = vbtext->getText().getFullString();
	const BufferString val = toUiString( tick.value_ ).getFullString();
	msg.set( survnm ).add( "tick " ).add( idx ).add( ": " )
	   .add( val ).add( " " ).add( crdStr(tick.pos_) ).add( " vs " )
	   .add( vbval ).add( " " ).add( crdStr(vbpos) );
	mRunStandardTestWithError( val==vbval && isClose(tick.pos_,vbpos,1e-6),
				   "Tick equals visBase", msg );
    }

    return true;
}


static bool testAnnotation()
{
    const TrcKeyZSampling survtkzs = SI().sampling( true );
    TrcKeyZSampling workarea = survtkzs;
    const StepInterval<int> inlrg = survtkzs.hsamp_.inlRange();
    const StepInterval<int> crlrg = survtkzs.hsamp_.crlRange();
    const StepInterval<float>& zrg = survtkzs.zsamp_;
    workarea.hsamp_.start_.inl() = inlrg.atIndex( inlrg.nrSteps()/4 );
    workarea.hsamp_.stop_.inl() = inlrg.atIndex( inlrg.nrSteps()/2 );
    workarea.hsamp_.start_.crl() = crlrg.atIndex( crlrg.nrSteps()/4 );
    workarea.hsamp_.stop_.crl() = crlrg.atIndex( crlrg.nrSteps()/2 );
    workarea.zsamp_.start_ = zrg.atIndex( zrg.nrSteps()/4 );
    workarea.zsamp_.stop_ = zrg.atIndex( zrg.nrSteps()/2 );

    TrcKeyZSampling oneinl = survtkzs;
    oneinl.hsamp_.stop_.inl() = oneinl.hsamp_.start_.inl();

    return compareAnnotations( survtkzs, "survey" ) &&
	   compareAnnotations( workarea, "small work area" ) &&
	   compareAnnotations( oneinl, "one inline" );
}


static bool checkCorners( const ODVsg::Scene& scene,
			  const TrcKeyZSampling& tkzs, float zstretch,
			  const char* desc )
{
    RefTransforms ref;
    computeRef( zstretch, tkzs.zsamp_.center(), ref );
    const ODVsg::Annotation* annot = scene.getAnnotation();
    for ( int idx=0; idx<8; idx++ )
    {
	Coord3 annotref = getCornerIC( tkzs, idx );
	ref.scale_->transform( annotref );
	const Coord3 annotres = annot->getCorner( idx );
	BufferString msg( SI().name() );
	msg.add( ", corner " ).add( idx ).add( ": " );
	mRunStandardTestWithError( isClose(annotres,annotref,1e-6), desc,
				   cmpMsg(msg,annotres,annotref) );
    }

    return true;
}


static bool testWorkArea()
{
    const TrcKeyZSampling survtkzs = SI().sampling( true );
    TrcKeyZSampling workarea = survtkzs;
    const StepInterval<int> inlrg = survtkzs.hsamp_.inlRange();
    workarea.hsamp_.start_.inl() = inlrg.atIndex( inlrg.nrSteps()/3 );
    const StepInterval<float>& zrg = survtkzs.zsamp_;
    workarea.zsamp_.stop_ = zrg.atIndex( zrg.nrSteps()/2 );

    RefMan<ODVsg::Scene> scene = ODVsg::Scene::create();
    scene->setFixedZStretch( 3.f );
    scene->setTrcKeyZSampling( workarea, true );
    if ( !checkCorners(*scene,workarea,3.f,
		       "Annotation follows the work area") )
	return false;

    const mDGBVisTrans* oldscale =
			scene->getAnnotation()->getDisplayTransformation();
    scene->setFixedZStretch( 5.f );
    mRunStandardTest(
	scene->getAnnotation()->getDisplayTransformation() != oldscale,
	"Annotation gets the new display transformation" );
    return checkCorners( *scene, workarea, 5.f,
			 "Annotation follows the Z stretch" );
}


static bool testVirtualCalls()
{
    RefMan<ODVsg::Scene> scene = ODVsg::Scene::create();
    visSurvey::Scene& osgscene = *scene.ptr();

    const OD::Color col( 10, 20, 30 );
    osgscene.setAnnotColor( col );
    mRunStandardTest( scene->getAnnotColor()==col &&
		      osgscene.getAnnotColor()==col,
		      "setAnnotColor through visSurvey::Scene reaches the scene" );

    osgscene.showAnnotGrid( false );
    mRunStandardTest( !scene->isAnnotGridShown() &&
		      !osgscene.isAnnotGridShown(),
		      "showAnnotGrid through visSurvey::Scene reaches the scene" );

    osgscene.showAnnotGrid( true );
    mRunStandardTest( scene->isAnnotGridShown(),
		      "The grid can be shown again through visSurvey::Scene" );

    RefMan<visSurvey::Scene> other = visSurvey::Scene::create();
    mRunStandardTest( scene->getID()!=other->getID(),
		      "The scene has an id of its own among the OSG scenes" );
    mRunStandardTest( visBase::DM().getObject(scene->id())==
		      static_cast<visBase::DataObject*>(scene.ptr()),
		      "The id of the scene finds it in the OSG data manager" );

    return true;
}


static bool testBackgroundColor()
{
    const BufferString key( "dTect.Scene.",
			    ODVsg::Scene::sKeyBackgroundColor() );
    Settings& setts = Settings::common();
    OD::Color savedcol;
    const bool hadcol = setts.get( key.buf(), savedcol );
    setts.removeWithKey( key.buf() );

    RefMan<ODVsg::Scene> scene = ODVsg::Scene::create();
    mRunStandardTest( scene->getBackgroundColor()==OD::Color::Anthracite(),
		      "Background is anthracite without a setting" );
    mRunStandardTest( !scene->isBackgroundGradient(),
		      "The background has no gradient at first" );

    const OD::Color col( 10, 20, 30 );
    setts.set( key.buf(), col );
    scene = ODVsg::Scene::create();
    const bool isset = scene->getBackgroundColor() == col;
    if ( hadcol )
	setts.set( key.buf(), savedcol );
    else
	setts.removeWithKey( key.buf() );

    mRunStandardTest( isset, "Background follows the OD scene setting" );

    scene->setBackgroundColor( OD::Color::White() );
    mRunStandardTest( scene->getBackgroundColor()==OD::Color::White(),
		      "Background can be changed" );

    scene->setAnnotColor( OD::Color::White() );
    const OD::Color onwhite = scene->getDisplayedAnnotColor();
    mRunStandardTest( scene->getAnnotColor()==OD::Color::White() &&
		      onwhite.r()<160 && onwhite.r()==onwhite.g() &&
		      onwhite.g()==onwhite.b(),
		      "The annotation is drawn dark gray on a white background" );
    scene->setBackgroundColor( OD::Color::Anthracite() );
    mRunStandardTest( scene->getDisplayedAnnotColor()==OD::Color::White(),
		      "The annotation is drawn white on a dark background" );

    mRunStandardTest( !scene->isBackgroundGradient() &&
		      scene->getBackgroundCornerColor()==
						OD::Color::Anthracite(),
		      "Without a gradient the corners have the background "
		      "color" );
    scene->setBackgroundGradient( true );
    const OD::Color corner = scene->getBackgroundCornerColor();
    mRunStandardTest( scene->isBackgroundGradient() && corner.r()>=16 &&
		      corner.r()<=20 && corner.r()==corner.g() &&
		      corner.g()==corner.b(),
		      "With a gradient the corners are darker" );
    mRunStandardTest( scene->getDisplayedAnnotColor()==OD::Color::White(),
		      "The annotation stays white on the dark gradient" );

    scene->setBackgroundColor( OD::Color(255,250,250) );
    mRunStandardTest( scene->getDisplayedAnnotColor()!=OD::Color::White() &&
		      scene->getDisplayedCornerAnnotColor()==OD::Color::White(),
		      "On a light gradient the middle is drawn gray and the "
		      "darker corners white" );
    scene->setBackgroundGradient( false );
    return true;
}


class ClickTarget : public ODVsg::VisualObjectImpl
{
public:
    static RefMan<ClickTarget> create()
    {
	RefMan<ClickTarget> ret = new ClickTarget;
	return ret;
    }

    int			rightclicks_ = 0;

private:
			ClickTarget() : VisualObjectImpl(true)
			{
			    rightClicked()->notify(
					mCB(this,ClickTarget,rightCB) );
			}
			~ClickTarget()
			{
			    if ( rightClicked() )
				rightClicked()->remove(
					mCB(this,ClickTarget,rightCB) );
			}

    void		rightCB( CallBacker* )	{ rightclicks_++; }
};


bool testSurveyCorner( ODVsg::Scene& scene, int corner, float zstretch )
{
    scene.setFixedZStretch( zstretch );
    const TrcKeyZSampling& cs = scene.getTrcKeyZSampling();
    const BinID start = cs.hsamp_.start_;
    const BinID stop = cs.hsamp_.stop_;
    const BinID bids[4] = {
	BinID( start.inl(), start.crl() ),
	BinID( stop.inl(), start.crl() ),
	BinID( stop.inl(), stop.crl() ),
	BinID( start.inl(), stop.crl() ) };
    const float z = corner<4 ? cs.zsamp_.start_ : cs.zsamp_.stop_;
    const BinID bid = bids[corner%4];

    const Coord3 disp = scene.getAnnotation()->getCorner( corner );
    vsg::dvec4 point( disp.x_, disp.y_, disp.z_, 1. );
    point = scene.getTempZStretchTransform()->getMatrix() *
	    (scene.getInlCrl2DisplayTransform()->getMatrix() * point);

    Coord3 world( point.x, point.y, point.z );
    scene.getUTM2DisplayTransform()->transformBack( world );

    ODVsg::EventInfo eventinfo;
    eventinfo.type = ODVsg::MouseMovement;
    eventinfo.displaypickedpos = Coord3( point.x, point.y, point.z );
    eventinfo.worldpickedpos = world;
    eventinfo.pickedobjids += scene.getAnnotation()->id();
    scene.applyEvent( eventinfo );

    const TrcKeyValue pos = scene.getMousePos();
    const int inlstep = mMAX( 1, cs.hsamp_.step_.inl() );
    const int crlstep = mMAX( 1, cs.hsamp_.step_.crl() );
    const double dinl = Math::Abs( double(pos.tk_.inl()-bid.inl()) ) / inlstep;
    const double dcrl = Math::Abs( double(pos.tk_.crl()-bid.crl()) ) / crlstep;
    const double zspan = mMAX( 1e-6, double(cs.zsamp_.width()) );
    const double dz = Math::Abs( double(pos.val_)-double(z) );
    BufferString status;
    scene.getMousePosString().getFullString( &status );
    const bool ok = dinl<=0.5 && dcrl<=0.5 && dz<=1e-4*zspan &&
		    status.indexOf(toString(bid.inl()))>=0 &&
		    status.indexOf(toString(bid.crl()))>=0;
    BufferString msg( "Survey corner ", corner, ", stretch " );
    msg.add( zstretch ).add( " inl/crl " ).add( pos.tk_.inl() ).add( "/" )
       .add( pos.tk_.crl() ).add( " expected " ).add( bid.inl() ).add( "/" )
       .add( bid.crl() ).add( " z " ).add( pos.val_ ).add( " expected " )
       .add( z ).add( " status '" ).add( status ).add( "'" );
    mRunStandardTestWithError( ok, "Survey corner round-trips", msg );
    return true;
}


bool testPicking()
{
    RefMan<ODVsg::Scene> scene = ODVsg::Scene::create();
    scene->setInteractionEnabled( false );
    for ( int corner=0; corner<8; corner++ )
    {
	if ( !testSurveyCorner(*scene,corner,1.f) ||
	     !testSurveyCorner(*scene,corner,2.f) )
	    return false;
    }

    ODVsg::EventInfo miss;
    miss.type = ODVsg::MouseMovement;
    scene->applyEvent( miss );
    BufferString empty;
    scene->getMousePosString().getFullString( &empty );
    mRunStandardTest( empty.isEmpty(), "Status clears on a miss" );

    RefMan<ClickTarget> first = ClickTarget::create();
    RefMan<ClickTarget> second = ClickTarget::create();
    scene->setInteractionEnabled( true );

    ODVsg::EventInfo click;
    click.type = ODVsg::MouseClick;
    click.buttonstate_ = OD::LeftButton;
    click.pressed = false;
    click.dragging = false;
    click.pickedobjids += first->id();
    click.worldpickedpos = Coord3( 0, 0, 0 );
    click.displaypickedpos = click.worldpickedpos;
    scene->applyEvent( click );
    mRunStandardTest( ODVsg::DM().selMan().selected().size()==1 &&
		      ODVsg::DM().selMan().selected()[0]==first->id(),
		      "First click selects the object" );

    click.pickedobjids.setEmpty();
    click.pickedobjids += second->id();
    scene->applyEvent( click );
    mRunStandardTest( ODVsg::DM().selMan().selected().size()==1 &&
		      ODVsg::DM().selMan().selected()[0]==second->id(),
		      "Second click replaces the selection" );

    click.pickedobjids.setEmpty();
    scene->applyEvent( click );
    mRunStandardTest( ODVsg::DM().selMan().selected().isEmpty(),
		      "Empty click clears the selection" );

    scene->setInteractionEnabled( false );
    click.pickedobjids += first->id();
    scene->applyEvent( click );
    mRunStandardTest( ODVsg::DM().selMan().selected().isEmpty(),
		      "View mode does not select" );

    scene->setInteractionEnabled( true );
    click.pickedobjids.setEmpty();
    click.buttonstate_ = OD::RightButton;
    click.dragging = false;
    click.pickedobjids += first->id();
    scene->applyEvent( click );
    mRunStandardTest( first->rightclicks_==1,
		      "Right release fires rightClicked" );

    click.dragging = true;
    scene->applyEvent( click );
    mRunStandardTest( first->rightclicks_==1,
		      "Right drag does not fire rightClicked" );

    ODVsg::DM().selMan().deSelectAll();
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProgDR();

    OD::ModDeps().ensureLoaded( "visSurvey" );
    ODVsg::DataObject::setVisualizationThread( Threads::currentThread() );
    ODVsg::initStdClasses();
    ODVsg::initSurveyClasses();

    clParser().setKeyHasValue( "datadir" );
    BufferString basedir;
    clParser().getVal( "datadir", basedir );
    const BufferStringSet& survdirnms = survDirNames();
    if ( basedir.isEmpty() ||
	 !File::isDirectory(FilePath(basedir.buf(),survdirnms.first()->buf())
						.fullPath()) )
    {
	logStream() << "No test surveys found (--datadir), test skipped"
		    << od_endl;
	return 0;
    }

    const uiRetVal uirv = IOMan::setDataSource( basedir.buf(),
						survdirnms.first()->str() );
    mRunStandardTestWithError( uirv.isOK(), "Initialize the first project",
			       toString(uirv) )

    SurveyDiskLocation sdl( IOM().rootDir() );
    for ( const auto* survdirnm : survdirnms )
    {
	if ( !File::isDirectory(FilePath(basedir.buf(),survdirnm->buf())
							.fullPath()) )
	{
	    logStream() << "Survey " << survdirnm->buf()
			<< " not found, skipped" << od_endl;
	    continue;
	}

	sdl.setDirName( survdirnm->buf() );
	SurveyChanger changer( sdl );
	if ( !testVirtualCalls() ||
	     !testBackgroundColor() ||
	     !testTransforms(survdirnm->buf()) ||
	     !testAnnotation() ||
	     !testWorkArea() ||
	     !testPicking() )
	    return 1;
    }

    return 0;
}
