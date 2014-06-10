/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegysip.h"
#include "uisegyread.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimain.h"
#include "segyscanner.h"
#include "segytr.h"
#include "segyhdr.h"
#include "posinfodetector.h"
#include "cubesampling.h"
#include "ptrman.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "od_ostream.h"


class uiSEGYSIPMgrDlg : public uiDialog
{
public:

uiSEGYSIPMgrDlg( uiSEGYSurvInfoProvider* sip, uiParent* p,
		 const uiDialog::Setup& su )
    : uiDialog(p,su)
    , sip_(sip)
{
    new uiLabel( this, "To be able to scan your data\n"
	    "You must define the specific properties of your SEG-Y file(s)" );
    afterPopup.notify( mCB(this,uiSEGYSIPMgrDlg,start) );
}

void start( CallBacker* )
{
    uiSEGYRead::Setup srsu( uiSEGYRead::SurvSetup );
    sr_ = new uiSEGYRead( this, srsu );
    sr_->processEnded.notify( mCB(this,uiSEGYSIPMgrDlg,atEnd) );
}

void atEnd( CallBacker* )
{
    sr_->fillPar( sip_->imppars_ );
    done( sr_->state() != uiSEGYRead::cCancelled() ? 1 : 0 );
}

    uiSEGYRead*			sr_;
    uiSEGYSurvInfoProvider*	sip_;

};


uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    uiDialog::Setup su( "Survey setup (SEG-Y)", mNoDlgTitle, mNoHelpKey );
    su.oktext("").canceltext("");
    xyinft_ = false;
    return new uiSEGYSIPMgrDlg( this, p, su );
}

#define mShowErr(s) \
    uiMainWin* mw = uiMSG().setMainWin( uiMain::theMain().topLevel() ); \
    uiMSG().error(s); \
    uiMSG().setMainWin(mw);

static void showReport( const SEGY::Scanner& scanner )
{
    BufferString fnm( GetProcFileName("SEGY_survey_scan.txt" ) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	fnm.set( FilePath::getTempName() );
	strm.open( fnm );
    }
    if ( !strm.isOK() )
	{ mShowErr(BufferString("Cannot open temporary file:\n",fnm)); return; }
    IOPar iop;
    scanner.getReport( iop );
    if ( !iop.write(strm,IOPar::sKeyDumpPretty()) )
	{ mShowErr(BufferString("Cannot write to file:\n",fnm)); return; }

    File::launchViewer( fnm );
}


bool uiSEGYSurvInfoProvider::getInfo( uiDialog* d, CubeSampling& cs,
				      Coord crd[3] )
{
    if ( !d ) return false;
    mDynamicCastGet(uiSEGYSIPMgrDlg*,dlg,d)
    if ( !dlg ) { pErrMsg("Huh?"); return false; }

    PtrMan<SEGY::Scanner> scanner = dlg->sr_->getScanner();
    if ( !scanner ) { pErrMsg("Huh?"); return false; }

    showReport( *scanner );

    if ( Seis::is2D(scanner->geomType()) )
	return false;

    const char* errmsg = scanner->posInfoDetector().getSurvInfo(cs.hrg,crd);
    if ( errmsg && *errmsg )
    {	mShowErr( errmsg ); return false; }

    cs.zrg = scanner->zRange();
    const SEGYSeisTrcTranslator* tr = scanner->translator();
    xyinft_ = tr && tr->binHeader().isInFeet();
    return true;
}


void uiSEGYSurvInfoProvider::startImport( uiParent* p, const IOPar& iop )
{
    uiSEGYRead::Setup srsu( uiSEGYRead::Import );
    srsu.initialstate_ = uiSEGYRead::SetupImport;
    new uiSEGYRead( p, srsu, &iop );
}


const char* uiSEGYSurvInfoProvider::importAskQuestion() const
{
    Seis::GeomType gt = Seis::Vol;
    Seis::getFromPar( imppars_, gt );
    return gt == Seis::Vol ? "Import the scanned SEG-Y file(s) now?" : 0;
}
