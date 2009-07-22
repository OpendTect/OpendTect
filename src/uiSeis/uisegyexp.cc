/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyexp.cc,v 1.18 2009-07-22 16:01:41 cvsbert Exp $";

#include "uisegyexp.h"
#include "uisegydef.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uicompoundparsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seiswrite.h"
#include "seissingtrcproc.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uifiledlg.h"
#include "uitextedit.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "iostrm.h"
#include "ioman.h"
#include "oddirs.h"
#include "pixmap.h"
#include "filepath.h"
#include "filegen.h"

static const char* txtheadtxt =
"Define the SEG-Y text header. Note that:"
"\n- The Cnn line start and Rev.1 indicators will always be retained"
"\n- You can only define 40 lines of 80 characters";

class uiSEGYExpTxtHeaderDlg : public uiDialog
{
public:

uiSEGYExpTxtHeaderDlg( uiParent* p, BufferString& hdr, bool& ag )
    : uiDialog(p,Setup("Define SEG-Y Text Header",txtheadtxt,mTODOHelpID))
    , hdr_(hdr)
    , autogen_(ag)
{
    const CallBack cb( mCB(this,uiSEGYExpTxtHeaderDlg,agSel) );
    autogenfld_ = new uiGenInput( this, "Automatically generate",
	    			  BoolInpSpec(false) );
    autogenfld_->valuechanged.notify( cb );
    uiToolButton* wtb = new uiToolButton( this, "Write to file",
	    ioPixmap("saveset.png"), mCB(this,uiSEGYExpTxtHeaderDlg,writePush));
    wtb->attach( rightBorder );
    uiToolButton* rtb = new uiToolButton( this, "Read file",
	    ioPixmap("openset.png"), mCB(this,uiSEGYExpTxtHeaderDlg,readPush) );
    rtb->attach( leftOf, wtb );

    edfld_ = new uiTextEdit( this, "Hdr edit" );
    edfld_->setPrefWidthInChar( 81 );
    edfld_->setPrefHeightInChar( 24 );
    if ( hdr_.isEmpty() )
    {
	SEGY::TxtHeader th; th.clear();
	th.getText( hdr_ );
    }
    edfld_->setText( hdr_ );
    edfld_->attach( ensureBelow, autogenfld_ );
    finaliseDone.notify( cb );
}

void agSel( CallBacker* )
{
    edfld_->display( !autogenfld_->getBoolValue() );
}

void readPush( CallBacker* )
{
    FilePath fp( GetDataDir() ); fp.add( "Seismics" );
    uiFileDialog dlg( this, true, fp.fullPath() );
    if ( !dlg.go() ) return;

    if ( !File_exists(dlg.fileName()) )
	{ uiMSG().error("Cannot open file"); return; }

    edfld_->readFromFile( dlg.fileName(), 80 );
}

void writePush( CallBacker* )
{
    FilePath fp( GetDataDir() ); fp.add( "Seismics" );
    uiFileDialog dlg( this, false, fp.fullPath() );
    if ( !dlg.go() ) return;

    fp.set( dlg.fileName() );
    if ( !File_isWritable(fp.pathOnly()) )
	{ uiMSG().error("Cannot write to this directory"); return; }
    const BufferString fnm( fp.fullPath() );
    if ( File_exists(fnm) && !File_isWritable(fnm) )
	{ uiMSG().error("Cannot write to this file"); return; }

    if ( !edfld_->saveToFile(fnm,80,false) )
	{ uiMSG().error("Failed to write to this file"); return; }
}

bool acceptOK( CallBacker* )
{
    autogen_ = autogenfld_->getBoolValue();
    if ( !autogen_ )
	hdr_ = edfld_->text();
    return true;
}

    BufferString&	hdr_;
    bool&		autogen_;

    uiGenInput*		autogenfld_;
    uiTextEdit*		edfld_;

};

class uiSEGYExpTxtHeader : public uiCompoundParSel
{
public:

uiSEGYExpTxtHeader( uiSEGYExp* se )
    : uiCompoundParSel(se,"Text header","Define")
    , se_(se)
{
    butPush.notify( mCB(this,uiSEGYExpTxtHeader,butPushed) );
}

void butPushed( CallBacker* )
{
    uiSEGYExpTxtHeaderDlg dlg( this, se_->hdrtxt_, se_->autogentxthead_ );
    dlg.go();
}

BufferString getSummary() const
{
    if ( se_->autogentxthead_ )
	return BufferString( "<generate>" );
    else
	return BufferString( "<user-defined>" );
}

    uiSEGYExp*	se_;

};


uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
	: uiDialog(p,uiDialog::Setup("SEG-Y I/O","Export to SEG-Y","103.0.7"))
	, ctio_(*mMkCtxtIOObj(SeisTrc))
    	, geom_(gt)
    	, morebut_(0)
    	, autogentxthead_(true)
{
    seissel_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(geom_) );
    seissel_->selectiondone.notify( mCB(this,uiSEGYExp,inpSel) );

    uiSeisTransfer::Setup tsu( geom_ );
    tsu.withnullfill(true).fornewentry(false).onlyrange(false);
    transffld_ = new uiSeisTransfer( this, tsu );
    transffld_->attach( alignedBelow, seissel_ );

    fpfld_ = new uiSEGYFilePars( this, false );
    fpfld_->attach( alignedBelow, transffld_ );

    txtheadfld_ = new uiSEGYExpTxtHeader( this );
    txtheadfld_->attach( alignedBelow, fpfld_ );

    uiSEGYFileSpec::Setup su; su.forread(false).canbe3d(!Seis::is2D(geom_));
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfld_ );

    if ( Seis::is2D(geom_) )
    {
	morebut_ = new uiCheckBox( this, "Export more from same Line Set" );
	morebut_->attach( alignedBelow, fsfld_ );
    }
}


uiSEGYExp::~uiSEGYExp()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSEGYExp::inpSel( CallBacker* )
{
    if ( seissel_->commitInput() )
	transffld_->updateFrom( *ctio_.ioobj );
}


class uiSEGYExpMore : public uiDialog
{
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi, const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-export",
		    		     "Specify file details","103.0.7"))
	, inioobj_(ii)
	, outioobj_(oi)
	, segyexp_(p)
	, attrnm_(anm)
{
    setHelpID( "103.0.7" );
    const BufferString fnm( outioobj_.fullUserExpr(false) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Exp " ); setupnm += uiSEGYFileSpec::sKeyLineNmToken;

    uiLabel* lbl = 0;
    const bool isrealattrib = strcmp(attrnm_,LineKey::sKeyDefAttrib());
    if ( isrealattrib )
    {
	BufferString lbltxt( "Attribute: " );
	lbltxt += attrnm_;
	lbl = new uiLabel( this, lbltxt );
    }

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Lines to export",
						    true );
    lnmsfld_ = llb->box();
    SeisIOObjInfo sii( inioobj_ );
    BufferStringSet lnms;
    sii.getLineNamesWithAttrib( attrnm_, lnms );
    for ( int idx=0; idx<lnms.size(); idx++ )
	lnmsfld_->addItem( lnms.get(idx) );
    lnmsfld_->selectAll();
    if ( lbl )
	llb->attach( alignedBelow, lbl );

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken );
    if ( isrealattrib )
    {
	setupnm += " ("; setupnm += attrnm_; setupnm += ")";
	BufferString clnattrnm( attrnm_ );
	cleanupString( clnattrnm.buf(), mC_False, mC_False, mC_True );
	newfnm += "_"; newfnm += clnattrnm;
    }
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Output (Line name replaces '" );
    txt += uiSEGYFileSpec::sKeyLineNmToken; txt += "')";

    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(false) );
    fnmfld_->attach( alignedBelow, llb );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File_isDirectory(dirnm) )
	File_createDir( dirnm, 0 );
    if ( !File_isDirectory(dirnm) || !File_isWritable(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken; msg += "'\n";
	msg += "That will then be replaced by the line name";
	uiMSG().error( msg );
	return false;
    }

    IOM().to( inioobj_.key() );
    return doExp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = outioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    const IOObj& in = inioobj_; const IOObj& out = *newioobj;
    bool res = segyexp_->doWork( in, out, lnm, attrnm_ );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askContinue("Continue with next?") )
	    return false;
    }
    return true;
}

bool doExp( const FilePath& fp )
{
    BufferStringSet lnms;
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( lnmsfld_->isSelected(idx) )
	    lnms.add( lnmsfld_->textOfItem(idx) );
    }
    if ( lnms.size() < 1 )
    {
	uiMSG().error( "Please select lines to export" );
	return false;
    }

    bool nofails = true;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString& lnm = *lnms[idx];
	BufferString filenm( fp.fullPath() );
	replaceString( filenm.buf(), uiSEGYFileSpec::sKeyLineNmToken, lnm );
	IOObj* newioobj = getSubstIOObj( filenm );
	if ( !doWork( newioobj, lnm, idx > lnms.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiListBox*		lnmsfld_;
    uiSEGYExp*		segyexp_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;
    const char*		attrnm_;

};


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK( CallBacker* )
{
    if ( !seissel_->commitInput() )
    {
	uiMSG().error( "Please select the data to export" );
	return false;
    }

    const IOObj* inioobj = ctio_.ioobj;
    PtrMan<IOObj> outioobj = fsfld_->getSpec().getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );
    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIs2D(), is2d );

    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : 0;
    if ( !morebut_ || !morebut_->isChecked() )
	return doWork( *inioobj, *outioobj, lnm, attrnm );
    else
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj, attrnm );
	return dlg.go();
    }
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    SEGY::TxtHeader::info2D() = is2d;
    Executor* exec = transffld_->getTrcProc( inioobj, outioobj,
				    "Export seismic data", "Putting traces",
				    attrnm, linenm );
    if ( !exec )
	return false;
    PtrMan<Executor> execptrman = exec;

    mDynamicCastGet(SeisSingleTraceProc*,sstp,exec)
    if ( sstp && !autogentxthead_ && !hdrtxt_.isEmpty() )
    {
	const SeisTrcWriter* wrr = sstp->writer();
	SeisTrcTranslator* tr =
	    	const_cast<SeisTrcTranslator*>(wrr->seisTranslator());
	mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,tr)
	if ( segytr )
	{
	    SEGY::TxtHeader* th = new SEGY::TxtHeader;
	    th->setText( hdrtxt_ );
	    segytr->setTxtHeader( th );
	}
    }

    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( exec->name() );
	nm += " ("; nm += linenm; nm += ")";
	exec->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *exec );
    execptrman.erase();

    SEGY::TxtHeader::info2D() = false;
    return rv;
}
