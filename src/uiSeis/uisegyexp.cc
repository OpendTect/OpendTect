/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisegyexp.cc,v 1.44 2012/05/22 10:17:26 cvsbert Exp $";

#include "uisegyexp.h"
#include "uisegydef.h"
#include "uisegymanip.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uicompoundparsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seissingtrcproc.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uifiledlg.h"
#include "uiselsimple.h"
#include "uitextedit.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "iostrm.h"
#include "ioman.h"
#include "oddirs.h"
#include "filepath.h"
#include "file.h"
#include "zdomain.h"
#include "strmprov.h"

static const char* txtheadtxt =
"Define the SEG-Y text header. Note that:"
"\n- The Cnn line start and Rev.1 indicators will always be retained"
"\n- You can only define 40 lines of 80 characters";

class uiSEGYExpTxtHeaderDlg : public uiDialog
{
public:

uiSEGYExpTxtHeaderDlg( uiParent* p, BufferString& hdr, bool& ag )
    : uiDialog(p,Setup("Define SEG-Y Text Header",txtheadtxt,"103.0.3"))
    , hdr_(hdr)
    , autogen_(ag)
{
    const CallBack cb( mCB(this,uiSEGYExpTxtHeaderDlg,agSel) );
    autogenfld_ = new uiGenInput( this, "Automatically generate",
	    			  BoolInpSpec(false) );
    autogenfld_->valuechanged.notify( cb );
    uiToolButton* wtb = new uiToolButton( this, "saveset.png", "Write to file",
			    mCB(this,uiSEGYExpTxtHeaderDlg,writePush));
    wtb->attach( rightBorder );
    uiToolButton* rtb = new uiToolButton( this, "openset.png", "Read file",
			    mCB(this,uiSEGYExpTxtHeaderDlg,readPush) );
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
    postFinalise().notify( cb );
}

void agSel( CallBacker* )
{
    edfld_->display( !autogenfld_->getBoolValue() );
}

void readPush( CallBacker* )
{
    uiFileDialog dlg( this, true, FilePath(GetDataDir(),"Seismics").fullPath());
    if ( !dlg.go() ) return;

    StreamData sd( StreamProvider(dlg.fileName()).makeIStream() );
    if ( !sd.usable() )
	{ uiMSG().error("Cannot open file"); return; }

    SEGY::TxtHeader txthdr;
    sd.istrm->read( (char*)txthdr.txt_, SegyTxtHeaderLength );
    sd.close();
    txthdr.setAscii();
    BufferString txt; txthdr.getText( txt );
    edfld_->setText( txt );
}

void writePush( CallBacker* )
{
    FilePath fp( GetDataDir(), "Seismics" );
    uiFileDialog dlg( this,false, fp.fullPath());
    if ( !dlg.go() ) return;

    fp.set( dlg.fileName() );
    if ( !File::isWritable(fp.pathOnly()) )
	{ uiMSG().error("Cannot write to this directory"); return; }
    const BufferString fnm( fp.fullPath() );
    if ( File::exists(fnm) && !File::isWritable(fnm) )
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
	, Usage::Client("SEG-Y")
    	, geom_(gt)
    	, morebox_(0)
    	, manipbox_(0)
    	, autogentxthead_(true)
	, selcomp_(-1)
{
    prepUsgStart( "Export" ); sendUsgInfo();
    setCtrlStyle( DoAndStay );
    const CallBack inpselcb( mCB(this,uiSEGYExp,inpSel) );
    
    PtrMan<CtxtIOObj> ctio = uiSeisSel::mkCtxtIOObj( geom_, true );
    uiSeisSel::Setup sssu( geom_ ); sssu.steerpol(uiSeisSel::Setup::InclSteer);
    seissel_ = new uiSeisSel( this, ctio->ctxt, sssu );
    seissel_->selectionDone.notify( inpselcb );

    uiSeisTransfer::Setup tsu( geom_ );
    tsu.withnullfill(true).fornewentry(false).onlyrange(false);
    transffld_ = new uiSeisTransfer( this, tsu );
    transffld_->attach( alignedBelow, seissel_ );

    fpfld_ = new uiSEGYFilePars( this, false );
    fpfld_->attach( alignedBelow, transffld_ );

    txtheadfld_ = new uiSEGYExpTxtHeader( this );
    txtheadfld_->attach( alignedBelow, fpfld_ );

    uiSEGYFileSpec::Setup su( geom_ != Seis::Line );
    su.forread( false ).canbe3d( !Seis::is2D(geom_) );
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfld_ );

    if ( Seis::is2D(geom_) && !Seis::isPS(geom_) )
    {
	morebox_ = new uiCheckBox( this, "Export more from same Line Set" );
	morebox_->attach( alignedBelow, fsfld_ );
    }
    else
    {
	manipbox_ = new uiCheckBox( this,
			"Manipulate output file after creation" );
	manipbox_->attach( alignedBelow, fsfld_ );
    }

    postFinalise().notify( inpselcb );
}


uiSEGYExp::~uiSEGYExp()
{
    prepUsgEnd(); sendUsgInfo();
}


void uiSEGYExp::inpSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj(true);
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


class uiSEGYExpMore : public uiDialog
{
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi, const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-export",
		    		     "Specify file details","103.0.2"))
	, inioobj_(ii)
	, outioobj_(oi)
	, segyexp_(p)
	, attrnm_(anm)
{
    const BufferString fnm( outioobj_.fullUserExpr(false) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Exp " ); 
    setupnm += uiSEGYFileSpec::sKeyLineNmToken();

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

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    if ( isrealattrib )
    {
	setupnm += " ("; setupnm += attrnm_; setupnm += ")";
	BufferString clnattrnm( attrnm_ );
	cleanupString( clnattrnm.buf(), false, false, true );
	newfnm += "_"; newfnm += clnattrnm;
    }
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Output (Line name replaces '" );
    txt += uiSEGYFileSpec::sKeyLineNmToken(); txt += "')";

    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(false) );
    fnmfld_->attach( alignedBelow, llb );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File::isDirectory(dirnm) )
	File::createDir( dirnm );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken()) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken(); msg += "'\n";
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
	replaceString( filenm.buf(), uiSEGYFileSpec::sKeyLineNmToken(), lnm );
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
    const IOObj* inioobj = seissel_->ioobj(true);
    if ( !inioobj )
	{ uiMSG().error( "Please select the data to export" ); return false; }
    const SEGY::FileSpec sfs( fsfld_->getSpec() );
    if ( sfs.fname_.isEmpty() )
	{ uiMSG().error( "Please select the output file" ); return false; }

    PtrMan<IOObj> outioobj = sfs.getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );
    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIs2D(), is2d );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIsPS(), Seis::isPS(geom_) );

    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : 0;
    bool needinfo = false;
    if ( morebox_ && morebox_->isChecked() )
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj, attrnm );
	dlg.go();
    }
    else
    {
	bool result = doWork( *inioobj, *outioobj, lnm, attrnm );
	if ( !result || !manipbox_ || !manipbox_->isChecked() )
	    needinfo = result;
	else
	{
	    uiSEGYFileManip dlg( this, outioobj->fullUserExpr(false) );
	    dlg.go();
	}
    }

    if ( needinfo )
	uiMSG().message( "Successful export of:\n", sfs.fname_ );

    selcomp_ = -1;
    return false;
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    const IOObj* useoutioobj = &outioobj; IOObj* tmpioobj = 0;
    const bool inissidom = ZDomain::isSI( inioobj.pars() );
    if ( !inissidom )
    {
	tmpioobj = outioobj.clone();
	ZDomain::Def::get(inioobj.pars()).set( tmpioobj->pars() );
	useoutioobj = tmpioobj;
    }

    SEGY::TxtHeader::info2D() = is2d;
    Executor* exec = transffld_->getTrcProc( inioobj, *useoutioobj,
				    "Export seismic data", "Putting traces",
				    attrnm, linenm );
    if ( !exec )
	{ delete tmpioobj; return false; }
    PtrMan<Executor> execptrman = exec;

    mDynamicCastGet(SeisSingleTraceProc*,sstp,exec)
    if ( sstp )
    {
	SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *sstp->reader(0) );
	SeisIOObjInfo oinf( rdr.ioObj() );
	if ( oinf.isOK() && oinf.nrComponents() > 1 && selcomp_ < 0 )
	{
	    const LineKey lk( linenm, attrnm );
	    BufferStringSet cnms; oinf.getComponentNames( cnms, lk );
	    uiSelectFromList dlg( this,
		uiSelectFromList::Setup("Please select the component",cnms) );
	    dlg.setHelpID("103.0.15");
	    if ( !dlg.go() )
		return false;
	    selcomp_ = dlg.selection();
	}
	rdr.setComponent( selcomp_ );

	if ( !autogentxthead_ && !hdrtxt_.isEmpty() )
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

    if ( tmpioobj )
	IOM().commitChanges( *tmpioobj );
    delete tmpioobj;
    SEGY::TxtHeader::info2D() = false;
    return rv;
}
