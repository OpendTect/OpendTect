
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"
#include "uiseisfileman.h"

#include "uisegydirectinserter.h"
#include "uisegydirectinserter.h"

#include "odplugin.h"


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"SEG-Y support",
	"OpendTect",
	"dGB (Bert)",
	"1.0",
    	"Supports the SEG-Y format") )
    return &retpi;
}


class uiSEGYMgr	: public CallBacker
{ mODTextTranslationClass(uiSEGYMgr);
public:

			uiSEGYMgr(uiODMain&);

    uiODMain&		appl_;
    uiODMenuMgr&	mnumgr;

    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);

    IOPar		toolpar_;

    static uiSEGYMgr*		theinst_;
    static const uiString	sSEGYString( bool imm )
    				{ return imm ? tr("SEG-Y") : tr("SEG-Y ..."); }

};

uiSEGYMgr::theinst_ = 0;


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)


uiSEGYMgr::uiSEGYMgr( uiODMain& a )
    : mnumgr(a.menuMgr())
    , ishidden_(false)
    , appl_(a)
{
    uiSEGYDirectVolOpts::initClass();
    uiSEGYDirectPS3DOpts::initClass();
    uiSEGYDirectVolInserter::initClass();
    uiSEGYDirect2DInserter::initClass();
    uiSEGYDirectPS3DInserter::initClass();

    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef(
	    			SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = uiString( "Change file/directory names in SEG-Y file" );
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider();
    uiSurveyInfoEditor::addInfoProvider( sip );

    mnumgr.dTectTBChanged.notify( muiSEGYMgrCB(updateToolBar) );
    mnumgr.dTectMnuChanged.notify( muiSEGYMgrCB(updateMenu) );
    IOM().surveyToBeChanged.notify( muiSEGYMgrCB(survChg) );

    updateToolBar(0);
    updateMenu(0);
}


void uiSEGYMgr::updateToolBar( CallBacker* )
{
    mnumgr.dtectTB()->addButton( "segy", tr("SEG-Y import"),
	    			 mCB(this,uiSEGYMgr,doImp) );
}


void uiSEGYMgr::updateMenu( CallBacker* )
{
    uiAction* impact = new uiAction( sSEGYString(false),
	    			     muiSEGYMgrCB(genImpCB), "segy" );
    uiAction* linkact = new uiAction( tr("SEG-Y Data Link ..."),
				      muiSEGYMgrCB(linkImpCB), "segy_link" );

    uiMenu* impseismnu = mnumgr.getMnu( true, uiODApplMgr::Seis );
    impseismnu->insertItem( impact );
    impseismnu->insertItem( linkact );

    const bool have2d = SI().has2D(); const bool only2d = !SI().has3D();
    uiMenu* expseismnu = *mnumgr.getMnu( false, uiODApplMgr::Seis );
    uiMenu* expsgymnu = !only2d ? new uiMenu( &appl_, sSEGYString(true), "segy")
				: expseismnu;
    if ( expsgymnu != expseismnu )
	expseismnu->insertItem( expsgymnu );

    if ( have2d )
	expsgymnu->insertItem( new uiAction( only2d ? sSEGYString(false)
		    				    : uiStrings::s2D(false),
				muiSEGYMgrCB(exp2DCB), "" );
    if ( !only2d )
    {
        expsgymnu->insertItem( new uiAction( have2d ? uiStrings::s3D(false)
		    				    : tr("Cube"),
				muiSEGYMgrCB(exp3DCB), "" );
        expsgymnu->insertItem( new uiAction( have2d ? tr("PreStack 3D")
		    				    : tr("Pre-Stack volume"),
				muiSEGYMgrCB(exp3DPSCB), "" );
    }

    mnumgr.getMnu( true, uiODApplMgr::Wll )->insertItem(
	new uiAction( tr("VSP (SEG-Y) ..."), muiSEGYMgrCB(expVSPCB), "" );
    mnumgr.crSeisOutMnu()->insertItem(
	new uiAction(tr("Re-sort Scanned SEG-Y ..."), muiSEGYMgrCB(reSortCB)) );
}

void uiSEGYMgr::genImpCB( CallBacker* )
{
}


void uiSEGYMgr::linkImpCB( CallBacker* )
{
}


void uiSEGYMgr::exp2DCB( CallBacker* )
{
}


void uiSEGYMgr::exp3DCB( CallBacker* )
{
}


void uiSEGYMgr::exp3DPSCB( CallBacker* )
{
}


void uiSEGYMgr::expVSPCB( CallBacker* )
{
}


void uiSEGYMgr::survChg( CallBacker* )
{
    toolpar_.setEmpty();
}


void uiSEGYMgr::reSortCB( CallBacker* )
{
}


void uiSEGYMgr::edFiles( CallBacker* cb )
{
    mDynamicCastGet(uiSeisFileMan*,sfm,cb)
    if ( !sfm || !sfm->curIOObj() )
	return;

    uiEditSEGYFileDataDlg dlg( sfm, *sfm->curIOObj() );
    dlg.go();
}


mDefODInitPlugin(uiSEGY)
{
    if ( !theinst_ )
	theinst_ = new uiSEGYMgr( *ODMainWin() );
    return 0;
}
