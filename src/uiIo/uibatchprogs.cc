/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/

#include "uibatchprogs.h"
#include "uifileinput.h"
#include "uitextfile.h"
#include "uicombobox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "manobjectset.h"
#include "od_istream.h"
#include "iopar.h"
#include "oddirs.h"
#include "oscommand.h"
#include "separstr.h"
#include "envvars.h"
#include "dirlist.h"

#include "od_helpids.h"


class BatchProgPar
{
public:

			BatchProgPar(const char*);

    enum Type		{ FileRead, FileWrite, Words, QWord };

    Type		type;
    bool		mandatory;
    BufferString	desc;

    static Type		getType( const char* s )
			{
			    if ( *s == 'W' ) return Words;
			    if ( *s == 'Q' ) return QWord;
			    if ( FixedString(s)=="FileRead" )
				return FileRead;
			    else
				return FileWrite;
			}
};


BatchProgPar::BatchProgPar( const char* val )
{
    FileMultiString fms( val );
    mandatory = *fms[0] == 'M';
    type = getType( fms[1] );
    desc = fms[2];
}


class BatchProgInfo
{
public:

    enum UIType			{ NoUI, TxtUI, HasUI };

				BatchProgInfo( const char* nm )
				: name(nm), issys(false), uitype_(NoUI) {}
				~BatchProgInfo()	 { deepErase(args); }

    BufferString		name;
    ObjectSet<BatchProgPar>	args;
    BufferString		comments;
    BufferString		exampleinput;
    bool			issys;
    UIType			uitype_;

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
	DirList dlrel( dirnm, File::FilesInDir, searchkey );
	for ( int idx=0; idx<dlrel.size(); idx++ )
	    getEntries( dlrel.fullPath(idx) );
    }
}


void BatchProgInfoList::getEntries( const char* fnm )
{
    if ( File::isEmpty(fnm) )
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
	BatchProgInfo* bpi = new BatchProgInfo(astrm.keyWord());
	if ( issys ) bpi->issys = true;

	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.hasKeyword("ExampleInput") )
		bpi->exampleinput = astrm.value();
	    else if ( astrm.hasKeyword("Arg") )
		bpi->args += new BatchProgPar( astrm.value() );
	    else if ( astrm.hasKeyword("Comment") )
	    {
		char* ptr = const_cast<char*>(astrm.value());
		while ( *ptr && *ptr == '>' ) *ptr++ = ' ';
		if ( !bpi->comments.isEmpty() )
		    bpi->comments += "\n";
		bpi->comments += astrm.value();
	    }
	    else if ( astrm.hasKeyword("UI") )
	    {
		char firstchar = *astrm.value();
		bpi->uitype_ = firstchar=='T'	? BatchProgInfo::TxtUI
			     : (firstchar=='Y'	? BatchProgInfo::HasUI
						: BatchProgInfo::NoUI);
	    }
	}

	if ( bpi ) *this += bpi;
    }
}


uiBatchProgLaunch::uiBatchProgLaunch( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Run batch program"),
	       tr("Specify batch program and parameters"),
	       mODHelpKey(mBatchProgLaunchHelpID) ) )
    , pil_(*new BatchProgInfoList)
    , progfld_(0)
    , browser_(0)
    , exbut_(0)
{
    if ( pil_.size() < 1 )
    {
	setCtrlStyle( CloseOnly );
        new uiLabel( this, tr("Not found any BatchPrograms.*"
                              " file in application"));
	return;
    }
    setCtrlStyle( RunAndClose );

    uiLabeledComboBox* lcc = new uiLabeledComboBox( this, tr("Batch program") );
    progfld_ = lcc->box();
    for ( int idx=0; idx<pil_.size(); idx++ )
	progfld_->addItem( toUiString(pil_[idx]->name) );
    progfld_->setCurrentItem( 0 );
    progfld_->selectionChanged.notify(
			mCB(this,uiBatchProgLaunch,progSel) );
    progfld_->setHSzPol( uiObject::WideVar );

    commfld_ = new uiTextEdit( this, "Comments" );
    commfld_->attach( leftAlignedBelow, lcc );
    commfld_->setPrefHeightInChar( 5 );
    commfld_->setPrefWidth( 400 );

    for ( int ibpi=0; ibpi<pil_.size(); ibpi++ )
    {
	const BatchProgInfo& bpi = *pil_[ibpi];
	ObjectSet<uiGenInput>* inplst = new ObjectSet<uiGenInput>;
	inps_ += inplst;
	for ( int iarg=0; iarg<bpi.args.size(); iarg++ )
	{
	    uiString txt;
	    const BatchProgPar& bpp = *bpi.args[iarg];
	    if ( !bpp.mandatory )
		txt = toUiString("[%1]").arg(mToUiStringTodo(bpp.desc));
	    else
		txt = mToUiStringTodo(bpp.desc);

	    uiGenInput* newinp;
	    if ( bpp.type == BatchProgPar::Words ||
		 bpp.type == BatchProgPar::QWord )
		newinp = new uiGenInput( this, txt );
	    else
	    {
		BufferString filt;
		if ( bpp.desc == "Parameter file" )
		    filt = "Parameter files (*.par)";
		bool forread = bpp.type == BatchProgPar::FileRead;
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
	    (*inplst) += newinp;
	}
	if ( !bpi.exampleinput.isEmpty() )
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


    postFinalize().notify( mCB(this,uiBatchProgLaunch,progSel) );
}


uiBatchProgLaunch::~uiBatchProgLaunch()
{
    if ( browser_ ) browser_->reject(0);
    delete &pil_;
}


void uiBatchProgLaunch::progSel( CallBacker* )
{
    const int selidx = progfld_->currentItem();
    const BatchProgInfo& bpi = *pil_[selidx];
    commfld_->setText( bpi.comments );

    for ( int ilst=0; ilst<inps_.size(); ilst++ )
    {
	ObjectSet<uiGenInput>& inplst = *inps_[ilst];
	for ( int iinp=0; iinp<inplst.size(); iinp++ )
	    inplst[iinp]->display( ilst == selidx );
    }

    if ( exbut_ )
	exbut_->display( !bpi.exampleinput.isEmpty() );
}


void uiBatchProgLaunch::exButPush( CallBacker* )
{
    const int selidx = progfld_->currentItem();
    const BatchProgInfo& bpi = *pil_[selidx];
    if ( bpi.exampleinput.isEmpty() )
	{ pErrMsg("In CB that shouldn't be called for entry"); return; }
    BufferString sourceex( mGetSetupFileName(bpi.exampleinput) );
    if ( File::isEmpty(sourceex) )
	{ pErrMsg("Installation problem"); return; }

    BufferString targetex = GetProcFileName( bpi.exampleinput );
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
	browser_->editor()->fileNmChg.notify(
				mCB(this,uiBatchProgLaunch,filenmUpd) );
    }
    browser_->show();
}


bool uiBatchProgLaunch::acceptOK( CallBacker* )
{
    if ( !progfld_ ) return true;

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
	    if ( bpi.args[iinp]->type == BatchProgPar::QWord )
		val.quote();
	}

	mc.addArg( val );
    }

    OS::CommandExecPars execpars( OS::RunInBG );
    execpars.needmonitor( bpi.uitype_ == BatchProgInfo::NoUI )
	    .isconsoleuiprog( bpi.uitype_ == BatchProgInfo::TxtUI )
	    .createstreams( bpi.uitype_ == BatchProgInfo::NoUI );

    if ( !mc.execute(execpars) )
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
