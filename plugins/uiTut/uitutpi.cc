/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutorialattrib.h"
#include "uituthortools.h"
#include "uitutseistools.h"
#include "uitutvolproc.h"
#include "uitutwelltools.h"

#include "uihelpview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "viswelldisplay.h"

#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "ptrman.h"
#include "seistype.h"
#include "survinfo.h"

#include "odplugin.h"

static const int cTutIdx = -1100;


mDefODPluginInfo(uiTut)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Tutorial plugin (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Raman/Bert)",
	"=od",
	"User Interface for some simple plugin development basics.\n"
	"Can be loaded into od_main only." ))
    return &retpi;
}



class uiTutMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiTutMgr);
public:
			uiTutMgr();

private:

    uiVisMenuItemHandler wellmnuitmhandler_;

    void		dTectMenuChanged() override;

    void		doSeis(CallBacker*);
    void		do2DSeis(CallBacker*);
    void		do3DSeis(CallBacker*);
    void		launchDialog(Seis::GeomType);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);
};


uiTutMgr::uiTutMgr()
    : uiPluginInitMgr()
    , wellmnuitmhandler_(visSurvey::WellDisplay::sFactoryKeyword(),
		     *appl().applMgr().visServer(),m3Dots(tr("Tut Well Tools")),
		     mCB(this,uiTutMgr,doWells),nullptr,cTutIdx)
{
    init();
}


void uiTutMgr::dTectMenuChanged()
{
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
    const VisID displayid = wellmnuitmhandler_.getDisplayID();
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


mDefODInitPlugin(uiTut)
{
    mDefineStaticLocalObject( PtrMan<uiTutMgr>, theinst_,
		= new uiTutMgr() );

    if ( !theinst_ )
	return "Cannot instantiate the Tutorial plugin";

    uiTutorialAttrib::initClass();
    TutHelpProvider::initClass();
    VolProc::uiTutOpCalculator::initClass();

    return nullptr;
}
