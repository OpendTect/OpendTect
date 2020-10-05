/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegyscandlg.h"

#include "datainpspec.h"
#include "dbman.h"
#include "keystrs.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "uigeninput.h"
#include "uisegydef.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uibatchjobdispatchersel.h"

#include "segyfiledef.h"
#include "segyfiledata.h"
#include "segyscanner.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segydirect2d.h"
#include "seispsioprov.h"
#include "file.h"
#include "od_strstream.h"


uiSEGYScanDlg::uiSEGYScanDlg( uiParent* p, const uiSEGYReadDlg::Setup& su,
			      IOPar& iop, bool ss )
    : uiSEGYReadDlg(p,su,iop,ss)
    , scanner_(0)
    , forsurvsetup_(ss)
    , outfld_(0)
    , lnmfld_(0)
{
    uiObject* attobj = 0;
    if ( setup_.dlgtitle_.isEmpty() )
    {
	SEGY::FileSpec fs; fs.usePar( iop );
	uiString ttl( tr("Scan %1 %2").arg(Seis::nameOf( setup_.geom_ ))
				      .arg(fs.dispName()) );
	setTitleText( ttl );
    }

    if ( forsurvsetup_ )
    {
	if ( !optsfld_ )
	    attobj = new uiLabel( this,
			tr("Press Go or hit enter to start SEG-Y scan"));
    }
    else
    {
	IOObjContext ctxt = uiSeisSel::ioContext( su.geom_, false );
	ctxt.toselect_.allownonuserselectable_ = true;
	ctxt.fixTranslator( SEGYDirectSeisTrcTranslator::translKey() );
	uiSeisSel::Setup sssu( setup_.geom_ );
	sssu.withwriteopts( false );
	outfld_ = new uiSeisSel( this, ctxt, sssu );
	if ( optsfld_ )
	    outfld_->attach( alignedBelow, optsfld_ );
	else
	    attobj = outfld_->attachObj();

	if ( Seis::is2D(setup_.geom_) )
	{
	    outfld_->setConfirmOverwrite( false );
	    lnmfld_ = new uiSeis2DLineNameSel( this, false );
	    lnmfld_->attach( alignedBelow, outfld_ );
	}

	batchfld_ = new uiBatchJobDispatcherSel( this, false,
						 Batch::JobSpec::SEGY );
	batchfld_->setJobName( "scan SEG-Y" );
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.set( SEGY::IO::sKeyTask(), Seis::isPS(setup_.geom_)
		? SEGY::IO::sKeyIndexPS() : SEGY::IO::sKeyIndex3DVol() );
	js.pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(setup_.geom_) );
	batchfld_->attach( alignedBelow,
		lnmfld_ ? lnmfld_->attachObj() : outfld_->attachObj() );
    }

    if ( attobj )
    {
	uiToolButton* tb = new uiToolButton( this, "prescan",
					tr("Limited Pre-scan"),
				       mCB(this,uiSEGYScanDlg,preScanCB) );
	tb->attach( rightTo, attobj ); tb->attach( rightBorder );
    }
}


uiSEGYScanDlg::~uiSEGYScanDlg()
{
    delete scanner_;
}


SEGY::Scanner* uiSEGYScanDlg::getScanner()
{
    SEGY::Scanner* ret = scanner_;
    scanner_ = 0;
    return ret;
}


#define mErrRet(s1,s2)  \
{			 \
    if ( !s1.isEmpty() ) uiMSG().error(s1,s2); \
    return false;	  \
}


bool uiSEGYScanDlg::doWork( const IOObj& )
{
    BufferString pathnm, lnm;
    if ( outfld_ )
    {
	if ( lnmfld_ )
	{
	    lnm = lnmfld_->getInput();
	    if ( lnm.isEmpty() )
		mErrRet(uiStrings::phrSelect(tr("the line name")),
						    uiString::empty())
	}

        if ( !outfld_->commitInput() )
	{
	    if ( !outfld_->isEmpty() )
		mErrRet(uiString::empty(),uiString::empty())
	    else if ( Seis::isPS( setup_.geom_ ) )
		mErrRet(uiStrings::phrEnter(
			tr("a name for the output data store scan")),
			uiString::empty())
	    else
		mErrRet(uiStrings::phrEnter(
			tr("a name for the output cube scan")),
			uiString::empty())
	}

	pathnm = outfld_->ioobj(true)->mainFileName();
	if ( lnmfld_ )
	{
	    if ( !File::isDirectory(pathnm) )
	    {
		File::createDir(pathnm);
		if ( !File::isDirectory(pathnm) )
		    mErrRet(uiStrings::phrCannotCreate(
			    tr("directory for output:\n")),toUiString(pathnm))
	    }
	    if ( !File::isWritable(pathnm) )
		mErrRet(uiStrings::phrOutput(
			tr("directory is not writable:\n")),toUiString(pathnm))
	}
	else
	{
	    if ( File::exists(pathnm) && !File::isWritable(pathnm) )
		mErrRet(tr("Cannot overwrite output file:\n"),
			toUiString(pathnm))
	}
    }

    SEGY::FileSpec fs;
    fs.usePar( pars_ );

    Executor* exec = 0;
    delete scanner_; scanner_ = 0;

    if ( outfld_ )
    {
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.merge( pars_ );
	js.pars_.set( sKey::Output(), outfld_->key(true) );
	if ( lnmfld_ )
	{
	    Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
	    if ( mIsUdfGeomID(geomid) )
	    {
		PtrMan<IOObj> geomobj = SurvGeom2DTranslator::getEntry( lnm,
				SEGYDirectSurvGeom2DTranslator::translKey() );
		if ( !geomobj )
		    mErrRet(uiStrings::phrCannotCreate(
			    tr("geometry entry for 2D line")),toUiString(lnm))

		geomobj->pars().set(
			SEGYDirectSurvGeom2DTranslator::sKeySEGYDirectID(),
			outfld_->key(true) );
		geomobj->commitChanges();
		geomid = SurvGeom2DTranslator::getGeomID( *geomobj );
	    }

	    js.pars_.set( sKey::GeomID(), geomid );
	}

	return batchfld_->start();
    }

    exec = scanner_ = new SEGY::Scanner( fs, setup_.geom_, pars_ );

    if ( setup_.rev_ == uiSEGYRead::Rev0 )
	scanner_->setForceRev0( true );
    if ( forsurvsetup_ )
	scanner_->setRichInfo( true );

    uiTaskRunner taskrunner( parent_ );
    bool rv = TaskRunner::execute( &taskrunner, *exec );
    if ( !rv )
    {
	if ( outfld_ )
	    DBM().removeEntry( outfld_->key(true) );
	return false;
    }

    const bool haveoutput = (bool)outfld_;
    if ( !uiSEGY::displayWarnings(this,scanner_->warnings(),haveoutput) )
    {
	if ( haveoutput )
	    DBM().removeEntry( outfld_->key(true) );
	return false;
    }

    IOPar rep( "SEG-Y scan report" ); scanner_->getReport( rep );
    uiSEGY::displayReport( parent(), rep );
    return true;
}


DBKey uiSEGYScanDlg::outputID() const
{
    return outfld_ ? outfld_->key(true) : DBKey::getInvalid();
}
