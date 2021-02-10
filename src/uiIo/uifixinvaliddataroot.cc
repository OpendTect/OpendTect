/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra / Bert
 Date:          June 2002 / Oct 2016
________________________________________________________________________

-*/

#include "uifixinvaliddataroot.h"

#include "uidatarootsel.h"
#include "uiselsimple.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uifileselector.h"
#include "uimain.h"
#include "uimsg.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odinst.h"
#include "ziputils.h"
#include "od_helpids.h"
#include "genc.h"


uiFixInvalidDataRoot::uiFixInvalidDataRoot( uiParent* p )
	: uiDialog(p,uiDialog::Setup(tr("Set Data Directory"),
				     tr("Specify a data storage directory"),
				     mODHelpKey(mSetDataDirHelpID) ))
{
    const BufferString curdatadir = GetBaseDataDir();
    uiString titletxt;
    if ( !curdatadir.isEmpty() )
	titletxt = tr("The current %1 is invalid.\n"
		    "\nPlease specify a valid %1\n");
    else
    {
	titletxt =
	    tr("OpendTect needs a place to store your data files: the %1.\n\n"
	    "You have not yet specified a location,\n"
	    "and there is no 'DTECT_DATA' set in your environment.\n\n"
	    "Please specify where the %1 should\n"
	    "be created or select an existing %1.\n");
	uiString addstr = tr(
	    "\nNote that you can still put surveys and "
	    "individual cubes on other disks;\nbut this is where the "
	    "'base' data store will be.");
#ifndef __win__
	titletxt.append( addstr, true );
#endif
    }
    titletxt.arg( uiDataRootSel::userDataRootString() );
    setTitleText( titletxt );

    dirfld_ = new uiDataRootSel( this, curdatadir );
}


bool uiFixInvalidDataRoot::rejectOK()
{
    uiString msg = tr("OpendTect cannot start without a valid data root "
		      "directory.\n\nExit OpendTect?");
    if ( uiMSG().askGoOn(msg) )
	uiMain::theMain().exit();

    return false;
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiFixInvalidDataRoot::acceptOK()
{
    BufferString chosendir = dirfld_->getDir();
    if ( chosendir.isEmpty() )
	return false; // errmsg done by dirfld

    if ( !uiDataRootSel::setRootDirOnly(chosendir) )
	return false;

    BufferStringSet survnms;
    uiSurvey::getDirectoryNames( survnms, false, chosendir );
    if ( survnms.isEmpty() )
	offerCreateSomeSurveys( chosendir );

    return true;
}


static BufferString getInstalledDemoSurvey()
{
    BufferString ret;
    if ( ODInst::getPkgVersion("demosurvey") )
    {
	File::Path demosurvfp( mGetSWDirDataDir(), "DemoSurveys",
			     "F3_Start.zip" );
	ret = demosurvfp.fullPath();
    }
    if ( !File::exists(ret) )
	ret.setEmpty();
    return ret;
}


void uiFixInvalidDataRoot::offerCreateSomeSurveys( const char* datadir )
{
    BufferString zipfilenm = getInstalledDemoSurvey();
    const bool havedemosurv = !zipfilenm.isEmpty();
    uiStringSet opts;
    opts.add( tr("I will set up a new survey myself") );
    if ( havedemosurv )
	opts.add( tr("Install the F3 Demo Survey from the "
			"OpendTect installation") );
    opts.add( tr("Unzip a survey zip file") );

    struct OSRPageShower : public CallBacker
    {
	void go( CallBacker* )
	    { uiDesktopServices::openUrl( "https://opendtect.org/osr" ); }
    };
    uiGetChoice uigc( this, opts, uiStrings::phrSelect(tr("next action")) );
    OSRPageShower ps;
    uiPushButton* pb = new uiPushButton( &uigc,
				 tr("visit OSR web site (for free surveys)"),
				 mCB(&ps,OSRPageShower,go), true );
    pb->attach( rightAlignedBelow, uigc.bottomFld() );
    if ( !uigc.go() || uigc.choice() == 0 )
	return;

    if ( (havedemosurv && uigc.choice() == 2) ||
         (!havedemosurv && uigc.choice() == 1))
    {
	uiFileSelector::Setup fssu;
	fssu.setFormat( File::Format::zipFiles() )
	    .initialselectiondir( datadir );
        uiFileSelector uifs( this, fssu );
        if ( !uifs.go() )
            return;

        zipfilenm = uifs.fileName();
    }

    (void)uiSurvey::unzipFile( this, zipfilenm, datadir );
}
