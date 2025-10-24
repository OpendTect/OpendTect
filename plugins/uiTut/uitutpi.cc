/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutexternalattribrandom.h"
#include "uituthortools.h"
#include "uitutorialattrib.h"
#include "uitutseistools.h"
#if __has_include("uitutversion.h")
# include "uitutversion.h"
#endif
#include "uitutvolproc.h"
#include "uitutwelltools.h"

#include "uihelpview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"

#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "odplugin.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "viswelldisplay.h"


static const int cTutIdx = -1100;

static const char* getProductName()
{
#ifdef uiTut_PRODUCT_NAME
    return uiTut_PRODUCT_NAME;
#else
    return "OpendTect";
#endif
}

static const char* getCreatorNm()
{
#ifdef Vendor
    return Vendor;
#else
    return "dGB Earth Sciences";
#endif
}

static const char* getVersion()
{
#ifdef uiTut_VERSION
    return uiTut_VERSION;
#else
    return "=od";
#endif
}

static const char* dispName()	    { return "Tutorial plugin (GUI)"; }

static const char* dispText()
{
    return "User Interface for some simple plugin development basics.\n"
	   "Can be loaded into od_main only.";
}

static PluginInfo::LicenseType getLicType()
{
#ifdef Vendor
    return PluginInfo::LicenseType::COMMERCIAL;
#else
    return PluginInfo::LicenseType::GPL;
#endif
}


class TutHelpProvider : public SimpleHelpProvider
{
public:
TutHelpProvider( const char* baseurl, const char* linkfnm )
    : SimpleHelpProvider(baseurl,linkfnm)
{}

static void initClass()
{
    HelpProvider::factory().addCreator( TutHelpProvider::createInstance, "tut");
}

static HelpProvider* createInstance()
{
    FilePath fp( GetDocFileDir(""), "User", "tut" );
    BufferString baseurl( "file:///" );
    baseurl.add( fp.fullPath() ).add( "/" );
    fp.add( "KeyLinkTable.txt" );
    BufferString tablefnm = fp.fullPath();
    return new TutHelpProvider( baseurl.buf(), tablefnm.buf() );
}

};


mDefODPluginInfo(uiTut)
{
    static PluginInfo retpi(
	dispName(),
	getProductName(),
	getCreatorNm(),
	getVersion(),
	dispText(),
	getLicType() );
    return &retpi;
}



class uiTutMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiTutMgr);
public:
			uiTutMgr();
			~uiTutMgr();

private:
    void		init() override;
    void		dTectMenuChanged() override;

    void		createVisObjMenu(CallBacker*);
    void		handleVisObjMenu(CallBacker*);
    void		doSeis(CallBacker*);
    void		do2DSeis(CallBacker*);
    void		do3DSeis(CallBacker*);
    void		launchDialog(Seis::GeomType);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);

    uiVisMenuItemHandler* wellmnuitmhandler_ = nullptr;
    MenuItem		addrandomattribmnuitem_;
};


uiTutMgr::uiTutMgr()
    : uiPluginInitMgr()
    , addrandomattribmnuitem_(ExternalAttrib::Random::sFactoryDisplayName())
{
    init();
    addrandomattribmnuitem_.iconfnm = "random_color";
    uiVisPartServer* visserv = appl().applMgr().visServer();
    if ( !visserv )
	return;

    wellmnuitmhandler_ = new uiVisMenuItemHandler(
			visSurvey::WellDisplay::sFactoryKeyword(),
			*visserv, m3Dots(tr("Tut Well Tools")),
			mCB(this,uiTutMgr,doWells), nullptr, cTutIdx );
    mAttachCB( visserv->getMenuHandler()->createnotifier,
	       uiTutMgr::createVisObjMenu );
    mAttachCB( visserv->getMenuHandler()->handlenotifier,
	       uiTutMgr::handleVisObjMenu );
}


uiTutMgr::~uiTutMgr()
{
    detachAllNotifiers();
    delete wellmnuitmhandler_;
}


void uiTutMgr::init()
{
    uiPluginInitMgr::init();

    uiTutorialAttrib::initClass();
    TutHelpProvider::initClass();
    VolProc::uiTutOpCalculator::initClass();
    ExternalAttrib::uiRandomTreeItem::initClass();
}


void uiTutMgr::dTectMenuChanged()
{
    if ( !ODMainWin() )
	return;

    auto* mnu = new uiMenu( &appl(), tr( "Tut Tools" ) );
    if ( SI().has2D() && SI().has3D() )
    {
	mnu->insertAction( new uiAction( m3Dots( tr( "Seismic 2D (Direct)" ) ),
	    mCB( this, uiTutMgr, do2DSeis ) ) );
	mnu->insertAction( new uiAction( m3Dots( tr( "Seismic 3D (Direct)" ) ),
	    mCB( this, uiTutMgr, do3DSeis ) ) );
    }
    else
	mnu->insertAction( new uiAction( m3Dots( tr( "Seismic (Direct)" ) ),
	    mCB( this, uiTutMgr, doSeis ) ) );

    mnu->insertAction( new uiAction( m3Dots( uiStrings::sHorizon( 1 ) ),
	mCB( this, uiTutMgr, doHor ) ) );

    appl().menuMgr().toolsMnu()->addMenu( mnu );
}


void uiTutMgr::createVisObjMenu( CallBacker* cb )
{
    uiVisPartServer* visserv = appl().applMgr().visServer();
    if ( !visserv || !cb )
	return;

    mDynamicCastGet(MenuHandler*,menu,cb)
    const VisID displayid( menu->menuID() );
    const visBase::DataObject* dataobj = visserv->getObject( displayid );
    mDynamicCastGet(const visSurvey::SurveyObject*,so,dataobj);
    if ( !so || !so->canHaveMultipleAttribs() ||
	 (so->getAttributeFormat()!=visSurvey::SurveyObject::Cube &&
	  so->getAttributeFormat()!=visSurvey::SurveyObject::Traces) )
    {
	mResetMenuItem( &addrandomattribmnuitem_ );
	return;
    }

    const visSurvey::PlaneDataDisplay* pdd = nullptr;
    const visSurvey::RandomTrackDisplay* rtd = nullptr;
    const visSurvey::Seis2DDisplay* s2dd = nullptr;
    if ( SI().has2D() )
	mDynamicCast(const visSurvey::Seis2DDisplay*,s2dd,so);

    if ( SI().has3D() )
    {
	mDynamicCast(const visSurvey::PlaneDataDisplay*,pdd,so)
	mDynamicCast(const visSurvey::RandomTrackDisplay*,rtd,so)
    }

    if ( !pdd && !rtd && !s2dd )
    {
	mResetMenuItem( &addrandomattribmnuitem_ );
	return;
    }

    MenuItem* additem = menu->findItem( "Add" );
    if ( additem )
    {
	const bool islocked = visserv->isLocked( displayid );
	const bool canaddattrib = visserv->canAddAttrib( displayid );
	const bool enabmnu = !islocked && canaddattrib;
	mAddMenuItem( additem, &addrandomattribmnuitem_, enabmnu, false )
    }
    else
	mResetMenuItem( &addrandomattribmnuitem_ )
}


void uiTutMgr::handleVisObjMenu( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    uiVisPartServer* visserv = appl().applMgr().visServer();
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !visserv || !menu || menu->isHandled() || mnuid==-1 )
	return;

    const VisID displayid( menu->menuID() );
    uiTreeItem* parent = appl().sceneMgr().findItem( displayid );
    if ( !parent || mnuid != addrandomattribmnuitem_.id )
	return;

    menu->setIsHandled( true );
    const int attrib = visserv->addAttrib( displayid );
    const BufferString defstr = ExternalAttrib::Random::createDefinition();
    const uiString userref = ExternalAttrib::Random::createDisplayName();
    Attrib::SelSpec spec( userref.getFullString(),
			  Attrib::SelSpec::cOtherAttrib() );
    spec.setDefString( defstr.buf() );
    visserv->setSelSpec( displayid, attrib, spec );

    auto* newitem =
		new ExternalAttrib::uiRandomTreeItem( typeid(*parent).name() );
    parent->addChild( newitem, false );
    appl().applMgr().getNewData( newitem->displayID(), newitem->attribNr() );
}


void uiTutMgr::do3DSeis( CallBacker* )
{
    launchDialog( Seis::Vol );
}


void uiTutMgr::do2DSeis( CallBacker* )
{
    launchDialog( Seis::Line );
}

void uiTutMgr::doSeis( CallBacker* )
{
    launchDialog( SI().has2D() ? Seis::Line : Seis::Vol );
}


void uiTutMgr::launchDialog( Seis::GeomType tp )
{
    uiTutSeisTools dlg( &appl(), tp );
    dlg.go();
}


void uiTutMgr::doHor( CallBacker* )
{
    uiTutHorTools dlg( &appl() );
    dlg.go();
}


void uiTutMgr::doWells( CallBacker* )
{
    const VisID displayid = wellmnuitmhandler_->getDisplayID();
    mDynamicCastGet(visSurvey::WellDisplay*,wd,
			appl().applMgr().visServer()->getObject(displayid))
    if ( !wd )
	return;

    const MultiID wellid = wd->getMultiID();
    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot find well in database.\n"
		          "Perhaps it's not stored yet?") );
	return;
    }

    uiTutWellTools dlg( &appl(), wellid );
    dlg.go();
}


mDefODInitPlugin(uiTut)
{
    static PtrMan<uiTutMgr> pimgr = new uiTutMgr();
    if ( !pimgr )
	return "Cannot instantiate the Tutorial plugin";

    return nullptr;
}
