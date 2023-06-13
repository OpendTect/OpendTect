/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchprogs.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uitextedit.h"
#include "uitextfile.h"

#include "ascstream.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "manobjectset.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "progressmeter.h"
#include "separstr.h"


class BatchProgPar
{
public:

			BatchProgPar(const char*);

    enum Type		{ FileRead, FileWrite, Words, QWord };

    Type		type_;
    bool		mandatory_;
    BufferString	desc_;

    static Type		getType( const char* s )
			{
			    if ( *s == 'W' ) return Words;
			    if ( *s == 'Q' ) return QWord;
			    if ( StringView(s)=="FileRead" )
				return FileRead;
			    else
				return FileWrite;
			}
};


BatchProgPar::BatchProgPar( const char* val )
{
    const FileMultiString fms( val );
    if ( fms.size() > 0 )
	mandatory_ = *fms[0] == 'M';
    if ( fms.size() > 1 )
	type_ = getType( fms[1] );
    if ( fms.size() > 2 )
	desc_ = fms[2];
}


class BatchProgInfo
{
public:

    enum UIType			{ NoUI, TxtUI, HasUI };

				BatchProgInfo( const char* nm )
				    : name_(nm)
				{}

    BufferString		name_;
    ManagedObjectSet<BatchProgPar> args_;
    BufferString		comments_;
    BufferString		exampleinput_;
    bool			issys_ = false;
    UIType			uitype_ = NoUI;
    bool			getstdout_ = false;

};


class BatchProgInfoList : public ManagedObjectSet<BatchProgInfo>
{
public:

			BatchProgInfoList();

    void		getEntries(const char*);
};


BatchProgInfoList::BatchProgInfoList()
{
    const char* fromenv = GetEnvVar( "OD_BATCH_PROGRAMS_FILE" );
    if ( fromenv && *fromenv )
	getEntries( fromenv );
    else
    {
	const char* searchkey = "BatchPrograms*";

	BufferString dirnm = mGetApplSetupDataDir();
	if ( !dirnm.isEmpty() )
	{
	    const DirList dlsite( dirnm, File::FilesInDir,searchkey);
	    for ( int idx=0; idx<dlsite.size(); idx++ )
		getEntries( dlsite.fullPath(idx) );
	}

	dirnm = mGetSWDirDataDir();
	const DirList dlrel( dirnm, File::FilesInDir, searchkey );
	for ( int idx=0; idx<dlrel.size(); idx++ )
	    getEntries( dlrel.fullPath(idx) );
    }
}


void BatchProgInfoList::getEntries( const char* fnm )
{
    if ( !File::exists(fnm) || File::isEmpty(fnm) )
	return;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    ascistream astrm( strm, true );
    while ( astrm.type() != ascistream::EndOfFile )
    {
	if ( atEndOfSection(astrm.next()) )
	    continue;

	const bool issys = astrm.hasValue("Sys");
	auto* bpi = new BatchProgInfo( astrm.keyWord() );
	if ( issys )
	    bpi->issys_ = true;

	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword("ExampleInput") )
		bpi->exampleinput_ = astrm.value();
	    else if ( astrm.hasKeyword("Arg") )
		bpi->args_.add( new BatchProgPar( astrm.value() ) );
	    else if ( astrm.hasKeyword("Comment") )
	    {
		char* ptr = const_cast<char*>(astrm.value());
		while ( *ptr && *ptr == '>' ) *ptr++ = ' ';
		if ( !bpi->comments_.isEmpty() )
		    bpi->comments_.addNewLine();
		bpi->comments_ += astrm.value();
	    }
	    else if ( astrm.hasKeyword("UI") )
	    {
		char firstchar = *astrm.value();
		bpi->uitype_ = firstchar=='T'	? BatchProgInfo::TxtUI
			     : (firstchar=='Y'	? BatchProgInfo::HasUI
						: BatchProgInfo::NoUI);
	    }
	    else if ( astrm.hasKeyword("Stdout") )
	    {
		bpi->getstdout_ = astrm.getYN();
	    }
	}

	if ( bpi )
	   add( bpi );
    }
}


uiBatchProgLaunch::uiBatchProgLaunch( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Run batch program"),
	       tr("Specify batch program and parameters"),
	       mODHelpKey(mBatchProgLaunchHelpID) ) )
    , pil_(*new BatchProgInfoList)
{
    if ( pil_.size() < 1 )
    {
	setCtrlStyle( CloseOnly );
        new uiLabel( this, tr("Not found any BatchPrograms.*"
                              " file in application"));
	return;
    }

    setCtrlStyle( RunAndClose );

    auto* lcc = new uiLabeledComboBox( this, tr("Batch program") );
    progfld_ = lcc->box();
    for ( int idx=0; idx<pil_.size(); idx++ )
	progfld_->addItem( toUiString(pil_[idx]->name_) );
    progfld_->setCurrentItem( 0 );
    mAttachCB( progfld_->selectionChanged, uiBatchProgLaunch::progSel );
    progfld_->setHSzPol( uiObject::WideVar );

    commfld_ = new uiTextEdit( this, "Comments" );
    commfld_->attach( leftAlignedBelow, lcc );
    commfld_->setPrefHeightInChar( 5 );
    commfld_->setPrefWidth( 400 );

    for ( int ibpi=0; ibpi<pil_.size(); ibpi++ )
    {
	const BatchProgInfo& bpi = *pil_[ibpi];
	auto* inplst = new ObjectSet<uiGenInput>;
	inps_.add( inplst );
	for ( int iarg=0; iarg<bpi.args_.size(); iarg++ )
	{
	    uiString txt;
	    const BatchProgPar& bpp = *bpi.args_[iarg];
	    if ( bpp.mandatory_ )
		txt = mToUiStringTodo(bpp.desc_);
	    else
		txt = toUiString("[%1]").arg(mToUiStringTodo(bpp.desc_));

	    uiGenInput* newinp;
	    if ( bpp.type_ == BatchProgPar::Words ||
		 bpp.type_ == BatchProgPar::QWord )
		newinp = new uiGenInput( this, txt );
	    else
	    {
		BufferString filt;
		if ( bpp.desc_ == "Parameter file" )
		    filt = "Parameter files (*.par)";
		bool forread = bpp.type_ == BatchProgPar::FileRead;
		newinp = new uiFileInput( this, txt,
			uiFileInput::Setup(uiFileDialog::Gen)
			.forread(forread).filter(filt.buf()) );
	    }

	    if ( iarg )
		newinp->attach( alignedBelow, (*inplst)[iarg-1] );
	    else
	    {
		newinp->attach( alignedBelow, lcc );
		newinp->attach( ensureBelow, commfld_ );
	    }

	    inplst->add( newinp );
	}
	if ( !bpi.exampleinput_.isEmpty() )
	{
	    if ( !exbut_ )
	    {
		exbut_ = new uiPushButton( this, tr("Show example input"),
				mCB(this,uiBatchProgLaunch,exButPush), false );
		if ( inplst->size() )
		    exbut_->attach( alignedBelow, (*inplst)[inplst->size()-1] );
	    }
	    else if ( inplst->size() )
		exbut_->attach( ensureBelow, (*inplst)[inplst->size()-1] );
	}
    }

    mAttachCB( postFinalize(), uiBatchProgLaunch::progSel );
}


uiBatchProgLaunch::~uiBatchProgLaunch()
{
    detachAllNotifiers();
    if ( browser_ )
	browser_->reject();

    delete &pil_;
}


void uiBatchProgLaunch::progSel( CallBacker* )
{
    const int selidx = progfld_->currentItem();
    const BatchProgInfo& bpi = *pil_[selidx];
    commfld_->setText( bpi.comments_ );

    for ( int ilst=0; ilst<inps_.size(); ilst++ )
    {
	ObjectSet<uiGenInput>& inplst = *inps_[ilst];
	for ( int iinp=0; iinp<inplst.size(); iinp++ )
	    inplst[iinp]->display( ilst == selidx );
    }

    if ( exbut_ )
	exbut_->display( !bpi.exampleinput_.isEmpty() );
}


void uiBatchProgLaunch::exButPush( CallBacker* )
{
    const int selidx = progfld_->currentItem();
    const BatchProgInfo& bpi = *pil_[selidx];
    if ( bpi.exampleinput_.isEmpty() )
	{ pErrMsg("In CB that shouldn't be called for entry"); return; }

    BufferString sourceex( mGetSetupFileName(bpi.exampleinput_) );
    if ( File::isEmpty(sourceex) )
	{ pErrMsg("Installation problem"); return; }

    BufferString targetex = GetProcFileName( bpi.exampleinput_ );
    if ( !File::exists(targetex) )
    {
	File::copy( sourceex, targetex );
	File::makeWritable( targetex, true, false );
    }

    if ( browser_ )
	browser_->setFileName( targetex );
    else
    {
	browser_ = new uiTextFileDlg( this, targetex );
	mAttachCB( browser_->editor()->fileNmChg, uiBatchProgLaunch::filenmUpd);
    }

    browser_->show();
}


bool uiBatchProgLaunch::acceptOK( CallBacker* )
{
    if ( !progfld_ )
	return true;

    const int selidx = progfld_->currentItem();
    const BatchProgInfo& bpi = *pil_[selidx];
    ObjectSet<uiGenInput>& inplst = *inps_[selidx];

    BufferString prognm = progfld_->text();
    if ( prognm.isEmpty() )
	return false;

    int firstinp = 0;
    if ( prognm.firstChar() == '[' )
    {
	prognm = inplst[0]->text();
	firstinp = 1;
    }

    OS::MachineCommand mc( prognm );
    for ( int iinp=firstinp; iinp<inplst.size(); iinp++ )
    {
	uiGroup* curinp = inplst[iinp];
	mDynamicCastGet(uiFileInput*,finp,curinp)
	mDynamicCastGet(uiGenInput*,inp,curinp)
	BufferString val;
	if ( finp )
	{
	    val = finp->fileName();
	}
	else if ( inp )
	{
	    val = inp->text();
	    if ( val.isEmpty() )
		continue;
	    if ( bpi.args_[iinp]->type_ == BatchProgPar::QWord )
		val.quote();
	}

	mc.addArg( val );
    }

    bool res = false;
    OS::CommandExecPars execpars;
    if ( bpi.getstdout_ )
    {
	BufferString stdoutstr, stderrstr;
	res = mc.execute( stdoutstr, &stderrstr );
	if ( res && !stdoutstr.isEmpty() )
	    displayText( stdoutstr.str(), mc.toString(&execpars) );
    }
    else
    {
	execpars = OS::CommandExecPars( OS::RunInBG );
	execpars.createstreams( bpi.uitype_ == BatchProgInfo::NoUI )
		.isconsoleuiprog( bpi.uitype_ == BatchProgInfo::TxtUI );
	res = mc.execute( execpars );
    }

    if ( !res )
	uiMSG().error(tr("Cannot execute command:\n%1")
			.arg(mc.toString(&execpars)));

    return false;
}


void uiBatchProgLaunch::filenmUpd( CallBacker* cb )
{
    mDynamicCastGet(uiTextFile*,uitf,cb)
    if ( !uitf ) return;

    const int selidx = progfld_->currentItem();

    ObjectSet<uiGenInput>& inplst = *inps_[selidx];
    for ( int iinp=0; iinp<inplst.size(); iinp++ )
    {
	uiGenInput* inp = inplst[iinp];
	mDynamicCastGet(uiFileInput*,finp,inp)
	BufferString val;
	if ( finp )
	    finp->setText( uitf->fileName() );
    }
}


class uiTextViewerWin : public uiMainWin
{ mODTextTranslationClass(uiTextViewer)
public:
			uiTextViewerWin(uiParent*,const char* txt,
					const uiString& sbtxt);
			~uiTextViewerWin();
private:

    void		initWinCB(CallBacker*);

    uiTextBrowser*	txtfld_;
    uiString		sbtxt_;

};


uiTextViewerWin::uiTextViewerWin( uiParent* p, const char* txt,
				  const uiString& sbtxt )
    : uiMainWin(p,tr("Command output viewer"),1,false)
    , sbtxt_(sbtxt)
{
    txtfld_ = new uiTextBrowser( this, "Command Output" );
    uiFont& fnt = FontList().add( "Non-prop",
			FontData(FontData::defaultPointSize(),"Courier") );
    txtfld_->setFont( fnt );

    //Ensure we have space for 80 chars
    const int nrchars = TextStreamProgressMeter::cNrCharsPerRow()+5;
    mAllocVarLenArr( char, str, nrchars+1 );
    OD::memSet( str, ' ', nrchars );
    str[nrchars] = '\0';

    int deswidth = fnt.width( mToUiStringTodo(str) );
    const int desktopwidth = uiMain::instance().desktopSize().hNrPics();
    if ( !mIsUdf(desktopwidth) && deswidth>desktopwidth )
	deswidth = desktopwidth;

    if ( deswidth > txtfld_->defaultWidth() )
	txtfld_->setPrefWidth( deswidth );

    txtfld_->setText( txt );
    mAttachCB( postFinalize(), uiTextViewerWin::initWinCB );
}


uiTextViewerWin::~uiTextViewerWin()
{
    detachAllNotifiers();
}


void uiTextViewerWin::initWinCB( CallBacker* )
{
    if ( statusBar() && !sbtxt_.isEmpty() )
	statusBar()->message( sbtxt_ );
}


void uiBatchProgLaunch::displayText( const char* txt, const char* commandtxt )
{
    const uiString sbtxt = tr("Output of: '%1'").arg( commandtxt );
    auto* txtwin = new uiTextViewerWin( this, txt, sbtxt );
    txtwin->setDeleteOnClose( true );
    txtwin->show();
}
