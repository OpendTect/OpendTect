/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uisegysip.h"
#include "uisegyread.h"
#include "uilabel.h"
#include "uimsg.h"
#include "segyscanner.h"
#include "segytr.h"
#include "segyhdr.h"
#include "posinfodetector.h"
#include "cubesampling.h"
#include "ptrman.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "errh.h"
#include "oddirs.h"
#include "strmprov.h"
#include "timer.h"


class uiSEGYSIPMgrDlg : public uiDialog
{
public:

uiSEGYSIPMgrDlg( uiSEGYSurvInfoProvider* sip, uiParent* p,
		 const uiDialog::Setup& su )
    : uiDialog(p,su)
    , sip_(sip)
    , timer_(*new Timer("Next dialog"))
{
    new uiLabel( this, "To be able to scan your data\n"
	    "You must define the specific properties of your SEG-Y file(s)" );
    timer_.tick.notify( mCB(this,uiSEGYSIPMgrDlg,start) );
    timer_.start( 500, true );
}

~uiSEGYSIPMgrDlg()
{
    delete &timer_;
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
    Timer&			timer_;

};


uiDialog* uiSEGYSurvInfoProvider::dialog( uiParent* p )
{
    uiDialog::Setup su( "Survey setup (SEG-Y)", mNoDlgTitle, mNoHelpID );
    su.oktext("").canceltext("");
    xyinft_ = false;
    return new uiSEGYSIPMgrDlg( this, p, su );
}

#define mErrRet(s) { uiMSG().error(s); return; }

static void showReport( const SEGY::Scanner& scanner )
{
    const BufferString fnm( GetProcFileName("SEGY_survey_scan.txt" ) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet("Cannot open temporary file in Proc directory")
    IOPar iop; 
    scanner.getReport( iop );
    if ( !iop.write(*sd.ostrm,IOPar::sKeyDumpPretty()) )
    {
	sd.close();
	mErrRet("Cannot write to temporary file in Proc directory")
    }

    sd.close();
    ExecuteScriptCommand( "od_FileBrowser", fnm );
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
	{ uiMSG().error( errmsg ); return false; }

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
