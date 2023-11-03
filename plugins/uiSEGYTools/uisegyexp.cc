/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyexp.h"

#include "uibatchjobdispatchersel.h"
#include "uicompoundparsel.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uifiledlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiscrollarea.h"
#include "uisegydef.h"
#include "uisegymanip.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "od_helpids.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "seisstor.h"
#include "segybatchio.h"
#include "segydirecttr.h"
#include "segyhdr.h"
#include "seispacketinfo.h"
#include "settings.h"
#include "zdomain.h"

#define mTxtHdrWidth	80
#define mLineHdrWidth	4
#define mEditorWidth	76

#define mNrTxtHdrLines	40
#define mNrEditorLines	38
#define mNrFooterLines	2

static const char* getLineHdr( int idx )
{
    static BufferString ret;
    ret = "C";
    if ( idx < 9 )
	ret.add( "0" );

    ret.add( idx + 1 );
    return ret.buf();
}


static void removeLineHdr( BufferString& linebuf )
{
    if ( (linebuf[0] == 'C' || linebuf[0] == 'c') &&
	 (linebuf[1] == ' ' || isdigit(linebuf[1])) && isdigit(linebuf[2]) )
    {
	BufferString tmp( linebuf.getCStr() + 3 );
	linebuf = tmp;
    }
}


class uiSEGYExpTxtHeaderDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYExpTxtHeaderDlg)
public:

uiSEGYExpTxtHeaderDlg( uiParent* p, BufferString& hdr, bool& ag )
    : uiDialog(p,Setup(tr("Edit SEG-Y Text Header"),mNoDlgTitle,
			mODHelpKey(mSEGYExpTxtHeaderDlgHelpID) ))
    , hdr_(hdr)
    , autogen_(ag)
    , resetPushed(this)
{
    auto* wtb = new uiToolButton( this, "save", tr("Write to file"),
			mCB(this,uiSEGYExpTxtHeaderDlg,writePush) );
    wtb->attach( rightBorder );
    auto* rtb = new uiToolButton( this, "open", tr("Read file"),
			mCB(this,uiSEGYExpTxtHeaderDlg,readPush) );
    rtb->attach( leftOf, wtb );
    auto* resetb = new uiPushButton( this, tr("Reset to Auto-generated"),
			mCB(this,uiSEGYExpTxtHeaderDlg,resetPush), true );
    resetb->attach( leftOf, rtb );


    auto* sa = new uiScrollArea( this );
    auto* grp = new uiGroup( nullptr, "Group" );

    auto* linehdrfld1 = new uiTextEdit( grp, "Line Hdr1", true );
    linehdrfld1->setStretch( 0, 0 );
    linehdrfld1->setPrefWidthInChar( mLineHdrWidth );
    linehdrfld1->setPrefHeightInChar( mNrEditorLines );
    linehdrfld1->hideFrame();
    linehdrfld1->hideScrollBar( true );
    linehdrfld1->ignoreWheelEvents( true );
    BufferString txt;
    for ( int idx=0; idx<mNrEditorLines; idx++ )
	txt.add( getLineHdr(idx) ).add( "\n" );

    linehdrfld1->setText( txt );

    auto* linehdrfld2 = new uiTextEdit( grp, "Line Hdr2", true );
    linehdrfld2->setStretch( 0, 0 );
    linehdrfld2->setPrefWidthInChar( mLineHdrWidth );
    linehdrfld2->setPrefHeightInChar( mNrFooterLines );
    linehdrfld2->hideFrame();
    linehdrfld2->hideScrollBar( true );
    linehdrfld2->ignoreWheelEvents( true );
    txt.setEmpty();
    for ( int idx=mNrEditorLines; idx<mNrTxtHdrLines; idx++ )
	txt.add( getLineHdr(idx) ).add( "\n" );

    linehdrfld2->setText( txt );
    linehdrfld2->attach( alignedBelow, linehdrfld1, 0 );

    edfld_ = new uiTextEdit( grp, "Hdr edit" );
    edfld_->setStretch( 0, 0 );
    edfld_->setPrefWidthInChar( mEditorWidth );
    edfld_->setPrefHeightInChar( mNrEditorLines );
    edfld_->setLineWrapColumn( mEditorWidth );
    edfld_->hideFrame();
    edfld_->hideScrollBar( true );
    edfld_->ignoreWheelEvents( true );
    edfld_->attach( rightOf, linehdrfld1 );

    footerfld_ = new uiTextEdit( grp, "Footer" );
    footerfld_->setStretch( 0, 0 );
    footerfld_->setPrefWidthInChar( mEditorWidth );
    footerfld_->setPrefHeightInChar( mNrFooterLines );
    footerfld_->setLineWrapColumn( mEditorWidth );
    footerfld_->hideFrame();
    footerfld_->hideScrollBar( true );
    footerfld_->ignoreWheelEvents( true );
    footerfld_->attach( alignedBelow, edfld_, 0 );

    grp->attachObj()->setMinimumHeight(
	    edfld_->prefVNrPics() + footerfld_->prefVNrPics() + 2 );
    const int minwidth = linehdrfld1->prefHNrPics() + edfld_->prefHNrPics();
    grp->attachObj()->setMinimumWidth( minwidth );
    sa->attach( ensureBelow, resetb );
    sa->limitWidth( true );
    sa->setObject( grp->attachObj() );
    sa->setObjectResizable( true );
    sa->setPrefHeight( 400 );

    afterPopup.notify( mCB(this,uiSEGYExpTxtHeaderDlg,poppedUpCB) );
}

void poppedUpCB( CallBacker* )
{
    setText( hdr_ );
}

void setText( const char* txt )
{
    BufferString txtbuf( txt );
    BufferString editorbuf, footerbuf;
    int inpidx = 0;
    for ( int idx=0; idx<mNrTxtHdrLines; idx++ )
    {
	BufferString linebuf;
	char c;
	while ( inpidx < txtbuf.size() )
	{
	    c = txtbuf[inpidx++];
	    if ( !c || c == '\n' )
		break;

	    if ( linebuf.size()==mTxtHdrWidth )
	    {
		inpidx--;
		break;
	    }

	    linebuf.add( c );
	}

	if ( !linebuf.isEmpty() )
	{
	    removeLineHdr( linebuf );
	    removeTrailingBlanks( linebuf.getCStr() );
	    if ( linebuf[0] == ' ' )
	    {
		BufferString tmp( linebuf.getCStr() + 1 );
		linebuf = tmp;
	    }

	    if ( linebuf.size() > mEditorWidth )
		linebuf[mEditorWidth] = '\0';
	}

	linebuf.add( "\n" );
	if ( idx < mNrEditorLines )
	    editorbuf.add( linebuf );
	else
	    footerbuf.add( linebuf );
    }

    edfld_->setText( editorbuf );
    footerfld_->setText( footerbuf );
}

void readPush( CallBacker* )
{
    FilePath fp( GetDataDir() );
    const BufferString filefilter( "SEG-Y or Text files "
				   "(*.sgy *.SGY *.segy *.txt *.dat)" );
    uiFileDialog dlg( this, true, fp.fullPath(), filefilter,
			tr("Read SEG-Y Textual Header from file") );
    if ( !dlg.go() ) return;

    od_istream strm( dlg.fileName() );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open file") );
	return;
    }

    const int maxnrchars = SegyTxtHeaderLength + mNrTxtHdrLines;
    BufferString txt( maxnrchars + 1, true );
    strm.getC( txt.getCStr(), txt.bufSize(), maxnrchars );
    if ( txt.size() >= SegyTxtHeaderLength && !txt.find('\n') && txt[0] != 'C' )
    {
	SEGY::TxtHeader ebcidichdr;
	OD::memCopy( ebcidichdr.txt_, txt.getCStr(), SegyTxtHeaderLength );
	if ( !ebcidichdr.isAscii() )
	{
	    ebcidichdr.setAscii();
	    ebcidichdr.getText( txt );
	}
    }

    setText( txt );
    autogen_ = false;
}

void writePush( CallBacker* )
{
    FilePath fp( GetDataDir(), "SEG-Y_Text_Header.dat" );
    uiFileDialog dlg( this, false, fp.fullPath(), File::asciiFilesFilter(),
	    tr("Write SEG-Y Textual Header to a file") );
    if ( !dlg.go() ) return;

    fp.set( dlg.fileName() );
    if ( !File::isWritable(fp.pathOnly()) )
	{ uiMSG().error(tr("Cannot write to this folder")); return; }
    const BufferString fnm( fp.fullPath() );
    if ( File::exists(fnm) && !File::isWritable(fnm) )
	{ uiMSG().error(tr("Cannot write to this file")); return; }

    od_ostream strm( fnm );
    BufferString txt;
    getText( txt );
    strm.add( txt );
    if ( !strm.isOK() )
	uiMSG().error( tr("Failed to save text to %1").arg(fnm) );
}

void resetPush( CallBacker* )
{
    autogen_ = true;
    resetPushed.trigger();
    setText( hdr_ );
}

void getText( BufferString& txt )
{
    txt.setEmpty();
    BufferString editorbuf = edfld_->text();
    BufferString footerbuf = footerfld_->text();
    int inpidx = 0;
    for ( int idx=0; idx<mNrTxtHdrLines; idx++ )
    {
	BufferString linebuf;
	BufferString& inpbuf = idx < mNrEditorLines ? editorbuf : footerbuf;
	if ( idx == mNrEditorLines )
	    inpidx = 0;

	linebuf.add( getLineHdr(idx) ).add( ' ' );
	char c;
	while ( inpidx < inpbuf.size() )
	{
	    c = inpbuf[inpidx++];
	    if ( !c || c == '\n' )
		break;

	    if ( linebuf.size()==mTxtHdrWidth )
	    {
		inpidx--;
		break;
	    }

	    linebuf.add( c );
	}

	while ( linebuf.size() < mTxtHdrWidth )
	    linebuf.add( ' ' );

	linebuf.add( "\n" );
	txt.add( linebuf );
    }
}

bool acceptOK( CallBacker* )
{
    getText( hdr_ );
    if ( edfld_->isModified() )
	autogen_ = false;

    return true;
}

    BufferString&	hdr_;
    bool		autogen_;
    uiTextEdit*		edfld_;
    uiTextEdit*		footerfld_;

    Notifier<uiSEGYExpTxtHeaderDlg>	resetPushed;
};

class uiSEGYExpTxtHeader : public uiCompoundParSel
{ mODTextTranslationClass(uiSEGYExpTxtHeader)
public:

uiSEGYExpTxtHeader( uiSEGYExp* se )
    : uiCompoundParSel(se,tr("Text header"),OD::Edit)
    , se_(se)
{
    butPush.notify( mCB(this,uiSEGYExpTxtHeader,butPushed) );
}

void butPushed( CallBacker* )
{
    BufferString hdrtxt;
    se_->getTextHeader( hdrtxt );
    uiSEGYExpTxtHeaderDlg dlg( this, hdrtxt,  se_->autogentxthead_);
    dlg.resetPushed.notify( mCB(this,uiSEGYExpTxtHeader,resetPushed) );
    if ( !dlg.go() )
	return;

    se_->hdrtxt_ = hdrtxt;
    se_->autogentxthead_ = dlg.autogen_;
}

void resetPushed( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYExpTxtHeaderDlg*,dlg,cb)
    if ( !dlg )
	return;

    se_->generateAutoTextHeader( dlg->hdr_ );
}

BufferString getSummary() const
{
    if ( se_->autogentxthead_ )
	return BufferString( "Auto-generated" );
    else
	return BufferString( "User-defined" );
}

    uiSEGYExp*	se_;

};



uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
    : uiDialog(p,uiDialog::Setup(tr("Export Seismic Data to SEG-Y"),
				 mNoDlgTitle,mNoHelpKey).modal(false))
    , geom_(gt)
    , autogentxthead_(true)
    , morebox_(nullptr)
    , manipbox_(nullptr)
    , batchfld_(nullptr)
    , othercrsfld_(nullptr)
    , coordsysselfld_(nullptr)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    switch (gt)
    {
    case Seis::Vol: setHelpKey( mODHelpKey(mSEGYExpHelpID) ); break;
    case Seis::VolPS: setHelpKey( mODHelpKey(mSEGYExpVolPSHelpID) ); break;
    case Seis::Line: setHelpKey( mODHelpKey(mSEGYExpLineHelpID) ); break;
    case Seis::LinePS: setHelpKey( mODHelpKey(mSEGYExpLinePSHelpID) ); break;
    }

    IOObjContext ctxt( uiSeisSel::ioContext( geom_, true ) );
    uiSeisSel::Setup sssu( geom_ ); sssu.steerpol(uiSeisSel::Setup::InclSteer);
    sssu.selectcomp(true);
    sssu.trsnotallwed_.add( mSEGYDirectTranslNm );
    seissel_ = new uiSeisSel( this, ctxt, sssu );
    mAttachCB( seissel_->selectionDone, uiSEGYExp::inpSel );

    uiSeisTransfer::Setup tsu( geom_ );
    tsu.withnullfill(true).fornewentry(false).onlyrange(false);
    transffld_ = new uiSeisTransfer( this, tsu );
    if ( transffld_->selfld )
	mAttachCB( transffld_->selfld->selChange, uiSEGYExp::updateTextHdrCB );

    transffld_->attach( alignedBelow, seissel_ );

    uiObject* attachobj = transffld_->attachObj();

    if ( SI().hasProjection() )
    {
	othercrsfld_ = new uiGenInput( this, tr("Export to other CRS"),
				       BoolInpSpec(false) );
	othercrsfld_->attach( alignedBelow, transffld_ );
	mAttachCB( othercrsfld_->valueChanged, uiSEGYExp::crsCB );
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	mAttachCB( coordsysselfld_->changed, uiSEGYExp::updateTextHdrCB );
	coordsysselfld_->attach( alignedBelow, othercrsfld_ );
	attachobj = coordsysselfld_->attachObj();
    }

    fpfld_ = new uiSEGYFilePars( this, false, nullptr, false );
    fpfld_->attach( alignedBelow, attachobj );

    txtheadfld_ = new uiSEGYExpTxtHeader( this );
    txtheadfld_->attach( alignedBelow, fpfld_ );

    const bool doebcdic =
	Settings::common().isTrue( SEGY::TxtHeader::sKeySettingEBCDIC() );
    txtheadfmtsel_ = new uiGenInput( this, tr("Text header format"),
	    BoolInpSpec(doebcdic,toUiString("EBCDIC"),uiStrings::sASCII()) );
    txtheadfmtsel_->attach( alignedBelow, txtheadfld_ );

    const bool is2d = Seis::is2D( geom_ );
    const bool issingle2dline = geom_ == Seis::Line;
    uiSEGYFileSpec::Setup su( !issingle2dline );
    su.forread( false ).canbe3d( !is2d );
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfmtsel_ );

    if ( issingle2dline )
    {
	morebox_ = new uiCheckBox( this, uiStrings::phrExport(
				tr("more lines from the same dataset")),
				mCB(this,uiSEGYExp,showSubselCB) );
	morebox_->attach( alignedBelow, fsfld_ );
    }
    else
    {
	manipbox_ = new uiCheckBox( this,
			tr("Manipulate output file after creation") );
	manipbox_->attach( alignedBelow, fsfld_ );

	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	mAttachCB( batchfld_->checked, uiSEGYExp::batchChg );
	batchfld_->attach( alignedBelow,
			   manipbox_ ?	manipbox_ : fsfld_->attachObj() );
    }

    mAttachCB( postFinalize(), uiSEGYExp::inpSel );
}


uiSEGYExp::~uiSEGYExp()
{
    detachAllNotifiers();
}


void uiSEGYExp::inpSel( CallBacker* )
{
    crsCB( nullptr );
    const IOObj* ioobj = seissel_->ioobj( true );
    if ( !ioobj )
	return;

    transffld_->updateFrom( *ioobj );

    const FilePath fp = ioobj->fullUserExpr();
    FilePath fnm( GetSurveyExportDir(), fp.baseName() );
    fnm.setExtension( "sgy" );
    fsfld_->setFileName( fnm.fullPath() );
}


void uiSEGYExp::crsCB( CallBacker* )
{
    if ( coordsysselfld_ )
	coordsysselfld_->display( othercrsfld_->getBoolValue() );
}


void uiSEGYExp::showSubselCB( CallBacker* )
{
    const bool multilinesel = morebox_->isChecked();
    transffld_->showSubselFld( !multilinesel );
    if ( multilinesel && !autogentxthead_ )
    {
	const bool resp = uiMSG().askGoOn(
		tr("User-defined text headers are not supported for multi-line "
		    "export. Do you want to switch to auto-generated headers?"),
		tr("Yes, use auto-generated header"),
		tr("No, go back to single-line export") );
	if ( !resp )
	{
	    morebox_->setChecked( false );
	    return;
	}
    }


    if ( multilinesel )
	updateTextHdrCB( nullptr );

    txtheadfld_->setSensitive( !multilinesel );
}


void uiSEGYExp::updateTextHdrCB( CallBacker* )
{
    autogentxthead_ = true;
    txtheadfld_->updateSummary();
}


void uiSEGYExp::batchChg( CallBacker* )
{
    if ( !manipbox_ || !batchfld_ )
	return;

    manipbox_->setSensitive( !batchfld_->wantBatch() );
}



class uiSEGYExpMore : public uiDialog
{ mODTextTranslationClass(uiSEGYExpMore)
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi )
    : uiDialog(p,uiDialog::Setup(tr("2D SEG-Y multi-export"),
				 tr("Specify file details"),
				 mODHelpKey(mSEGYExpMoreHelpID) ))
    , segyexp_(p)
    , inioobj_(ii)
    , outioobj_(oi)
{
    const BufferString fnm( outioobj_.fullUserExpr(false) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Exp " );
    setupnm += uiSEGYFileSpec::sKeyLineNmToken();

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Lines to export") );
    lnmsfld_ = new uiListBox( this, su );
    SeisIOObjInfo sii( inioobj_ );
    BufferStringSet lnms;
    sii.getLineNames( lnms );
    for ( int idx=0; idx<lnms.size(); idx++ )
	lnmsfld_->addItem( lnms.get(idx) );
    lnmsfld_->chooseAll();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "_"; newfnm += inioobj_.name();
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    const uiString txt( tr("Output (Line name replaces '%1'")
			.arg(uiSEGYFileSpec::sKeyLineNmToken()) );

    uiFileInput::Setup fisu( fp.fullPath() );
    fisu.objtype( uiStrings::sSEGY() ).forread( false );
    fnmfld_ = new uiFileInput( this, txt, fisu );
    fnmfld_->attach( alignedBelow, lnmsfld_ );
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
	uiMSG().error( tr("Directory provided not usable") );
	return false;
    }

    fnm = fp.fullPath();
    if ( !fnm.contains(uiSEGYFileSpec::sKeyLineNmToken()) )
    {
	uiString msg = tr("The file name has to contain at least one '%1'\n"
			  "That will then be replaced by the line name")
		     .arg(uiSEGYFileSpec::sKeyLineNmToken());
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
    iostrm->fileSpec().setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, bool islast, bool& nofails )
{
    const IOObj& in = inioobj_; const IOObj& out = *newioobj;
    bool res = segyexp_->doWork( in, out );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askContinue(tr("Continue with next?")) )
	    return false;
    }
    return true;
}

bool doExp( const FilePath& fp )
{
    BufferStringSet lnms;
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( lnmsfld_->isChosen(idx) )
	    lnms.add( lnmsfld_->textOfItem(idx) );
    }
    if ( lnms.size() < 1 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("lines to export")) );
	return false;
    }

    bool nofails = true;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString& lnm = *lnms[idx];
	BufferString filenm( fp.fullPath() );
	filenm.replace( uiSEGYFileSpec::sKeyLineNmToken(), lnm );
	IOObj* newioobj = getSubstIOObj( filenm );

	uiSeis2DSubSel* seissel2d = segyexp_->transffld_->selFld2D();
	if ( seissel2d && seissel2d->isSingLine() )
	    seissel2d->setSelectedLine( lnm );

	if ( !doWork(newioobj,idx>lnms.size()-2,nofails) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiListBox*		lnmsfld_;
    uiSEGYExp*		segyexp_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;

};


void uiSEGYExp::getTextHeader( BufferString& hdrtxt )
{
    if ( autogentxthead_ || hdrtxt_.isEmpty() )
	generateAutoTextHeader( hdrtxt_ );

    hdrtxt = hdrtxt_;
}


void uiSEGYExp::generateAutoTextHeader( BufferString& hdrtxt ) const
{
    const IOObj* inioobj = seissel_->ioobj(true);
    if ( !inioobj )
    {
	uiMSG().error( uiStrings::phrSelect(tr("the data to export")) );
	return;
    }

    StringPair datanm( inioobj->name() );
    SeisIOObjInfo inpinfo( inioobj );
    BufferStringSet compnames;
    inpinfo.getComponentNames( compnames );
    const int selcomp = seissel_->compNr();
    if ( compnames.validIdx(selcomp) )
	datanm.second() = compnames.get( selcomp );

    ConstRefMan<Coords::CoordSystem> crs = nullptr;
    if ( othercrsfld_ && othercrsfld_->getBoolValue() )
	crs = coordsysselfld_->getCoordSystem();

    SEGY::TrcHeaderDef thdef;
    SEGY::TrcHeader::fillRev1Def( thdef );

    PtrMan<Seis::SelData> seldata = transffld_->getSelData();
    if ( seldata )
    {
	thdef.pinfo = new SeisPacketInfo;
	thdef.pinfo->inlrg.setInterval( seldata->inlRange() );
	thdef.pinfo->crlrg.setInterval( seldata->crlRange() );
	thdef.pinfo->zrg.setInterval( seldata->zRange() );
    }

    SEGY::TxtHeader txthdr( 1 );
    txthdr.setInfo( datanm.getCompString(), crs, thdef );
    txthdr.getText( hdrtxt );
}


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK( CallBacker* )
{
    const IOObj* inioobj = seissel_->ioobj(true);
    if ( !inioobj )
	mErrRet( uiStrings::phrSelect(tr("the data to export")) )

    const SEGY::FileSpec sfs( fsfld_->getSpec() );
    if ( sfs.isEmpty() )
	mErrRet( uiStrings::phrSelect(uiStrings::sOutputFile().toLower()) )

    if ( !autogentxthead_ && !hdrtxt_.isEmpty() )
	transffld_->setOutputHeader( hdrtxt_ );

    if ( othercrsfld_ && othercrsfld_->getBoolValue() )
    {
	ConstRefMan<Coords::CoordSystem> crs =
					 coordsysselfld_->getCoordSystem();
	if ( crs )
	{
	    transffld_->setCoordSystem( *crs.ptr(), false );
	    SEGY::FilePars filepars = fpfld_->getPars();
	    filepars.setCoordSys( crs );
	    fpfld_->setPars( filepars );
	}
    }

    const bool doebcdic = txtheadfmtsel_->getBoolValue();
    Settings::common().setYN( SEGY::TxtHeader::sKeySettingEBCDIC(), doebcdic );
    Settings::common().write();

    if ( batchfld_ && batchfld_->wantBatch() )
    {
	const BufferString jobname( "Export_SEG-Y_",
				    fsfld_->getJobNameFromFileName() );
	batchfld_->setJobName( jobname );
	IOPar& jobpars = batchfld_->jobSpec().pars_;
	jobpars.setEmpty();
	Seis::putInPar( geom_, jobpars );
	jobpars.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyExport() );

	IOPar inpars;
	seissel_->fillPar( inpars );
	jobpars.mergeComp( inpars, sKey::Input() );

	IOPar outpars;
	transffld_->fillPar( outpars );
	fpfld_->fillPar( outpars );
	fsfld_->fillPar( outpars );

	jobpars.mergeComp( outpars, sKey::Output() );
	batchfld_->start();
	return false;
    }

    PtrMan<IOObj> outioobj = sfs.getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );

    const bool multilinesel = morebox_ && morebox_->isChecked();
    bool needmsgallok = false;
    if ( multilinesel )
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj );
	dlg.go();
    }
    else
    {
	const bool result = doWork( *inioobj, *outioobj );
	if ( !result || !manipbox_ || !manipbox_->isChecked() )
	    needmsgallok = result;
	else
	{
	    uiSEGYFileManip dlg( this, outioobj->fullUserExpr(false) );
	    dlg.go();
	}
    }

    if ( needmsgallok )
	uiMSG().message( tr("Successful export of:\n%1").arg(sfs.dispName()) );

    return false;
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj )
{
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    PtrMan<IOObj> useoutioobj = outioobj.clone();
    if ( SeisStoreAccess::zDomain(&inioobj).fillPar(useoutioobj->pars()) )
	IOM().commitChanges( *useoutioobj );

    BufferString execnm( "Output seismic data" );
    if ( transffld_->selFld2D() && transffld_->selFld2D()->isSingLine() )
    {
	execnm.add( " (" ).add( transffld_->selFld2D()->selectedLine() )
	      .add( ")" );
    }

    PtrMan<Executor> exec = transffld_->getTrcProc( inioobj, *useoutioobj,
			    execnm, tr("Writing traces"),
			    seissel_->compNr() );
    if ( !exec )
	return false;

    uiTaskRunner dlg( this );
    const bool res = TaskRunner::execute( &dlg, *exec );
    return res;
}
