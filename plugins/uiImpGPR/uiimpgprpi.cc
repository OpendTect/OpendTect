/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uiimpgprpi.cc,v 1.1 2010-03-11 16:17:24 cvsbert Exp $";

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uiseissel.h"
#include "uitaskrunner.h"
#include "executor.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "strmprov.h"
#include "survinfo.h"
#include "plugins.h"

static const char* menunm = "&GPR: DTZ";


mExternC int GetuiImpGPRPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiImpGPRPluginInfo()
{
    static PluginInfo retpi = {
	"GPR: .DTZ import",
	"Bert Bril/Matthias Schuh",
	"0.0.0",
	"Imports GPR data in DTZ format.\n" };
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiImpGPRMgr :  public CallBacker
{
public:

			uiImpGPRMgr(uiODMain&);

    uiODMain&		appl_;
    void		doWork(CallBacker*);
};


uiImpGPRMgr::uiImpGPRMgr( uiODMain& a )
	: appl_(a)
{
    appl_.menuMgr().getMnu( true, uiODApplMgr::Seis )->
		insertItem(new uiMenuItem(menunm,mCB(this,uiImpGPRMgr,doWork)));
}

#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }
#define mStrm (*sd_.istrm)

class ImpGPRMgr : public Executor
{
public:

ImpGPRMgr( const char* fnm, const IOObj& ioobj, const LineKey& lk )
    : Executor("Importing DTZ file")
    , msg_("Handling traces")
    , nrdone_(0)
    , totalnr_(-1) // default: -1 => unknown
    , nrsamples_(-1)
    , lk_(lk)
{
    sd_ = StreamProvider( fnm ).makeIStream();
    if ( !sd_.usable() ) return;

    //TODO 1: get file info like trace size, and sampling.
    // For test, I'll read a 'Simple File' header
    mStrm >> trc_.info().sampling.start >> trc_.info().sampling.step
    	  >> nrsamples_;
    if ( SI().zIsTime() )
    {
	trc_.info().sampling.start *= 0.001;
	trc_.info().sampling.step *= 0.001;
    }

    if ( nrsamples_ < 1 )
    {
	msg_ = "Header problem: found ";
	msg_.add( nrsamples_ ).add( " samples" );
	return;
    }
    else
    {
	trc_.reSize( nrsamples_, false );
	// If you know the total nr of traces, you can set totalnr_ to that
    }
    // End TODO 1

    wrr_ = new SeisTrcWriter( &ioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd->setIsAll( true );
    rsd->lineKey() = lk;
    wrr_ = new SeisTrcWriter( &ioobj );
    wrr_->setSelData( rsd );
}

const char* message() const	{ return msg_; }
const char* nrDoneText() const	{ return "Traces handled"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

int nextStep()
{
    if ( nrsamples_ < 1 )
    {
	if ( nrsamples_ < 0 )
	    mErrRet("Cannot open input file")
	else
	    return ErrorOccurred();
    }

    //TODO 2: Read a new trace
    // For test, reading a specific Simple File format
    int trcnr = 0;
    mStrm >> trcnr; if ( trcnr == 0 ) return Finished();
    trc_.info().nr = trcnr;
    mStrm >> trc_.info().coord.x >> trc_.info().coord.y;
    float val;
    for ( int isamp=0; isamp<nrsamples_; isamp++ )
    {
	mStrm >> val;
	trc_.set( isamp, val, 0 );
    }
    // End TODO 2

    if ( !wrr_->put(trc_) )
	mErrRet(wrr_->errMsg())

    nrdone_++;
    return mStrm.good() ? MoreToDo() :  Finished();
}

    SamplingData<float>	sampling_;
    int			nrsamples_;
    SeisTrc		trc_;
    SeisTrcWriter*	wrr_;
    StreamData		sd_;
    LineKey		lk_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    BufferString	msg_;

};


#undef mErrRet
#define mErrRet(s) { uiMSG().error(s); return false; }

class uiGPRImporter : public uiDialog
{
public:

uiGPRImporter( uiParent* p )
    : uiDialog(p,Setup("Import GPR Seismics","Import DTZ Seismics",mNoHelpID))
    , inpfld_(0)
{
    if ( !SI().has2D() )
	{ new uiLabel(this,"TODO: implement 3D loading"); }

    uiFileInput::Setup fisu( uiFileDialog::Gen );
    fisu.filter( "*.dtz" ).forread( true );
    inpfld_ = new uiFileInput( this, "Input DTZ file", fisu );

    lnmfld_ = new uiGenInput( this, "Output Line name" );
    lnmfld_->attach( alignedBelow, inpfld_ );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( Seis::Line, false, ctxt );
    outfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Line) );
    outfld_->setAttrNm( "GPR" );
    outfld_->attach( alignedBelow, lnmfld_ );
}

bool acceptOK( CallBacker* )
{
    if ( !inpfld_ ) return true;

    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) mErrRet("Please enter the input file name")
    const BufferString lnm( lnmfld_->text() );
    if ( lnm.isEmpty() ) mErrRet("Please enter the output line name")

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj ) return false;

    uiTaskRunner tr( this );
    ImpGPRMgr importer( fnm, *ioobj, LineKey(lnm,outfld_->attrNm()) );
    return tr.execute( importer );
}

    uiFileInput*	inpfld_;
    uiGenInput*		lnmfld_;
    uiSeisSel*		outfld_;

};


void uiImpGPRMgr::doWork( CallBacker* )
{
    uiGPRImporter dlg( &appl_ );
    dlg.go();
}


mExternC const char* InituiImpGPRPlugin( int, char** )
{
    (void)new uiImpGPRMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}
