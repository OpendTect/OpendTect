/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyscandlg.cc,v 1.30 2010-10-07 06:35:33 cvsnanne Exp $";

#include "uisegyscandlg.h"

#include "datainpspec.h"
#include "ioman.h"
#include "keystrs.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "uigeninput.h"
#include "uisegydef.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibatchlaunch.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "pixmap.h"

#include "segyfiledef.h"
#include "segyfiledata.h"
#include "segyscanner.h"
#include "segydirectdef.h"
#include "seispsioprov.h"
#include "file.h"
#include <sstream>


uiSEGYScanDlg::uiSEGYScanDlg( uiParent* p, const uiSEGYReadDlg::Setup& su,
			      IOPar& iop, bool ss )
    : uiSEGYReadDlg(p,su,iop,ss)
    , scanner_(0)
    , indexer_(0)
    , forsurvsetup_(ss)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_,false))
    , outfld_(0)
    , parfilefld_(0)
    , lnmfld_(0)
{
    uiObject* attobj = 0;
    if ( setup_.dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Scan " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += fs.fname_; ttl += "'";
	setTitleText( ttl );
    }

    if ( forsurvsetup_ )
    {
	if ( !optsgrp_ )
	    attobj = new uiLabel( this,
		    		  "Press OK or hit enter to start SEG-Y scan" );
    }
    else
    {
	ctio_.setObj( 0 );
	IOObjContext& ctxt = ctio_.ctxt;
	ctxt.deftransl = ctxt.toselect.allowtransls_ = "SEGYDirect";
	uiSeisSel::Setup sssu( setup_.geom_ ); sssu.selattr( false );
	outfld_ = new uiSeisSel( this, ctio_, sssu );
	if ( optsgrp_ )
	    outfld_->attach( alignedBelow, optsgrp_ );
	else
	    attobj = outfld_->attachObj();

	if ( Seis::is2D(setup_.geom_) )
	{
	    outfld_->setConfirmOverwrite( false );
	    lnmfld_ = new uiSeis2DLineSel( this );
	    lnmfld_->attach( alignedBelow, outfld_ );
	}

	parfilefld_ = new uiGenInput( this, "Parameter file",
		StringInpSpec( GetProcFileName("scan_segy.par" ) ) );
	parfilefld_->attach( alignedBelow,
		lnmfld_ ? (uiObject*) lnmfld_ : (uiObject*) outfld_ );
    }

    if ( attobj )
    {
	uiToolButton* tb = new uiToolButton( this, "Pre-scan",
			   ioPixmap("prescan.png"),
			   mCB(this,uiSEGYScanDlg,preScanCB) );
	tb->attach( rightTo, attobj ); tb->attach( rightBorder );
	tb->setToolTip( "Limited Pre-scan" );
    }
}


uiSEGYScanDlg::~uiSEGYScanDlg()
{
    delete ctio_.ioobj;
    delete scanner_;
    delete indexer_;
    delete &ctio_;
}


SEGY::Scanner* uiSEGYScanDlg::getScanner()
{
    SEGY::Scanner* ret = scanner_;
    scanner_ = 0;
    return ret;
}


#define mErrRet(s1,s2) { if ( s1 ) uiMSG().error(s1,s2); return false; }


bool uiSEGYScanDlg::doWork( const IOObj& )
{
    BufferString pathnm, lnm;
    if ( outfld_ )
    { 
	if ( lnmfld_ )
	{
	    lnm = lnmfld_->lineName();
	    if ( lnm.isEmpty() )
		mErrRet("Please select the line name",0)
	}

        if ( !outfld_->commitInput() )
	    mErrRet(outfld_->isEmpty() ?
		    "Please enter a name for the output data store" : 0, 0)

	pathnm = ctio_.ioobj->fullUserExpr( Conn::Write );
	if ( lnmfld_ )
	{
	    if ( !File::isDirectory(pathnm) )
	    {
		File::createDir(pathnm);
		if ( !File::isDirectory(pathnm) )
		    mErrRet("Cannot create directory for output:\n",pathnm)
	    }
	    if ( !File::isWritable(pathnm) )
		mErrRet("Output directory is not writable:\n",pathnm)
	}
	else
	{
	    if ( File::exists(pathnm) && !File::isWritable(pathnm) )
		mErrRet("Cannot overwrite output file:\n",pathnm)
	}
    }

    SEGY::FileSpec fs;
    fs.usePar( pars_ );

    Executor* exec = 0;

    delete scanner_;
    scanner_ = 0;

    delete indexer_;
    indexer_ = 0;

    if ( outfld_ )
    {
	pars_.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyIndexPS() );
	pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(setup_.geom_) );
	pars_.set( sKey::Output, ctio_.ioobj->key() );
	pars_.set( sKey::LineName, lnm );
	uiBatchLaunch launcher( this, pars_, 0, "od_process_segyio", false );
	launcher.setParFileName( parfilefld_->text() );

	return launcher.go();
    }
    else
    {
	exec = scanner_ = new SEGY::Scanner( fs, setup_.geom_, pars_ );

	if ( setup_.rev_ == uiSEGYRead::Rev0 )
	    scanner_->setForceRev0( true );
	if ( forsurvsetup_ )
	    scanner_->setRichInfo( true );
    }

    uiTaskRunner tr( parent_ );
    bool rv = tr.execute(*exec);
    if ( !rv )
    {
	if ( outfld_ )
	    IOM().permRemove( ctio_.ioobj->key() );
	return false;
    }

    if ( !displayWarnings( scanner_
		? scanner_->warnings()
		: indexer_->scanner()->warnings()
	, outfld_) )
    {
	if ( outfld_ )
	    IOM().permRemove( ctio_.ioobj->key() );

	return false;
    }

    if ( indexer_ )
	presentReport( parent(), *indexer_->scanner() );

    return true;
}


void uiSEGYScanDlg::presentReport( uiParent* p, const SEGY::Scanner& sc,
				   const char* fnm )
{
    static const char* titl = "SEG-Y scan report";
    IOPar rep( titl );
    sc.getReport( rep );
    if ( sc.warnings().size() == 1 )
	rep.add( "Warning", sc.warnings().get(0) );
    else
    {
	for ( int idx=0; idx<sc.warnings().size(); idx++ )
	{
	    if ( !idx ) rep.add( IOPar::sKeyHdr(), "Warnings" );
	    rep.add( toString(idx+1), sc.warnings().get(idx) );
	}
    }

    if ( fnm && *fnm && !rep.write(fnm,IOPar::sKeyDumpPretty()) )
	uiMSG().warning( "Cannot write report to specified file" );

    uiDialog* dlg = new uiDialog( p,
	    	    uiDialog::Setup(titl,mNoDlgTitle,mNoHelpID).modal(false) );
    dlg->setCtrlStyle( uiDialog::LeaveOnly );
    std::ostringstream strstrm; rep.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, titl );
    te->setText( strstrm.str().c_str() );
    dlg->setDeleteOnClose( true ); dlg->go();
}
