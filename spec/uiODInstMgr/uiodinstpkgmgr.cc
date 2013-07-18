/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uiodinstpkgmgr.cc 7996 2013-06-13 12:13:14Z ranojay.sen@dgbes.com $";

#include "uiodinstpkgmgr.h"

#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "odinstpkgprops.h"
#include "odinstappdata.h"
#include "odinsthtmlcomposer.h"
#include "odinstpkgselmgr.h"
#include "odinstinstallhandler.h"
#include "odinstlogger.h"
#include "odinstwinutils.h"
#include "odinstziphandler.h"
#include "oddlsite.h"
#include "strmprov.h"
#include "separstr.h"
#include "strmoper.h"
#include "timer.h"
#include "ziputils.h"
#include "sorting.h"
#include "odlogo32x32.xpm"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uiprogressbar.h"
#include "uisplitter.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitextfile.h"
#include "uidesktopservices.h"

static const char* unix_script_filename = "unix_install.csh";


uiODInstPkgMgr::uiODInstPkgMgr( uiParent* p, ODInst::AppData& ad,
		    ODInst::DLHandler& dlh, const ODInst::PkgGroupSet& pgs,
       		    const ODInst::Platform& plf, 
		    const ODInst::RelData& rd, int choice, bool isoonln )
    : uiODInstDlg(p,uiDialog::Setup("OpendTect Package Manager","","0.5.2")
	    		.nrstatusflds(1).menubar(true),&dlh)
    , appdata_(ad)
    , reldata_(rd)
    , isnewinst_(appdata_.isNewInst())
    , pkggrps_(pgs)
    , htmlcomp_(*new ODInst::HtmlComposer)
    , curpkg_( 0 )
    , platform_(plf)
    , autoupdate_(false)
    , packagechoice_(choice)
    , mainselfld_( 0 )
    , isviewingdetail_( false )
    , ishtmlon_(false)
    , closewintimer_(0)
    , isoonline_(isoonln)
{
    pkgselmgr_ = new ODInst::PkgSelMgr( appdata_, pkggrps_, platform_ );
    insthandler_ = new ODInst::InstallHandler(appdata_,rd,platform_,
						*pkgselmgr_,dlh);
    insthandler_->status.notify( mCB(this,uiODInstPkgMgr,statusMsgCB) );

    uiLabel* titlelbl = mkTitleStuff();

    pkglblfld_ = new uiComboBox( this, "Package labels" );
    pkglblfld_->addItem( "All" );
    pkglblfld_->addItems( getPkgLabels() );
    pkglblfld_->selectionChanged.notify( mCB(this,uiODInstPkgMgr,grpSelChg) );
    pkglblfld_->attach( alignedBelow, titlelbl );
    pkglblfld_->setHSzPol( uiObject::WideVar  );

  /*  switchviewbut_ = new uiPushButton( this, "Switch to Classic view",
	    			mCB(this,uiODInstPkgMgr,switchViewCB), true );
    switchviewbut_->attach( rightTo, pkglblfld_ );
    switchviewbut_->attach( rightBorder );*/

    classicgrp_ = new uiGroup( this, "Classic group" );
    //htmlgrp_ = new uiGroup( this, "HTML group" );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedAbove, classicgrp_ );
    //htmlgrp_->attach( ensureBelow, sep );
    pkglblfld_->attach( leftAlignedAbove, classicgrp_ );
    sep->attach( ensureBelow, pkglblfld_ );
    //sep->attach( ensureBelow, switchviewbut_ );

    mkClassicGrp();
    //mkHTMLGrp();

    finaliseDone.notify( mCB(this,uiODInstPkgMgr,initWin) );
}


uiODInstPkgMgr::~uiODInstPkgMgr()
{
    delete &htmlcomp_;
    delete pkgselmgr_;
    delete insthandler_;
    delete closewintimer_;
    cleanUp();
}


uiLabel* uiODInstPkgMgr::mkTitleStuff()
{
    BufferString dlgttl( reldata_.name_, " ", reldata_.version_.dirName(false) );
    dlgttl.add( " at " ).add( appdata_.baseDirName() );
    uiLabel* titlelbl = new uiLabel(  this, dlgttl );
    titlelbl->attach( leftBorder );

    uiLabel* instmgrlbl = new uiLabel( this, "Installation Manager" );
    instmgrlbl->attach( rightBorder );

    uiLabel* pixmaplbl = new uiLabel( this, 0 );
    pixmaplbl->setPrefHeight( 40 );
    pixmaplbl->setPrefWidth( 40 );
    pixmaplbl->setPixmap( ioPixmap(od_logo_32x32) );
    pixmaplbl->attach( leftOf, instmgrlbl );

    return titlelbl;
}


void uiODInstPkgMgr::mkClassicGrp()
{
    uiGroup* leftgrp = new uiGroup( classicgrp_, "Left group" );
    uiGroup* rightgrp = new uiGroup( classicgrp_, "Right group" );
    uiSplitter* splitter = new uiSplitter( classicgrp_ );
    splitter->addGroup( leftgrp ); splitter->addGroup( rightgrp );

    pkgsfld_ = new uiListBox( leftgrp, "Packages", false, 15 );
    pkgsfld_->setItemsCheckable( true );
    pkgsfld_->selectionChanged.notify( mCB(this,uiODInstPkgMgr,pkgSelChg) );
    pkgsfld_->itemChecked.notify( mCB(this,uiODInstPkgMgr,itmChck) );

    descfld_ = new uiTextBrowser( rightgrp, "Desc", 5, false );
    descfld_->setLinkBehavior( uiTextBrowser::FollowAll );
    descfld_->setPrefHeightInChar( 12 );
    descfld_->setLinkBehavior( uiTextBrowser::FollowAll );

    creatorfld_ = new uiGenInput( rightgrp, "Creator" );
    creatorfld_->attach( ensureBelow, descfld_ );
    creatorurlbut_ = new uiPushButton( rightgrp, "&Web", true );
    creatorurlbut_->activated.notify( mCB(this,uiODInstPkgMgr,creatorWebPush) );
    creatorurlbut_->attach( rightOf, creatorfld_ );

    filelistbut_ = new uiPushButton( rightgrp, "&File list", true );
    filelistbut_->activated.notify( mCB(this,uiODInstPkgMgr,filelistCB) );
    filelistbut_->attach( rightAlignedBelow, descfld_ );

    instverfld_ = new uiGenInput( rightgrp, "Installed version" );
    instverfld_->attach( alignedBelow, creatorfld_ );
    availverfld_ = new uiGenInput( rightgrp, "Available version" );
    availverfld_->attach( alignedBelow, instverfld_ );
    uiGroup* lblgrp = new uiGroup( rightgrp, "Action labels" );
    uiLabel* albl = new uiLabel( lblgrp, " Action needed: " );
    albl->setAlignment( Alignment::HCenter );
    actionlbl_ = new uiLabel( lblgrp, "" );
    actionlbl_->setPrefWidthInChar( 10 );
    actionlbl_->setAlignment( Alignment::HCenter );
    actionlbl_->attach( centeredBelow, albl );

    lblgrp->setFrame( true );
    lblgrp->attach( rightOf, instverfld_ );

    uiSeparator* hsep = new uiSeparator( classicgrp_ ); 
    hsep->attach( ensureBelow, splitter );

    pkgsfld_->setPrefWidth( 300 ); descfld_->setPrefWidth( 450 );
    pkgsfld_->setPrefHeight( 400 );
    reinstallfld_ = new uiCheckBox( rightgrp,"Re-Install",
				    mCB(this,uiODInstPkgMgr,reinstallCB) );
    reinstallfld_->attach( alignedBelow, availverfld_ );
}


void uiODInstPkgMgr::mkHTMLGrp()
{
    mainselfld_ = new uiTextBrowser( htmlgrp_, "Desc", mUdf(int), false );
    mainselfld_->linkClicked.notify( mCB(this,uiODInstPkgMgr,urlClickCB) );
    mainselfld_->linkHighlighted.notify( mCB(this,uiODInstPkgMgr,
					 linkHighlightedCB) );
    mainselfld_->setLinkBehavior( uiTextBrowser::None );
    mainselfld_->allowTextSelection( false );
    mainselfld_->setPrefWidth( 800 );
    generateMainSelHtml();
}


void uiODInstPkgMgr::addPreFinaliseStuff()
{
    uiPopupMenu* menu = new uiPopupMenu( this, "&Utilities" );

#define mInsertMenu( nm, cap, fn ) \
    uiMenuItem* nm##mitem = \
			new uiMenuItem(cap,mCB(this,uiODInstPkgMgr,fn)); \
    menu->insertItem( nm##mitem );

    mInsertMenu( explist, "&Export download list", exportDlListCB )
    mInsertMenu( rollback, "&Rollback", rollbackCB )
    mInsertMenu( logfile, "&Show log file", viewLog )
    if ( menuBar() )
	menuBar()->insertItem( menu );
}


void uiODInstPkgMgr::initWin( CallBacker* )
{
    mODInstToLog( "Entered Package Manager." );
    mODInstToLog2( "Number of available packages:", pkgselmgr_->size() );
    mODInstToLog2( "Packages already installed:", pkgselmgr_->nrSelections() );
    for ( int idx=0; idx<pkgselmgr_->size(); idx++ )
    {
	const ODInst::PkgSelection& ps = *(*pkgselmgr_)[idx];
	if ( ps.isinst_ )
	    mODInstLog() << "\t" << ps.pp_.usrname_
		<< " iv=" << ps.instver_.userName()
		<< " av=" << ps.pp_.ver_.userName() << std::endl;

	if ( ps.pp_.ver_.isEmpty() || ps.pp_.internal_ )
	    continue;

	if ( !isnewinst_ || ps.pp_.isDoc() || ps.pp_.pkgnm_ == "devel" )
	    continue;

	if ( packagechoice_ == Academic ||  packagechoice_ == Commercial
	    || ps.pp_.pkgnm_ == "base"
	    || (packagechoice_ != Custom && ps.pp_.pkgnm_ == "demosurvey") )
	    pkgselmgr_->setSelected( ps.pp_, true );
    }

    grpSelChg(0);
    switchViewCB(0);

    mODInstToLog( "" );

    if ( autoupdate_ )
    {
	if ( acceptOK(0) )
	{
	    closewintimer_ = new Timer;
	    closewintimer_->tick.notify( mCB(this,uiODInstPkgMgr,closeWinCB) );
	    closewintimer_->start( 250, true );
	}
    }
}


void uiODInstPkgMgr::closeWinCB( CallBacker* )
{
    done();
}


void uiODInstPkgMgr::switchViewCB( CallBacker* cb )
{
    if ( cb )
	ishtmlon_ = !ishtmlon_;

    classicgrp_->display( !ishtmlon_ );
   /* htmlgrp_->display( ishtmlon_ );
    switchviewbut_->setText( ishtmlon_ ? "&Switch to Classic view"
		    		       : "&Switch to HTML view" );*/
}


void uiODInstPkgMgr::exportDlListCB( CallBacker* )
{
    
    int nrsel = pkgselmgr_->nrSelections();
    if ( !nrsel )
    {
	uiMSG().error( "Please select at least one package to export" );
	return;
    }
    
    nrsel = pkgselmgr_->nrFiles2Get();
    if ( !nrsel )
    {
	uiMSG().error( "No update needed, which means no files to download" );
	return;
    }

    uiFileDialog dlg( this, uiFileDialog::Txt, 0, "Select output file" );
    if ( !dlg.go() )
	return;

    const char* fnm = dlg.fileName();
    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() )
    {
	uiMSG().error( "Cannot open file for writing" );
	return;
    }

    BufferStringSet pkgstodownload;
    pkgselmgr_->getPackageListsForAction( pkgstodownload );
    for ( int idx=0; idx<pkgstodownload.size(); idx++ )
    {
	BufferString zipfile = pkgselmgr_->getFullPackageName(
						pkgstodownload.get(idx) );
	*sd.ostrm << dlHandler().fullURL( appdata_.dirName() )
		  << "/" << zipfile.buf() << std::endl;
    }

    sd.close();
    BufferString msg( "List successfully exported to ", fnm );
    msg += "\nThe Installation Manager does much more than just "
	    "downloading and installing these files";
#ifdef __win__
    msg += "\n\nTo let the Installation Manager install/update OpendTect, "
	    "click on 'Proceed'";
#else
    msg.add( "\n\nIf you still want to install OpendTect manually, then after "
	    "downloading these files, you can run http://opendtect.org/relman/")
    .add( unix_script_filename ).add( " for a new installation." );
#endif
    uiMSG().message( msg );
}


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return; }

void uiODInstPkgMgr::rollbackCB( CallBacker* )
{
    if ( !checkIfODRunning() )
	return;

    FilePath appfp( appdata_.fullDirName() );
    BufferString oldversion( appdata_.dirName(), "_old" );
    BufferString newversion( appdata_.dirName(), "_new" );
    FilePath oldfp( appdata_.baseDirName(), oldversion );
    FilePath newfp( appdata_.baseDirName(), newversion );
    if ( !File::exists(oldfp.fullPath()) )
    {
	uiMSG().message( "No previous installation detected,"
			" Cannot roll back" );
	return;
    }

    BufferString msg( "A previous installation of OpendTect exists",
	    "\nAre you sure you want to switch to the previous version?" );
    if ( !uiMSG().askGoOn(msg) )
	return;

    if ( File::exists(newfp.fullPath()) )
	File::removeDir( newfp.fullPath() );

    if ( !File::rename(appfp.fullPath(),newfp.fullPath()) )
	uiMSG().error( "Rollback failed. It seems some of the OpendTect"
			" files or folders are still open. Please close all"
			" of them and try again." );
    if ( !File::rename(oldfp.fullPath(),appfp.fullPath()) )
    {
	if ( !File::rename(newfp.fullPath(),appfp.fullPath()) ) // undoing
	{
	    uiMSG().error( "Rollback failed. You need to install "
		    	   "OpendTect from scratch" );
	    done(); // everything screwed up : almost impossible
	    return;
	}

	uiMSG().error( "Rollback failed. Most likely you do not have write "
		"permissions in the previous installation. Therefore your "
		"current installation remains unchanged" );
	return;
    }

    uiMSG().message( "Rollback successful" );
    done();
}


void uiODInstPkgMgr::statusMsgCB( CallBacker* cb )
{
     mCBCapsuleUnpack(BufferString,msg,cb);
     const int idx = pkgselmgr_->idxOf( msg );
     if ( idx >= 0 && !(*pkgselmgr_)[idx]->pp_.isHidden() )
     {
	 pkgsfld_->setCurrentItem( pkgselmgr_->getUserName(msg) );
	 return;
     }
     toStatusBar( msg );
}


const ODInst::PkgProps& uiODInstPkgMgr::getPkg( const char* pkgnm ) const
{
    static ODInst::PkgProps* emptypkg = 0;
    if ( !emptypkg )
    {
	emptypkg = new ODInst::PkgProps;
	emptypkg->creator_ = (*pkggrps_[0])[0]->creator_;
    }
    if ( !pkgnm || !*pkgnm )
	return *emptypkg;

    const ODInst::PkgProps* ret = pkggrps_.getPkg( pkgnm, true );
    return ret ? *ret : *emptypkg;
}


const ODInst::PkgProps& uiODInstPkgMgr::curPkg() const
{
    return curpkg_ ? *curpkg_ : getPkg( pkgsfld_->getText() );
}



void uiODInstPkgMgr::showURL( bool creator )
{
    const ODInst::PkgProps& pp = curPkg();

    const BufferString sitenm( creator ? pp.creator_->url_ : pp.getURL() );
    if ( sitenm.isEmpty() ) return;

    uiDesktopServices::openUrl( BufferString( htmlprot, sitenm ) );
}



void uiODInstPkgMgr::getFilteredPackageList(
		ObjectSet<const ODInst::PkgProps>& res ) const
{
    res.erase();

    const BufferString curpkglabel = pkglblfld_->text();
    const bool isall = !pkglblfld_->currentItem();
    for ( int igrp=0; igrp<pkggrps_.size(); igrp ++ )
    {
	const ODInst::PkgGroup& pg = *pkggrps_[igrp];
	for ( int ipkg=0; ipkg<pg.size(); ipkg ++ )
	{
	    const ODInst::PkgProps& pp = *pg[ipkg];
	    if ( pp.ver_.isEmpty() || pp.internal_
		|| pkgselmgr_->idxOf(pp) < 0 )
		continue;
	    

	    if ( !isall )
	    {
		bool keep = false;
		for ( int idx=0; idx<pp.pkglabels_.size(); idx++ )
		{
		    if ( pp.pkglabels_[idx]->name_==curpkglabel )
		    {
			keep = true;
			break;
		    }
		}
		
		if ( !keep )
		    continue;
	    }
	    
	    res += &pp;
	}
    }
}



BufferStringSet uiODInstPkgMgr::getPkgLabels() const
{
    BufferStringSet ret;
    TypeSet<float> sorting;
    TypeSet<int> idxs;
    for ( int igrp=0; igrp<pkggrps_.size(); igrp ++ )
    {
	const ODInst::PkgGroup& pg = *pkggrps_[igrp];
	for ( int ipkg=0; ipkg<pg.size(); ipkg ++ )
	{
	    const ODInst::PkgProps& pp = *pg[ipkg];
	    if ( pp.ver_.isEmpty() || pp.internal_
	      || pkgselmgr_->idxOf(pp) < 0 )
		continue;
	    
	    for ( int idl=0; idl<pp.pkglabels_.size(); idl++ )
	    {
		if ( ret.addIfNew( pp.pkglabels_[idl]->name_ ) )
		{
		    sorting += pp.pkglabels_[idl]->sortval_;
		    idxs += idxs.size();
		}
	    }
	}
    }
    
    sort_coupled( sorting.arr(), idxs.arr(), idxs.size() );
    ret.useIndexes( idxs.arr() );

    return ret;
}


void uiODInstPkgMgr::generateMainSelHtml()
{
    if ( !mainselfld_ )
	return;

    ObjectSet<const ODInst::PkgProps> pkgs;
    getFilteredPackageList( pkgs );
    mainselfld_->setHtmlText( 
	htmlcomp_.generateMainSelHtml(pkgs,pkgselmgr_,pkggrps_) ); 

    isviewingdetail_ = false;
}


void uiODInstPkgMgr::grpSelChg( CallBacker* cb )
{
    NotifyStopper nsi( pkgsfld_->itemChecked );
    ObjectSet<const ODInst::PkgProps> pkgs;
    getFilteredPackageList( pkgs );
    pkgsfld_->setEmpty();

    for ( int idx=0; idx<pkgs.size(); idx++ )
    {
	const ODInst::PkgProps& pp = *pkgs[idx];
	pkgsfld_->addItem( pp.usrname_ );
	const int itmidx = pkgsfld_->size() - 1;
	pkgsfld_->setItemCheckable( itmidx, true );
	const bool issel = pkgselmgr_->isSelected(pp);
	pkgsfld_->setItemChecked( itmidx, issel );
    }

    pkgsfld_->setCurrentItem( 0 );
    if ( !isviewingdetail_ )
	generateMainSelHtml();
}


void uiODInstPkgMgr::pkgSelChg( CallBacker* cb )
{
    const ODInst::PkgProps& pp = getPkg( pkgsfld_->getText() );
    if ( pp.pkgnm_.isEmpty() )
    {
	curpkg_ = 0;
	return;
    }

    curpkg_ = &pp;

    BufferString imagename = curpkg_->pkgnm_;
    imagename += ".png";
    descfld_->setHtmlText( htmlcomp_.generateHTML(*curpkg_) );
    creatorfld_->setText( curpkg_->creator_->name_ );
    creatorurlbut_->setToolTip( curpkg_->creator_->url_ );
    availverfld_->setText( curpkg_->ver_.userName() );
    instverfld_->setText( pkgselmgr_->isInstalled(*curpkg_)
	    	? pkgselmgr_->version(*curpkg_,true).userName() : "-" );
    updActionDisp();
    filelistbut_->setSensitive( pkgselmgr_->isInstalled(*curpkg_) );
    makeFileList( curpkg_->pkgnm_, filelist_ );
}


void uiODInstPkgMgr::itmChck( CallBacker* cb )
{
    const ODInst::PkgProps& curpp = curPkg();
    if ( cb )
    {
	const int curitmidx = pkgsfld_->currentItem();
	pkgselmgr_->setSelected( curpp, pkgsfld_->isItemChecked(curitmidx) );
    }

    generateMainSelHtml();
    updatePkgSelFld();
}


void uiODInstPkgMgr::updActionDisp()
{
    const ODInst::PkgSelMgr::ReqAction ra = pkgselmgr_->reqAction( curPkg() );
    actionlbl_->setText( ODInst::PkgSelMgr::ReqActionNames()[ra] );
    BufferString captn( "Re-Install - ", curPkg().usrname_ );
    if( curPkg().usrname_ == "OpendTect" )
	captn = "Re-Install - OpendTect                   "
	"                                                                  ";
    reinstallfld_->setText( captn );
    reinstallfld_->setSensitive( 
	pkgselmgr_->isInstalled(curPkg()) && ( ra == ODInst::PkgSelMgr::None
				|| ra == ODInst::PkgSelMgr::Reinstall) );
    NotifyStopper ns( reinstallfld_->activated );
    reinstallfld_->setChecked( pkgselmgr_->shouldReInstall(curPkg()) );
}


void uiODInstPkgMgr::updatePkgSelFld()
{
    NotifyStopper ns( pkgsfld_->itemChecked );
    for ( int idx=0; idx<pkgsfld_->size(); idx++ )
    {
	const ODInst::PkgProps& pp = getPkg( pkgsfld_->textOfItem(idx) );
	pkgsfld_->setItemChecked( idx, pkgselmgr_->isSelected( pp ) );
    }

    const int nr2get = pkgselmgr_->nrFiles2Get( true );
    const int nrshown = pkgselmgr_->nrFiles2Get( false );
    BufferString sbm( nr2get < 1 ? "No downloads needed" : "" );
    if ( nr2get > 0 )
    {
	sbm.add( nrshown ).add( " package" ).add( nrshown > 1 ? "s " : " " );
	if ( nrshown != nr2get )
	    sbm.add( "(" ).add( nr2get ).add( " files) " );

	sbm.add( "to download" );
    }

    toStatusBar( sbm );
    updActionDisp();
}


bool uiODInstPkgMgr::checkIfODRunning() const
{
    if ( appdata_.isNewInst() )
	return true;

    while ( true )
    {
	BufferStringSet list;
	if ( !appdata_.getRunningProgs(list) )
	    break;
	else
	{
	    BufferString msg;
	    msg = "The following OpendTect programs are running\n\n";
	    for ( int idx=0; idx<list.size(); idx++ )
	    {
		const BufferString fnm = list.get( idx );
		msg += fnm;
		if ( fnm == "od_main.exe" )
		    msg += " (OpendTect main program)";
		msg += "\n";
	    }
	    
	    msg += "\nPlease make sure you close all of them and press continue"
		   " to proceed";
	    if ( uiMSG().askContinue(msg) == 0 )
		return false;
	}
	
    }

    //a rehearseal of restoreUpdate
    FilePath appfp( appdata_.fullDirName() );
    BufferString tempdir( appdata_.dirName(), "_temp" );
    FilePath tempfp( appdata_.baseDirName(), tempdir );
    if ( File::exists(tempfp.fullPath()) && !File::remove(tempfp.fullPath()) )
    {
	uiMSG().message( tempfp.fullPath(),
			" is blocking the installation process."
			" Please delete this folder manually to proceed." );
	return false;
    }

    if ( !File::rename(appfp.fullPath(),tempfp.fullPath()) )
    {
	BufferString msg( "The OpendTect installation folder " );
	msg += appfp.fullPath();
	msg += " is not available for update right now. ";
	msg += "This could mean one or more of the following:\n\n";
	msg += "1. You do not have permissions to modify this folder.\n";
	msg += "2. One or more OpendTect processes are still running\n";
	msg += "3. This folder or one of its subfolders is open.\n";
	msg += "4. FlexNet License Server daemon(dgbld/lmgrd) is running\n\n";
	msg += "If all else fails, please restart your computer and try again.";
	uiMSG().error( msg );
	return false;
    }

    if ( !File::rename(tempfp.fullPath(),appfp.fullPath()) )
    {
	uiMSG().error( "Update failed: Please exit the Installation Manager "
		       "and start all over again" );
	return false;
    }


    return true;
}


bool uiODInstPkgMgr::acceptOK( CallBacker* )
{
    mODInstToLog( "\nPackage Manager: OK" );
    const int nrsel = pkgselmgr_->nrSelections();
    if ( nrsel < 1 )
    {
	mODInstToLog( "No packages selected.\n" );
	if ( isnewinst_ )
	{
	    uiMSG().warning( "Please select (a) package(s) to install" );
	    return false;
	}

       if ( uiMSG().askGoOn(
		"Do you want to remove this OpendTect installation?") )
	   return removeEntireInstallation();

       return false;
    }

    if ( pkgselmgr_->isNeeded(ODInst::PkgSelMgr::Remove) )
    {
	if ( !uiMSG().askGoOn("Cannot remove packages (yet)."
	    "\nIf you want to remove the entire installation, "
#ifdef __win__
	    "\nuse the 'uninstall' option from the Start menu"
#else
	    "\nremove the installation directory by hand."
#endif
	    "\nDo you want to continue the update without removing?") )
	    return false;
    }

    const int nr2get = pkgselmgr_->nrFiles2Get();
    if ( nr2get < 1 )
    {
	mODInstToLog( "No packages to download, Pkg Mgr done.\n" );
	if ( autoupdate_ )
	    return true;

	uiMSG().message( "No updates required" );
	return true;
    }

    if ( !dlHandler().remoteStatusOK(true) )
    {
	BufferString msg( "You are very unfortunate:\n" );
	if ( !dlHandler().remoteStatus() )
	{
	    msg += "The download site is currently not available.\n";
	    msg += "Please try again after some time.";
	}
	else
	{
	    msg += "The download site has changed during package selection.\n";
	    msg += "You will have to do your selections again.";
	}

	uiMSG().error( msg.buf() );
	return true;
    }

#ifdef __win__
    if ( !checkIfODRunning() )
	return false;
#endif
    pkgsfld_->setCurrentItem( 0 );
    return doInstallation();
}


bool uiODInstPkgMgr::removeEntireInstallation()
{
    mODInstToLog( "Remove of entire app requested." );

    mODInstToLog( "Not implemented yet." );
    uiMSG().error( "TODO: not impl yet" );
    return false;
}


#define mErrorRet \
{ \
    if ( uitr.getState() == (int) Task::Stop ) \
    { mODInstToLog("Operation aborted by user"); return false; } \
    mODInstToLog(insthandler_->errorMsg()); \
    if ( uiMSG().askGoOn(insthandler_->errorMsg(),"View Log","Quit") ) \
    viewLog(); \
    else \
    { cleanUp(); ExitProgram(0); } \
    return false; \
}

#define mDirExistCheck( dirnm ) \
    if ( !File::exists(dirnm) ) \
    { \
	if ( !File::createDir(dirnm) ) \
	{ \
	    errmsg = "could not create directory in "; \
	    errmsg += dirnm; \
	    errmsg += ". Please check if you have write permission"; \
	    return false; \
	} \
    }

#ifdef __win__

#define mDirCheck( dirnm ) mDirExistCheck( dirnm )

#else

#define mDirWritableCheck( dirnm ) \
    if ( !File::isWritable(dirnm) ) \
    { \
	errmsg = "You do not have write permission in "; \
	errmsg += dirnm; \
	return false; \
    }

#define mDirCheck( dirnm ) mDirExistCheck( dirnm ) mDirWritableCheck( dirnm )

#endif


bool uiODInstPkgMgr::doInstallation()
{
    mODInstToLog( "Starting installation." );
    uiTaskRunner uitr( this, false );
    if ( !insthandler_->downLoadPackages(&uitr,!isoonline_) )
	mErrorRet

    uitr.button( uiDialog::CANCEL )->setSensitive( false );  //do not abort
    if ( !insthandler_->installZipPackages(!isoonline_,&uitr) )
    {
	reportError( "Installation failed", insthandler_->errorMsg() );
	return false;
    }

    if ( !isoonline_ )
    {
	BufferString msg( "OpendTect offline-installation packages"
			    " created successfully under " );
	msg += appdata_.baseDirName();
	if ( platform_.isWindows() )
	    msg += " .Please use od_setup.exe to install the packge";
	else
	{
	    msg.add( ".\nTo install the downloaded packages, please run the '" )
		.add( unix_script_filename ).add( "' script" );
	}

        uiMSG().message( msg );
	return true;
    }

    if ( isnewinst_ ) getDtectData();
    if ( !insthandler_->configureInstallation() )
    {
	BufferString msg( "Failed to configure: " );
	msg += insthandler_->errorMsg();
	msg += " .You can still run the program form the"
		"installation directory located in ";
	msg += appdata_.binPlfDirName();
	uiMSG().warning( msg );
	return true;
    }
	
    BufferString msg;
    getInstalledMsg( msg );
    uiMSG().message( msg );
    return true;
}


void uiODInstPkgMgr::reinstallCB( CallBacker* cb )
{
    pkgselmgr_->setForceReinstall( curPkg(), reinstallfld_->isChecked() );
    updActionDisp();
}


void uiODInstPkgMgr::getInstalledMsg( BufferString& msg )
{
#define mAddMsg( varnm, list, mmsg ) \
    const int varnm = list.size(); \
	if ( varnm ) \
	{ \
	    msg += "\n\n"; \
	    msg += mmsg; \
	    for ( int idx=0; idx<varnm; idx++ )\
	    {\
		 msg += "\n    ";\
		 msg += list.get( idx );\
	    }\
	}\

    if ( !autoupdate_ )
    {
	const BufferStringSet& installedpckglist =
	    			insthandler_->installedPackages();
	const BufferStringSet& updatedpckglist =
	    			insthandler_->updatedPackages();
	const BufferStringSet& reinstalledpckglist =
	    			insthandler_->reinstalledPackages();
	mAddMsg( instsz, installedpckglist, "Installed packages :-" );
	mAddMsg( updsz, updatedpckglist, "Updated packages :-" );
	mAddMsg( reinstsz, reinstalledpckglist, "Re-Installed packages :-" );
    }
}


bool uiODInstPkgMgr::rejectOK( CallBacker* cb )
{
    if ( isviewingdetail_ )
    {
	generateMainSelHtml();
	mainselfld_->restoreScrollPos();
	return false;
    }

    return uiDialog::rejectOK( cb );
}


void uiODInstPkgMgr::filelistCB( CallBacker* )
{
    uiTextFileDlg::Setup su( 0 );
    su.modal( true );
    uiTextFileDlg dlg( this, su );
    dlg.editor()->textBrowser()->clear();
    dlg.editor()->textBrowser()->setText( filelist_.buf() );
    dlg.setCaption( "File list" );
    dlg.go();
}


void uiODInstPkgMgr::viewLog() const
{
    const BufferString& logfnm = mODInstLogger().logfnm_;
    StreamData sd = StreamProvider( logfnm ).makeIStream();
    BufferString filedata;
    StrmOper::readFile( *sd.istrm, filedata );
    sd.close();
    uiTextFileDlg::Setup su( 0 );
    su.modal( true );
    uiODInstPkgMgr* _this = const_cast<uiODInstPkgMgr*>( this );
    uiTextFileDlg dlg( _this, su );
    dlg.editor()->textBrowser()->clear();
    dlg.editor()->textBrowser()->setText( filedata.buf() );
    dlg.setCaption( logfnm );
    dlg.go();
}


void uiODInstPkgMgr::makeFileList( const char* pkgnm,
				   BufferString& filelist ) const
{
    appdata_.getFileList( pkgnm, filelist );
}


void uiODInstPkgMgr::cleanUp() const
{
    File::removeDir( ODInst::sTempDataDir() );
}


void uiODInstPkgMgr::urlClickCB( CallBacker* cb )
{
    BufferString url = mainselfld_->lastLink();

    if ( htmlprot.isStartOf( url ) )
    {
	uiDesktopServices::openUrl( url );
	return;
    }

    if ( !odprotocol.isStartOf( url ) )
	return;

    SeparString parser( url.buf()+odprotocol.size(), '/' );
    if ( parser.size()!=2 )
	return;

    BufferString action = parser[0];

    BufferString package = parser[1];

    if ( action==closedetailcmd )
    {
	generateMainSelHtml();
	mainselfld_->restoreScrollPos();
	return;
    }

    const int idx = pkgselmgr_->idxOf( package );
    if ( idx==-1 )
	return;

    const ODInst::PkgProps& pp = (*pkgselmgr_)[idx]->pp_;
    if ( pp.pkgnm_.isEmpty() )
	return;

    if ( action==filelistcmd )
    {
	BufferString filelist;
	makeFileList( pp.pkgnm_, filelist );
	uiTextFileDlg::Setup su( 0 );
	su.modal( true );
	uiTextFileDlg dlg( this, su );
	dlg.editor()->textBrowser()->clear();
	dlg.editor()->textBrowser()->setText( filelist.buf() );
	dlg.setCaption( "File list" );
	dlg.go();
	return;
    }

    if ( !isviewingdetail_ )
	mainselfld_->recordScrollPos();

    if ( action==uninstallcmd || action==installcmd )
    {
	pkgselmgr_->setSelected( pp, action==installcmd );
	if ( isviewingdetail_ )
	    mainselfld_->setHtmlText( htmlcomp_.generateHTML(pp,pkgselmgr_) );
	else
	    generateMainSelHtml();
	
	updatePkgSelFld();
    }
    else if ( action==detailscmd )
    {
	mainselfld_->setHtmlText(  htmlcomp_.generateHTML(pp,pkgselmgr_) );
	curpkg_ = &pp;
	isviewingdetail_ = true;
	return;
    }

    if ( !isviewingdetail_ )
	mainselfld_->restoreScrollPos();
}


void uiODInstPkgMgr::linkHighlightedCB( CallBacker* cb )
{
    BufferString url = mainselfld_->lastLink();
    if ( htmlprot.isStartOf( url ) )
    {
	mainselfld_->showToolTip( url );
	return;
    }

    if ( !odprotocol.isStartOf( url ) )
	return;

    SeparString parser( url.buf()+odprotocol.size(), '/' );
    if ( parser.size()!=2 )
	return;

    BufferString action = parser[0];
    BufferString package = parser[1];
    if ( action==closedetailcmd )
    {
	mainselfld_->showToolTip( "Back to package selection page" );
	return;
    }
    if ( action==detailscmd )
    {
	mainselfld_->showToolTip( "View details" );
	return;
    }
    if ( action==filelistcmd )
    {
	mainselfld_->showToolTip( "List of installed files for this package" );
	return;
    }


    const int idx = pkgselmgr_->idxOf( package );
    if ( idx==-1 )
	return;

    const ODInst::PkgSelection& pkgsel = *(*pkgselmgr_)[idx];
    const ODInst::PkgSelMgr::ReqAction reqact = pkgselmgr_->reqAction(pkgsel);
    const ODInst::PkgProps& pp = pkgsel.pp_;
    if ( pp.pkgnm_.isEmpty() )
	return;

    BufferString tooltip;
    if ( action==uninstallcmd )
    {
	if ( reqact == ODInst::PkgSelMgr::None )
	    tooltip = "Already installed.\nClick to uninstall";
	else if ( reqact == ODInst::PkgSelMgr::Install )
	    tooltip = "Selected for installation.\nClick to unselect";
	else //update
	    tooltip = "Selected for update.\nClick to uninstall";
    }
    else if ( action==installcmd )
    {
	if ( reqact == ODInst::PkgSelMgr::None )
	    tooltip = "Not installed yet.\nClick to install";
	else //remove
	    tooltip = "Selected for uninstallation.\nClick to keep";
    }
    else
	return;

    mainselfld_->showToolTip( tooltip );
}


void uiODInstPkgMgr::getDtectData()
{
#ifdef __lux__
    BufferString dtectdatavar = GetEnvVar( "DTECT_DATA" );
    if ( !dtectdatavar.isEmpty() && File::isDirectory(dtectdatavar) )
	return;

    BufferString msg( "OpendTect needs a location (DTECT_DATA) to store ",
	    	      "its data/surveys. You can force the initial value of ",
		      "DTECT_DATA" );
    if ( !dtectdatavar.isEmpty() )
    {
	msg = "You have set the value of DTECT_DATA to ";
	msg += dtectdatavar;
	msg += ". But this path does not exist or is not mounted. ";
	msg += "You can set another path as DTECT_DATA";
    }

    if ( !uiMSG().askGoOn(msg,"Select DTECT_DATA","Skip this step") )
	return;

    uiFileDialog dlg( this, false, 0, 0, "Select DTECT_DATA" );
    dlg.setMode( uiFileDialog::DirectoryOnly );
    if ( dlg.go() )
	SetEnvVar( "DTECT_DATA", dlg.fileName() );
#endif
}

