/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uisurvey.h"

#include "uiconvpos.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilatlong2coord.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisetdatadir.h"
#include "uisip.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "latlong.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "odver.h"
#include "strmprov.h"
#include "survinfo.h"

#include <iostream>
#include <math.h>

#define mMapWidth	300
#define mMapHeight	300


static ObjectSet<uiSurvey::Util>& getUtils()
{
    static ObjectSet<uiSurvey::Util>* utils = 0;
    if ( !utils )
    {
	utils = new ObjectSet<uiSurvey::Util>;
	*utils += new uiSurvey::Util( "xy2ic", "Convert (X,Y) to/from I/C",
				      CallBack() );
	*utils += new uiSurvey::Util( "spherewire",
				"Setup geographical coordinates",
				      CallBack() );
    }
    return *utils;
}


void uiSurvey::add( const uiSurvey::Util& util )
{
    getUtils() += new uiSurvey::Util( util );
}


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File::isLink(dirnm) )
    {
	BufferString newdirnm = File::linkTarget(dirnm);
	fp.set( newdirnm );
	if ( !fp.isAbsolute() )
	{
	    FilePath dirnmfp( dirnm );
	    dirnmfp.setFileName( 0 );
	    fp.setPath( dirnmfp.fullPath() );
	}
	dirnm = fp.fullPath();
    }
    return dirnm;
}


static bool copySurv( const char* from, const char* todirnm, int mb )
{
    const BufferString todir( todirnm );
    if ( File::exists(todir) )
    {
	BufferString msg( "A survey '" );
	msg += todirnm;
	msg += "' already exists.\nYou will have to remove it first";
        uiMSG().error( msg );
        return false;
    }

    BufferString msg;
    if ( mb > 0 )
	{ msg += mb; msg += " MB of data"; }
    else
	msg += "An unknown amount";
    msg += " of data needs to be copied.\nDuring the copy, OpendTect will "
	    "freeze.\nDepending on the data transfer rate, this can take "
	    "a long time!\n\nDo you wish to continue?";
    if ( !uiMSG().askContinue( msg ) )
	return false;

    const BufferString fromdir = getTrueDir( FilePath(from).fullPath() );

    MouseCursorChanger cc( MouseCursor::Wait );
    if ( !File::copy( fromdir, todir ) )
    {
        uiMSG().error( "Cannot copy the survey data" );
        return false;
    }

    File::makeWritable( todir, true, true );
    return true;
}


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey selection",
	       "Select and setup survey","0.3.1").nrstatusflds(1))
    , initialdatadir_(GetBaseDataDir())
    , initialsurvey_(GetSurveyName())
    , survinfo_(0)
    , survmap_(0)
    , impiop_(0)
    , impsip_(0)
    , initialsurveyparchanged_(false)
{
    const int lbwidth = 250;
    const int noteshght = 5;
    const int totwdth = lbwidth + mMapWidth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr ) return;

    mkDirList();

    uiGroup* rightgrp = new uiGroup( this, "Survey selection right" );

    survmap_ = new uiSurveyMap( rightgrp );
    survmap_->setStretch( 0, 0 );
    survmap_->setPrefWidth( mMapWidth );
    survmap_->setPrefHeight( mMapHeight );

    uiGroup* leftgrp = new uiGroup( this, "Survey selection left" );
    listbox_ = new uiListBox( leftgrp, dirlist_, "Surveys" );
    listbox_->setCurrentItem( GetSurveyName() );
    listbox_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    listbox_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    listbox_->setPrefWidth( lbwidth );
    listbox_->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    newbut_ = new uiPushButton( leftgrp, "&New",
	    			mCB(this,uiSurvey,newButPushed), false );
    newbut_->attach( rightOf, listbox_ );
    newbut_->setPrefWidthInChar( 12 );
    rmbut_ = new uiPushButton( leftgrp, "&Remove",
	    		       mCB(this,uiSurvey,rmButPushed), false );
    rmbut_->attach( alignedBelow, newbut_ );
    rmbut_->setPrefWidthInChar( 12 );
    editbut_ = new uiPushButton( leftgrp, "&Edit",
	    			 mCB(this,uiSurvey,editButPushed), false );
    editbut_->attach( alignedBelow, rmbut_ );
    editbut_->setPrefWidthInChar( 12 );
    copybut_ = new uiPushButton( leftgrp, "C&opy",
	    			 mCB(this,uiSurvey,copyButPushed), false );
    copybut_->attach( alignedBelow, editbut_ );
    copybut_->setPrefWidthInChar( 12 );

    archbut_ = new uiPushButton( leftgrp, "&Archive",
				 mCB(this,uiSurvey,archButPushed), false );
    archbut_->attach( alignedBelow, copybut_ );
    archbut_->setPrefWidthInChar( 12 );

    ObjectSet<uiSurvey::Util>& utils = getUtils();
    uiGroup* utilbutgrp = new uiGroup( rightgrp, "Surv Util buttons" );
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( utilbutgrp, util.pixmap_,
						util.tooltip_, cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( idx > 0 )
	    utilbuts_[idx]->attach( rightOf, utilbuts_[idx-1] );
    }
    utilbutgrp->attach( centeredBelow, survmap_ );

    datarootbut_ = new uiPushButton( leftgrp, "&Set Data Root",
	    			mCB(this,uiSurvey,dataRootPushed), false );
    datarootbut_->attach( centeredBelow, listbox_ );

    uiSeparator* horsep1 = new uiSeparator( this );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( stretchedBelow, rightgrp, -2 );
    horsep1->attach( ensureBelow, leftgrp );

    uiGroup* infoleft = new uiGroup( this, "Survey info left" );
    uiGroup* inforight = new uiGroup( this, "Survey info right" );
    infoleft->attach( alignedBelow, leftgrp );
    infoleft->attach( ensureBelow, horsep1 );
    inforight->attach( alignedBelow, rightgrp );
    inforight->attach( ensureBelow, horsep1 );

    infoleft->setFont( FontList().get(FontData::key(FontData::ControlSmall)) );
    inforight->setFont( FontList().get(FontData::key(FontData::ControlSmall)));

    inllbl_ = new uiLabel( infoleft, "" ); 
    crllbl_ = new uiLabel( infoleft, "" );
    arealbl_ = new uiLabel( infoleft, "" );
    zlbl_ = new uiLabel( inforight, "" ); 
    binlbl_ = new uiLabel( inforight, "" );
    typelbl_ = new uiLabel( inforight, "" );
#if 0
    inllbl_->setHSzPol( uiObject::widevar );
    crllbl_->setHSzPol( uiObject::widevar );
    zlbl_->setHSzPol( uiObject::widevar );
    binlbl_->setHSzPol( uiObject::widevar );
    arealbl_->setHSzPol( uiObject::widevar );
#else
    inllbl_->setPrefWidthInChar( 40 );
    crllbl_->setPrefWidthInChar( 40 );
    zlbl_->setPrefWidthInChar( 40 );
    binlbl_->setPrefWidthInChar( 40 );
    arealbl_->setPrefWidthInChar( 40 );
    typelbl_->setPrefWidthInChar( 40 );
#endif

    crllbl_->attach( alignedBelow, inllbl_ );
    arealbl_->attach( alignedBelow, crllbl_ );
    binlbl_->attach( alignedBelow, zlbl_ );
    typelbl_->attach( alignedBelow, binlbl_ );

    uiSeparator* horsep2 = new uiSeparator( this );
    horsep2->attach( stretchedBelow, infoleft, -2 );
    horsep2->setPrefWidth( totwdth );

    uiLabel* notelbl = new uiLabel( this, "Notes:" );
    notelbl->attach( alignedBelow, horsep2 );
    notes_ = new uiTextEdit( this, "Notes" );
    notes_->attach( alignedBelow, notelbl);
    notes_->setPrefHeightInChar( noteshght );
    notes_->setPrefWidth( totwdth );

    getSurvInfo(); 
    mkInfo();
    setOkText( "&Ok (Select)" );

    postFinalise().notify( mCB(this,uiSurvey,selChange) );
}


uiSurvey::~uiSurvey()
{
    delete impiop_;
    delete survinfo_;
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !survmap_ ) return;
    BufferString oldnm = listbox_->getText();
  
    delete survinfo_;
    FilePath fp( GetSoftwareDir(0), "data", "BasicSurvey" );
    survinfo_ = SurveyInfo::read( fp.fullPath() );
    survinfo_->dirname_ = "";
    mkInfo();
    if ( !survInfoDialog() )
	updateInfo(0);

    rmbut_->setSensitive(true);
    editbut_->setSensitive(true);
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive(true);
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !survInfoDialog() )
	updateInfo(0);
}


class uiSurveyGetCopyDir : public uiDialog
{
public:

uiSurveyGetCopyDir( uiParent* p, const char* cursurv )
	: uiDialog(p,uiDialog::Setup("Import survey from location",
		   "Copy surveys from any data root","0.3.1"))
{
    BufferString curfnm;
    if ( cursurv && *cursurv )
	curfnm = FilePath( GetBaseDataDir(), cursurv ).fullPath();
    else
	curfnm = GetBaseDataDir();

    inpsurveyfld_ = new uiSurveySelect( this,"Survey to copy" );
    inpsurveyfld_->setSurveyPath( curfnm );
    newsurveyfld_ = new uiSurveySelect( this, "New Survey" );
    newsurveyfld_->attach( alignedBelow,  inpsurveyfld_ );
}


void inpSel( CallBacker* )
{
    BufferString fullpath;
    inpsurveyfld_->getFullSurveyPath( fullpath );
    FilePath fp( fullpath );
    newsurveyfld_->setInputText( fp.fullPath() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    if ( !inpsurveyfld_->getFullSurveyPath( fname_ ) ||
	 !newsurveyfld_->getFullSurveyPath( newdirnm_) )
    {	 mErrRet( "No Valid or Empty Input" ); }

    return true;
}

    BufferString	fname_;
    BufferString	newdirnm_;
    uiSurveySelect*	inpsurveyfld_;
    uiSurveySelect*	newsurveyfld_;
};


void uiSurvey::copyButPushed( CallBacker* )
{
    uiSurveyGetCopyDir dlg( this, listbox_->getText() );
    if ( !dlg.go() )
	return;

    if ( !copySurv( dlg.fname_, dlg.newdirnm_, -1 ) )
	return;

    SurveyInfo* si = SurveyInfo::read( dlg.fname_ );
    if ( si && si->isValid() )
    {
	BufferString newnm( FilePath(dlg.newdirnm_).fileName() );
	si->dirname_ = newnm;
	replaceCharacter( newnm.buf(), '_', ' ' );
	si->setName( newnm );
	si->write();
    }
    delete si;

    updateSvyList();
    listbox_->setCurrentItem( dlg.newdirnm_ );
    updateSvyFile();
    newSurvey();
    SetSurveyName( listbox_->getText() );
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( dlg.go() )
    {
	mkDirList();
	updateSvyList();
    }
}


void uiSurvey::mkDirList()
{
    dirlist_.erase();
    getSurveyList( dirlist_ );
}


bool uiSurvey::survInfoDialog()
{
    delete impiop_; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *survinfo_ );
    if ( !dlg.isOK() )
	return false;

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    if ( initialsurvey_==listbox_->getText() )
	initialsurveyparchanged_ = true;

    updateSvyList();
    listbox_->setCurrentItem( dlg.dirName() );
    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;
    return true;
}


void uiSurvey::rmButPushed( CallBacker* )
{
    BufferString selnm( listbox_->getText() );
    const BufferString seldirnm = FilePath(GetBaseDataDir())
					    .add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey directory:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    if ( !uiMSG().askRemove( msg ) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    bool rmres = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmres )
    {
        msg = truedirnm; msg += "\nnot removed properly";
        return;
    }

    if ( seldirnm != truedirnm ) // must have been a link
	File::remove( seldirnm );

    updateSvyList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
        BufferString newsel( listbox_->getText() );
        writeSurveyName( newsel );
	if ( button(CANCEL) ) button(CANCEL)->setSensitive( false );
    }
}


void uiSurvey::archButPushed( CallBacker* )
{
    uiDialog dlg( this,
	uiDialog::Setup("Archive survey",mNoDlgTitle,mTODOHelpID) );
    (void)new uiFileInput( &dlg, "Destination",
			    uiFileInput::Setup().directories(true) );
    if ( !dlg.go() )
	return;

    uiMSG().error( "Not implemented yet" );
}


void uiSurvey::utilButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb ) { pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 ) { pErrMsg("Huh"); return; }

    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *survinfo_ );
	dlg.go();
    }
    else if ( butidx == 1 )
    {
	if ( !survinfo_ ) return;
	uiLatLong2CoordDlg dlg( this, survinfo_->latlong2Coord(), survinfo_ );
	if ( dlg.go() && !survinfo_->write() )
	    uiMSG().error( "Could not write the setup" );
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateSvyList()
{
    mkDirList();
    if ( dirlist_.isEmpty() ) updateInfo(0);
    listbox_->setEmpty();
    listbox_->addItems( dirlist_ );
}


bool uiSurvey::updateSvyFile()
{
    BufferString seltxt( listbox_->getText() );
    if ( seltxt.isEmpty() ) return true;

    if ( !writeSurveyName( seltxt ) )
    {
        ErrMsg( "Cannot update the 'survey' file in $HOME/.od/" );
        return false;
    }
    if ( !File::exists( FilePath(GetBaseDataDir()).add(seltxt).fullPath() ) )
    {
        ErrMsg( "Survey directory does not exist anymore" );
        return false;
    }

    return true;
}


bool uiSurvey::writeSurveyName( const char* nm )
{
    const char* ptr = GetSurveyFileName();
    if ( !ptr )
    {
        ErrMsg( "Error in survey system. Please check $HOME/.od/" );
        return false;
    }

    StreamData sd = StreamProvider( ptr ).makeOStream();
    if ( !sd.usable() )
    {
        BufferString errmsg = "Cannot write to ";
        errmsg += ptr;
        ErrMsg( errmsg );
        return false;
    }

    *sd.ostrm << nm;

    sd.close();
    return true;
}


void uiSurvey::mkInfo()
{
    const SurveyInfo& si = *survinfo_;
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range " );
    zinfo += si.getZUnitString(); zinfo += ": ";
    BufferString bininfo( "Bin size (", si.getXYUnitString(false), "/line): ");
    BufferString areainfo( "Area (sq ", si.xyInFeet() ? "mi" : "km", "): " );

    if ( si.sampling(false).hrg.totalNr() )
    {
	inlinfo += si.sampling(false).hrg.start.inl;
	inlinfo += " - "; inlinfo += si.sampling(false).hrg.stop.inl;
	inlinfo += "  step: "; inlinfo += si.inlStep();
	
	crlinfo += si.sampling(false).hrg.start.crl;
	crlinfo += " - "; crlinfo += si.sampling(false).hrg.stop.crl;
	crlinfo += "  step: "; crlinfo += si.crlStep();

	float inldist = si.inlDistance();
	float crldist = si.crlDistance();

	#define mkString(dist) \
	nr = (int)(dist*100+.5); bininfo += nr/100; \
	rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest; \

	int nr, rest;    
	bininfo += "inl: "; mkString(inldist);
	bininfo += "  crl: "; mkString(crldist);
	float area = (float) ( si.computeArea(false) * 1e-6 ); //in km2
	if ( si.xyInFeet() )
	    area /= 2.590; // square miles

	areainfo += area;
    }

    #define mkZString(nr) \
    zinfo += istime ? mNINT32(1000*nr) : nr;

    const bool istime = si.zIsTime(); 
    mkZString( si.zRange(false).start );
    zinfo += " - "; mkZString( si.zRange(false).stop );
    zinfo += "  step: "; mkZString( si.zRange(false).step );

    inllbl_->setText( inlinfo );
    crllbl_->setText( crlinfo );
    binlbl_->setText( bininfo );
    arealbl_->setText( areainfo );
    zlbl_->setText( zinfo );
    typelbl_->setText( BufferString("Survey type: ",
			SurveyInfo::toString(si.survDataType())) );
    notes_->setText( si.comment() );

    bool anysvy = dirlist_.size();
    rmbut_->setSensitive( anysvy );
    editbut_->setSensitive( anysvy );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( anysvy );

    FilePath fp( si.datadir_, si.dirname_ );
    fp.makeCanonical();
    toStatusBar( fp.fullPath() );
}


void uiSurvey::selChange( CallBacker* )
{
    writeComments();
    updateInfo(0);
}


void uiSurvey::updateInfo( CallBacker* cb )
{
    mDynamicCastGet(uiSurveyInfoEditor*,dlg,cb);
    if ( !dlg )
	getSurvInfo();

    mkInfo();
    survmap_->drawMap( survinfo_ );
}


void uiSurvey::writeComments()
{
    BufferString txt = notes_->text();
    if ( txt == survinfo_->comment() ) return;

    survinfo_->setComment( txt );
    if ( !survinfo_->write( GetBaseDataDir() ) )
        ErrMsg( "Failed to write survey info.\nNo changes committed." );
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( initialdatadir_ != GetBaseDataDir() )
    {
	if ( !uiSetDataDir::setRootDataDir( initialdatadir_ ) )
	{
	    uiMSG().error( "As we cannot reset to the old Data Root,\n"
		    	   "You *have to* select a survey now!" );
	    return false;
	}
    }

    return true;
}


bool uiSurvey::acceptOK( CallBacker* )
{
    if ( listbox_->currentItem() < 0 )
    {
	uiMSG().error( "Please create a survey (or press Cancel)" );
	return false;
    }
    writeComments();

    const bool samesurvey = initialsurvey_ == listbox_->getText();
    if ( !samesurvey || initialdatadir_ != GetBaseDataDir() || 
	 (samesurvey && initialsurveyparchanged_) )
	IOMan::enableSurveyChangeTriggers( true );
    const bool cansetnewsurv =
	updateSvyFile() && IOMan::setSurvey( listbox_->getText() );
    
    IOMan::enableSurveyChangeTriggers( false );
    if ( !cansetnewsurv )
	return false;

    newSurvey();
    if ( impiop_ && impsip_ )
    {
	const char* askq = impsip_->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(askq) )
	{
	    IOM().to( "100010" );
	    impsip_->startImport( parent(), *impiop_ );
	}
    }

    return true;
}


void uiSurvey::getSurvInfo()
{
    BufferString fname = FilePath( GetBaseDataDir() )
	    		.add( listbox_->getText() ).fullPath();
    delete survinfo_;
    survinfo_ = SurveyInfo::read( fname );
}


void uiSurvey::newSurvey()
{
    SI().setInvalid();
}


void uiSurvey::getSurveyList( BufferStringSet& list, const char* dataroot )
{
    BufferString basedir = dataroot;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const char* dirnm = dl.get( idx );
	if ( matchString("_New_Survey_",dirnm) )
	    continue;

	const FilePath fp( basedir, dirnm, ".survey" );
	if ( File::exists(fp.fullPath()) )
	    list.add( dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

    BufferString warnmsg( "Your survey is set up as '" );
    warnmsg += is2d ? "3-D only'.\nTo be able to actually use 2-D"
		    : "2-D only'.\nTo be able to actually use 3-D";
    warnmsg += " data\nyou will have to change the survey setup.";
    warnmsg += "\n\nDo you wish to continue?";

    return uiMSG().askContinue( warnmsg );
}
