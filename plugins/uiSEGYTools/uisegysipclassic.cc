/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
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
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "od_ostream.h"


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
    done( sr_->state() == uiSEGYRead::cCancelled() ?
			uiDialog::Rejected : uiDialog::Accepted );
}

    uiSEGYRead*				sr_;
    uiSEGYClassicSurvInfoProvider*	sip_;

};


uiDialog* uiSEGYClassicSurvInfoProvider::dialog( uiParent* p )
{
    uiDialog::Setup su( tr("Survey setup (SEG-Y)"), mNoDlgTitle, mNoHelpKey );
    su.oktext(uiString::emptyString()).canceltext(uiString::emptyString());
    xyinft_ = false;
    return new uiSEGYClassicSIPMgrDlg( this, p, su );
}

#define mShowErr(s) \
    uiMainWin* mw = uiMSG().setMainWin( uiMain::theMain().topLevel() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw);

void uiSEGYClassicSurvInfoProvider::showReport(
				    const SEGY::Scanner& scanner ) const
{
    BufferString fnm( GetProcFileName("SEGY_survey_classic_scan.txt" ) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	fnm.set( FilePath::getTempFullPath("segy_survinfo","txt") );
	strm.open( fnm );
    }
    if ( !strm.isOK() )
	{ mShowErr( tr("Cannot open temporary file:\n%1").arg(fnm) ); return; }

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

    uiString errmsg = scanner->posInfoDetector().getSurvInfoWithMsg(
							cs.hsamp_, crd );
    if ( !errmsg.isEmpty() )
	{ mShowErr( errmsg ); return false; }

    cs.zsamp_ = scanner->zRange();
    const SEGYSeisTrcTranslator* translator = scanner->translator();
    xyinft_ = translator && translator->binHeader().isInFeet();
    return true;
}


void uiSEGYClassicSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    uiSEGYRead::Setup srsu( uiSEGYRead::Import );
    srsu.initialstate_ = uiSEGYRead::SetupImport;
    new uiSEGYRead( p, srsu, &iop );
}


const char* uiSEGYClassicSurvInfoProvider::importAskQuestion() const
{
    Seis::GeomType gt = Seis::Vol;
    Seis::getFromPar( imppars_, gt );
    return gt == Seis::Vol ? "Import the scanned SEG-Y file(s) now?" : 0;
}


const uiString uiSEGYClassicSurvInfoProvider::importAskUiQuestion() const
{
    Seis::GeomType gt = Seis::Vol;
    Seis::getFromPar( imppars_, gt );
    return gt == Seis::Vol ? tr("Import the scanned SEG-Y file(s) now?")
			   : uiString::emptyString();
}
