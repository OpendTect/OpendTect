/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2004
________________________________________________________________________

-*/

#include "uisegysipclassic.h"
#include "uisegyread.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimain.h"
#include "segyscanner.h"
#include "segytr.h"
#include "segyhdr.h"
#include "posinfodetector.h"
#include "trckeyzsampling.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "oddirs.h"
#include "fileview.h"
#include "filepath.h"
#include "od_ostream.h"
#include "uistrings.h"


class uiSEGYClassicSIPMgrDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYClassicSIPMgrDlg)
public:

uiSEGYClassicSIPMgrDlg( uiSEGYClassicSurvInfoProvider* sip, uiParent* p,
			 const uiDialog::Setup& su )
    : uiDialog(p,su)
    , sip_(sip)
{
    new uiLabel( this, tr("To be able to scan your data\n"
	    "You must define the specific properties of your SEG-Y file(s)") );
    afterPopup.notify( mCB(this,uiSEGYClassicSIPMgrDlg,start) );
}

void start( CallBacker* )
{
    uiSEGYRead::Setup srsu( uiSEGYRead::SurvSetup );
    sr_ = new uiSEGYRead( this, srsu );
    sr_->processEnded.notify( mCB(this,uiSEGYClassicSIPMgrDlg,atEnd) );
}

void atEnd( CallBacker* )
{
    sr_->fillPar( sip_->imppars_ );
    done( sr_->state() != uiSEGYRead::cCancelled() ? Accepted : Rejected );
}

    uiSEGYRead*				sr_;
    uiSEGYClassicSurvInfoProvider*	sip_;

};


uiDialog* uiSEGYClassicSurvInfoProvider::dialog( uiParent* p, TDInfo )
{
    uiDialog::Setup su( tr("Survey setup (SEG-Y)"), mNoDlgTitle, mNoHelpKey );
    su.oktext(uiString::empty()).canceltext(uiString::empty());
    xyinft_ = false;
    return new uiSEGYClassicSIPMgrDlg( this, p, su );
}


#define mShowErr(s) gUiMsg().error(s); \

void uiSEGYClassicSurvInfoProvider::showReport(
				    const SEGY::Scanner& scanner ) const
{
    BufferString fnm( GetProcFileName("SEGY_survey_classic_scan.txt" ) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	fnm.set( File::Path::getTempFullPath("segy_survinfo","txt") );
	strm.open( fnm );
    }
    if ( !strm.isOK() )
	{ mShowErr( uiStrings::phrCannotCreateTempFile() ); return; }

    IOPar iop;
    scanner.getReport( iop );
    if ( !iop.write(strm,IOPar::sKeyDumpPretty()) )
	{ mShowErr(tr("Cannot write to file:\n%1").arg(fnm)); return; }

    File::launchViewer( fnm );
}


bool uiSEGYClassicSurvInfoProvider::getInfo( uiDialog* d, TrcKeyZSampling& cs,
				      Coord crd[3] )
{
    if ( !d )
	return false;
    mDynamicCastGet(uiSEGYClassicSIPMgrDlg*,dlg,d)
    if ( !dlg )
	{ pErrMsg("Huh?"); return false; }
    else if ( dlg->uiResult() != 1 )
	return false; // cancelled

    PtrMan<SEGY::Scanner> scanner = dlg->sr_->getScanner();
    if ( !scanner )
	{ pErrMsg("Huh?"); return false; }

    showReport( *scanner );

    if ( Seis::is2D(scanner->geomType()) )
	return false;

    uiString errmsg = scanner->posInfoDetector().getSurvInfo(cs.hsamp_,crd);
    if ( !errmsg.isEmpty() )
	{ mShowErr( errmsg ); return false; }

    cs.zsamp_ = scanner->zRange();
    userfilename_ = scanner->fileSpec().usrStr();
    const SEGYSeisTrcTranslator* transl = scanner->translator();
    xyinft_ = transl && transl->binHeader().isInFeet();
    return true;
}


void uiSEGYClassicSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    uiSEGYRead::Setup srsu( uiSEGYRead::Import );
    srsu.initialstate_ = uiSEGYRead::SetupImport;
    new uiSEGYRead( p, srsu, &iop );
}


uiString uiSEGYClassicSurvInfoProvider::importAskQuestion() const
{
    Seis::GeomType gt = Seis::Vol;
    Seis::getFromPar( imppars_, gt );
    return gt == Seis::Vol ? tr("Import the scanned SEG-Y file(s) now?")
			   : uiString::empty();
}
