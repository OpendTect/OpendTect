/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyscandlg.cc,v 1.22 2009-03-24 12:33:51 cvsbert Exp $";

#include "uisegyscandlg.h"

#include "uisegydef.h"
#include "uiseissel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "pixmap.h"

#include "segyfiledef.h"
#include "segyfiledata.h"
#include "segyscanner.h"
#include "segydirectdef.h"
#include "seispsioprov.h"
#include "filegen.h"
#include <sstream>


uiSEGYScanDlg::uiSEGYScanDlg( uiParent* p, const uiSEGYReadDlg::Setup& su,
				IOPar& iop, bool ss )
    : uiSEGYReadDlg(p,su,iop,ss)
    , scanner_(0)
    , forsurvsetup_(ss)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_,false))
    , outfld_(0)
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
	ctxt.deftransl = ctxt.trglobexpr = "SEGYDirect";
	uiSeisSel::Setup sssu( setup_.geom_ ); sssu.selattr( false );
	outfld_ = new uiSeisSel( this, ctio_, sssu );
	if ( optsgrp_ )
	    outfld_->attach( alignedBelow, optsgrp_ );
	else
	    attobj = outfld_->attachObj();

	if ( Seis::is2D(setup_.geom_) )
	{
	    outfld_->setConfirmOverwrite( false );
	    lnmfld_ = new uiSeisLineSel( this );
	    lnmfld_->attach( alignedBelow, outfld_ );
	}
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
    delete &ctio_;
}


SEGY::Scanner* uiSEGYScanDlg::getScanner()
{
    SEGY::Scanner* ret = scanner_;
    scanner_ = 0;
    return ret;
}


#define mErrRet(s1,s2) { uiMSG().error(s1,s2); return false; }


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
	    mErrRet("Please enter a name for the output data store file",0)

	pathnm = ctio_.ioobj->fullUserExpr( Conn::Write );
	if ( lnmfld_ )
	{
	    if ( !File_isDirectory(pathnm) )
	    {
		File_createDir(pathnm,0);
		if ( !File_isDirectory(pathnm) )
		    mErrRet("Cannot create directory for output:\n",pathnm)
	    }
	    if ( !File_isWritable(pathnm) )
		mErrRet("Output directory is not writable:\n",pathnm)
	}
	else
	{
	    if ( File_exists(pathnm) && !File_isWritable(pathnm) )
		mErrRet("Cannot overwrite output file:\n",pathnm)
	}
    }

    SEGY::FileSpec fs; fs.usePar( pars_ );
    delete scanner_; scanner_ = new SEGY::Scanner( fs, setup_.geom_, pars_ );
    if ( setup_.rev_ == uiSEGYRead::Rev0 )
	scanner_->setForceRev0( true );
    if ( forsurvsetup_ || Seis::is2D(setup_.geom_) )
	scanner_->setRichInfo( true );
    uiTaskRunner tr( parent_ );
    bool rv = tr.execute(*scanner_);
    if ( !rv ) return false;

    if ( !displayWarnings(scanner_->warnings(),outfld_) )
	return false;

    return outfld_ ? mkOutput( pathnm, lnm ) : true;
}


bool uiSEGYScanDlg::mkOutput( const char* pathnm, const char* lnm )
{
    const SEGY::FileDataSet& fds = scanner_->fileDataSet();
    if ( fds.isEmpty() )
	mErrRet("No files found",0)
    bool anydata = false;
    for ( int idx=0; idx<fds.size() ; idx++ )
    {
	if ( !fds[idx]->isEmpty() )
	    { anydata = true; break; }
    }
    if ( !anydata )
	mErrRet(fds.size() > 1 ? "No traces found in any of the files"
				: "No traces found in file",0)
    presentReport( parent(), *scanner_ );

    SEGY::DirectDef dd;
    dd.setData( fds, true );

    BufferString fnm;
    if ( *lnm ) fnm = SEGY::DirectDef::get2DFileName( pathnm, lnm );
    else	fnm = pathnm;

    if ( !dd.writeToFile( fnm ) )
    {
	uiMSG().error( "Cannot write data definition file to disk.\n"
			"You cannot use the data store." );
	return false;
    }

    if ( !Seis::is2D(setup_.geom_) )
	SPSIOPF().mk3DPostStackProxy( *ctio_.ioobj );
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
