/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.cc,v 1.13 2004-05-28 15:55:02 bert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodmenumgr.cc,v 1.13 2004-05-28 15:55:02 bert Exp $";

#include "uiodmenumgr.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodstdmenu.h"
#include "uicrdevenv.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "helpview.h"
#include "dirlist.h"
#include "pixmap.h"
#include "filegen.h"
#include "filepath.h"
#include "timer.h"


uiODMenuMgr::uiODMenuMgr( uiODMain* a )
	: appl(*a)
    	, timer(*new Timer("popup timer"))
{
    filemnu = new uiPopupMenu( &appl, "&File" );
    procmnu = new uiPopupMenu( &appl, "&Processing" );
    winmnu = new uiPopupMenu( &appl, "&Windows" );
    viewmnu = new uiPopupMenu( &appl, "&View" );
    utilmnu = new uiPopupMenu( &appl, "&Utilities" );
    helpmnu = new uiPopupMenu( &appl, "&Help" );

    dtecttb = appl.toolBar();
    dtecttb->setLabel( "OpendTect tools" );
    cointb = appl.newToolBar( "Graphical tools" );
    mantb = appl.newToolBar( "Manage data" );
}


void uiODMenuMgr::initSceneMgrDepObjs()
{
    uiMenuBar* menu = appl.menuBar();
    fillFileMenu();	menu->insertItem( filemnu );
    fillProcMenu();	menu->insertItem( procmnu );
    fillWinMenu();	menu->insertItem( winmnu );
    fillViewMenu();	menu->insertItem( viewmnu );
    fillUtilMenu();	menu->insertItem( utilmnu );
    fillHelpMenu();	menu->insertItem( helpmnu );

    fillDtectTB();
    fillCoinTB();
    fillManTB();

    timer.tick.notify( mCB(this,uiODMenuMgr,timerCB) );
}


uiPopupMenu* uiODMenuMgr::getBaseMnu( uiODApplMgr::ActType at )
{
    return at == uiODApplMgr::Imp ? impmnu :
	  (at == uiODApplMgr::Exp ? expmnu : manmnu);
}


uiPopupMenu* uiODMenuMgr::getMnu( bool imp, uiODApplMgr::ObjType ot )
{
    return imp ? impmnus[(int)ot] : expmnus[(int)ot];
}


void uiODMenuMgr::storePositions()
{
    dtecttb->storePosition();
    cointb->storePosition();
    mantb->storePosition();
}


void uiODMenuMgr::updateStereoMenu( bool stereo, bool quad )
{
    stereooffitm->setChecked( !stereo );
    stereoredcyanitm->setChecked( stereo && !quad );
    stereoquadbufitm->setChecked( stereo && quad );
    stereooffsetitm->setEnabled( stereo );
}


void uiODMenuMgr::updateViewMode( bool isview )
{
    cointb->turnOn( viewid, isview );
    cointb->turnOn( actid, !isview );
    cointb->setSensitive( axisid, isview );
}


void uiODMenuMgr::updateAxisMode( bool shwaxis )
{
    cointb->turnOn( axisid, shwaxis );
}


void uiODMenuMgr::enableMenuBar( bool yn )
{
    appl.menuBar()->setSensitive( yn );
}


void uiODMenuMgr::enableActButton( bool yn )
{
    cointb->setSensitive( actid, yn );
}


#define mInsertItem(menu,txt,id) \
    menu->insertItem( \
	new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick)), id )

#define mInsertIdealItem(menu,txt,id) \
    { \
	uiMenuItem* subitm = \
		    new uiMenuItem(txt,mCB(this,uiODMenuMgr,handleClick)); \
	menu->insertItem( subitm, id ); subitm->setEnabled( doenable ); \
    }

void uiODMenuMgr::fillFileMenu()
{
    mInsertItem( filemnu, "&Survey ...", mManSurveyMnuItm );

    uiPopupMenu* sessionitm = new uiPopupMenu( &appl, "S&ession");
    mInsertItem( sessionitm, "&Save ...", mSessSaveMnuItm );
    mInsertItem( sessionitm, "&Restore ...", mSessRestMnuItm );
    filemnu->insertItem( sessionitm );
    filemnu->insertSeparator();

    impmnu = new uiPopupMenu( &appl, "&Import" );
    uiPopupMenu* impseis = new uiPopupMenu( &appl, "&Seismics" );
    uiPopupMenu* imphor = new uiPopupMenu( &appl, "&Horizons" );
    uiPopupMenu* impfault = new uiPopupMenu( &appl, "&Faults" );
    uiPopupMenu* impwell = new uiPopupMenu( &appl, "&Wells" );
    impmnu->insertItem( impseis );
    mInsertItem( impmnu, "&Picksets ...", mImpPickMnuItm );
    impmnu->insertItem( imphor );
#ifdef __debug__
    impmnu->insertItem( impfault );
    mInsertItem( impfault, "&Landmark ...", mImpLmkFaultMnuItm );
#endif
    impmnu->insertItem( impwell );
    mInsertItem( impseis, "SEG-Y ...", mImpSeisSEGYMnuItm );
    mInsertItem( impseis, "CBVS ...", mImpSeisCBVSMnuItm );
    mInsertItem( imphor, "&Ascii ...", mImpHorAsciiMnuItm );
    uiPopupMenu* impwellasc = new uiPopupMenu( &appl, "&Ascii" );
    mInsertItem( impwellasc, "&Track ...", mImpWellAsciiTrackMnuItm );
    mInsertItem( impwellasc, "&Logs ...", mImpWellAsciiLogsMnuItm );
    mInsertItem( impwellasc, "&Markers ...", mImpWellAsciiMarkersMnuItm );
    impwell->insertItem( impwellasc );

    expmnu = new uiPopupMenu( &appl, "&Export" );
    uiPopupMenu* expseis = new uiPopupMenu( &appl, "&Seismics" );
    uiPopupMenu* exphor = new uiPopupMenu( &appl, "&Horizons" );
    expmnu->insertItem( expseis ); expmnu->insertItem( exphor );
    mInsertItem( expseis, "SEG-Y ...", mExpSeisSEGYMnuItm );
    mInsertItem( exphor, "&Ascii ...", mExpHorAsciiMnuItm );

    filemnu->insertItem( impmnu );
    filemnu->insertItem( expmnu );
    manmnu = new uiPopupMenu( &appl, "&Manage");
    mInsertItem( manmnu, "&Seismics ...", mManSeisMnuItm );
    mInsertItem( manmnu, "&Horizons ...", mManHorMnuItm );
#ifdef __debug__
    mInsertItem( manmnu, "&Faults ...", mManFaultMnuItm );
#endif
    mInsertItem( manmnu, "&Wells ...", mManWellMnuItm );
    filemnu->insertItem( manmnu );

    filemnu->insertSeparator();
    mInsertItem( filemnu, "E&xit", mExitMnuItm );

    impmnus.allowNull();
    impmnus += impseis; impmnus += imphor; impmnus += impfault; 
    impmnus += impwell; impmnus+= 0;
    expmnus.allowNull();
    expmnus += expseis; expmnus += exphor; expmnus+=0; expmnus+=0; expmnus+=0;
}


void uiODMenuMgr::fillProcMenu()
{
    mInsertItem( procmnu, "&Attributes ...", mManAttribsMnuItm );
    procmnu->insertSeparator();
    mInsertItem( procmnu, "Create &Volume ...", mCreateVolMnuItm );
    mInsertItem( procmnu, "&Re-Start ...", mReStartMnuItm );
}


void uiODMenuMgr::fillWinMenu()
{
    mInsertItem( winmnu, "&New", mAddSceneMnuItm );
    mInsertItem( winmnu, "&Cascade", mCascadeMnuItm );
    mInsertItem( winmnu, "&Tile", mTileMnuItm );
}


void uiODMenuMgr::fillViewMenu()
{
    mInsertItem( viewmnu, "&Work area ...", mWorkAreaMnuItm );
    mInsertItem( viewmnu, "&Z-scale ...", mZScaleMnuItm );
    uiPopupMenu* stereoitm = new uiPopupMenu( &appl, "&Stereo viewing" );
    viewmnu->insertItem( stereoitm );

#define mInsertStereoItem(itm,txt,docheck,id,idx) \
    itm = new uiMenuItem( txt, mCB(this,uiODMenuMgr,handleClick) ); \
    stereoitm->insertItem( itm, id, idx ); \
    itm->setChecked( docheck );

    mInsertStereoItem( stereooffitm, "&Off", true, mStereoOffMnuItm, 0 )
    mInsertStereoItem( stereoredcyanitm, "&Red/Cyan", false,
	    		mStereoRCMnuItm, 1 )
    mInsertStereoItem( stereoquadbufitm, "&Quad buffer", false,
	    		mStereoQuadMnuItm, 2 )

    stereooffsetitm = new uiMenuItem( "&Stereo offset ...",
				mCB(this,uiODMenuMgr,handleClick) );
    stereoitm->insertItem( stereooffsetitm, mStereoOffsetMnuItm, 3 );
    stereooffsetitm->setEnabled( false );
}


void uiODMenuMgr::fillUtilMenu()
{
    settmnu = new uiPopupMenu( &appl, "&Settings" );
    utilmnu->insertItem( settmnu );
    mInsertItem( settmnu, "&Fonts ...", mSettFontsMnuItm );
    mInsertItem( settmnu, "&Mouse controls ...", mSettMouseMnuItm );

    mInsertItem( utilmnu, "&Batch programs ...", mBatchProgMnuItm );
    mInsertItem( utilmnu, "&Plugins ...", mPluginsMnuItm );
    mInsertItem( utilmnu, "&Create Devel. Env. ...", mCrDevEnvMnuItm );
}


void uiODMenuMgr::fillHelpMenu()
{
    DirList dl( GetDataFileName(0), DirList::DirsOnly, "*Doc" );
    bool havedtectdoc = false;
    for ( int hidx=0, idx=0; idx<dl.size(); idx++ )
    {
	BufferString dirnm = dl.get( idx );
	if ( dirnm == "dTectDoc" )
	{
	    havedtectdoc = true;
	    mInsertItem( helpmnu, "&Index ...", mODIndexMnuItm );
	}
	else
	{
	    char* ptr = strstr( dirnm.buf(), "Doc" );
	    if ( !ptr ) continue; // Huh?
	    *ptr = '\0';
	    if ( dirnm == "" ) continue;

	    BufferString itmnm = "&"; // hope there's no duplication
	    itmnm += dirnm; itmnm += "-"; itmnm += "Index ...";
	    mInsertItem( helpmnu, itmnm, mStdHelpMnuBase + hidx + 1 );
	    hidx++;
	}
    }
    if ( havedtectdoc )
    {
	mInsertItem( helpmnu, "Ad&min ...", mAdminMnuItm );
	mInsertItem( helpmnu, "&Programmer ...", mProgrammerMnuItm );
    }
    mInsertItem( helpmnu, "&About ...", mAboutMnuItm );
}


#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( ioPixmap( GetDataFileName(fnm) ), \
	    	   mCB(&applMgr(),uiODApplMgr,fn), txt, togg )

void uiODMenuMgr::fillDtectTB()
{
    mAddTB(dtecttb,"survey.png","Survey setup",false,manSurvCB);
    mAddTB(dtecttb,"attributes.png","Edit attributes",false,manAttrCB);
    mAddTB(dtecttb,"out_vol.png","Output volume",false,outVolCB);
}

#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( ioPixmap( GetDataFileName(fnm) ), \
	    	   mCB(this,uiODMenuMgr,fn), txt, togg )


void uiODMenuMgr::fillManTB()
{
    mAddTB(mantb,"man_seis.png","Manage seismic data",false,manSeis);
    mAddTB(mantb,"man_hor.png","Manage horizons",false,manHor);
#ifdef __debug__
    mAddTB(mantb,"man_flt.png","Manage faults",false,manFlt);
#endif
    mAddTB(mantb,"man_wll.png","Manage well data",false,manWll);
}


#undef mAddTB
#define mAddTB(tb,fnm,txt,togg,fn) \
    tb->addButton( ioPixmap( GetDataFileName(fnm) ), \
	    	   mCB(&sceneMgr(),uiODSceneMgr,fn), txt, togg )

void uiODMenuMgr::fillCoinTB()
{
    viewid = mAddTB(cointb,"view.xpm","View mode",true,viewMode);
    actid = mAddTB(cointb,"pick.xpm","Interact mode",true,actMode);
    mAddTB(cointb,"home.xpm","To home position",false,toHomePos);
    mAddTB(cointb,"set_home.xpm","Save home position",false,saveHomePos);
    mAddTB(cointb,"view_all.xpm","View all",false,viewAll);
    mAddTB(cointb,"align.xpm","Align",false,align);
    axisid = mAddTB(cointb,"axis.xpm","Display rotation axis",true,showRotAxis);

    cointb->turnOn( actid, true );
}


static const char* getHelpF( const char* subdir, const char* infnm,
			     const char* basedirnm = "dTectDoc" )
{
    FilePath fp( basedirnm );
    if ( subdir && *subdir ) fp.add( subdir );
    fp.add( infnm );
    static BufferString fnm;
    fnm = fp.fullPath();
    return fnm.buf();
}


#define mDoOp(ot,at,op) \
	applMgr().doOperation(uiODApplMgr::at,uiODApplMgr::ot,op)

void uiODMenuMgr::handleClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return; // Huh?

    const int id = itm->id();
    switch( id )
    {
    case mManSurveyMnuItm: 	applMgr().manageSurvey(); break;
    case mSessSaveMnuItm: 	appl.saveSession(); break;
    case mSessRestMnuItm: 	{ appl.restoreSession(); 
				  timer.start(200,true);  break; }
    case mImpSeisSEGYMnuItm:	mDoOp(Imp,Seis,0); break;
    case mImpSeisCBVSMnuItm: 	mDoOp(Imp,Seis,1); break;
    case mExpSeisSEGYMnuItm: 	mDoOp(Exp,Seis,0); break;
    case mImpHorAsciiMnuItm: 	mDoOp(Imp,Hor,0); break;
    case mExpHorAsciiMnuItm: 	mDoOp(Exp,Hor,0); break;
    case mImpWellAsciiTrackMnuItm:  mDoOp(Imp,Wll,0); break;
    case mImpWellAsciiLogsMnuItm:  mDoOp(Imp,Wll,1); break;
    case mImpWellAsciiMarkersMnuItm:  mDoOp(Imp,Wll,2); break;
    case mManSeisMnuItm: 	mDoOp(Man,Seis,0); break;
    case mManHorMnuItm: 	mDoOp(Man,Hor,0); break;
    case mManFaultMnuItm: 	mDoOp(Man,Flt,0); break;
    case mManWellMnuItm: 	mDoOp(Man,Wll,0); break;
    case mImpPickMnuItm: 	applMgr().importPickSet(); break;
    case mImpLmkFaultMnuItm: 	applMgr().importLMKFault(); break;
    case mExitMnuItm: 		appl.exit(); break;
    case mManAttribsMnuItm: 	applMgr().manageAttributes(); break;
    case mCreateVolMnuItm: 	applMgr().createVol(); break;
    case mReStartMnuItm: 	applMgr().reStartProc(); break;
    case mAddSceneMnuItm: 		
				sceneMgr().tile(); // otherwise crash ...!
				sceneMgr().addScene(); break;
    case mCascadeMnuItm: 	sceneMgr().cascade(); break;
    case mTileMnuItm: 		sceneMgr().tile(); break;
    case mWorkAreaMnuItm: 	applMgr().setWorkingArea(); break;
    case mZScaleMnuItm: 	applMgr().setZScale(); break;
    case mAdminMnuItm: 		HelpViewer::doHelp(
				    getHelpF("ApplMan","index.html"),
				    "OpendTect System administrator"); break;
    case mProgrammerMnuItm:	HelpViewer::doHelp(
					getHelpF("Programmer","index.html"),
					"d-Tect" ); break;
    case mBatchProgMnuItm: 	applMgr().batchProgs(); break;
    case mPluginsMnuItm: 	applMgr().pluginMan(); break;
    case mCrDevEnvMnuItm: 	uiCrDevEnv::crDevEnv(&appl); break;
    case mSettFontsMnuItm: 	applMgr().setFonts(); break;
    case mSettMouseMnuItm: 	sceneMgr().setKeyBindings(); break;

    case mStereoOffsetMnuItm: 	applMgr().setStereoOffset(); break;
    case mStereoOffMnuItm: 
    case mStereoRCMnuItm : 
    case mStereoQuadMnuItm :
    {
	int idx = itm->index();
	bool stereo = (idx == 1 || idx == 2) && !itm->isChecked();
	bool quad = idx == 2 && !itm->isChecked();
	sceneMgr().setStereoViewing( stereo, quad );
	updateStereoMenu( stereo, quad );
    } break;

    case mAboutMnuItm:
    {
	const char* htmlfnm = "about.html";
	const BufferString dddirnm = GetDataFileName("dTectDoc");
	BufferString fnm = FilePath(dddirnm).add(htmlfnm).fullPath();
	fnm = File_exists(fnm) ? getHelpF(0,htmlfnm) : htmlfnm;
	HelpViewer::doHelp( fnm, "About OpendTect" );
    } break;

    default:
    {
	if ( id < mStdHelpMnuBase || id > mStdHelpMnuBase + 90 ) return;

	BufferString itmnm = itm->name();
	BufferString docnm = "dTect";
	char* ptr = strchr( itmnm.buf(),'-' );
	if ( ptr )
	{
	    *ptr = '\0';
	    docnm = itmnm.buf() + 1; // add one to skip "&"
	}

	BufferString dirnm( docnm ); dirnm += "Doc";
	itmnm = "OpendTect Documentation";
	if ( ptr )
	    { itmnm += " - "; itmnm += docnm; itmnm += " part"; }

	HelpViewer::doHelp( getHelpF(0,"index.html",dirnm.buf()), itmnm );
	break;

    } break;

    }
}

#define mDefManCBFn(typ) \
    void uiODMenuMgr::man##typ( CallBacker* ) { mDoOp(Man,typ,0); }

mDefManCBFn(Seis)
mDefManCBFn(Hor)
mDefManCBFn(Flt)
mDefManCBFn(Wll)


void uiODMenuMgr::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}
