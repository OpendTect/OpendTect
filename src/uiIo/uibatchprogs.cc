/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uibatchprogs.cc,v 1.50 2011/12/14 13:16:41 cvsbert Exp $";

#include "uibatchprogs.h"
#include "uifileinput.h"
#include "uitextfile.h"
#include "uicombobox.h"
#include "uitextedit.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "ascstream.h"
#include "separstr.h"
#include "strmprov.h"
#include "file.h"
#include "filepath.h"
#include "manobjectset.h"
#include "iopar.h"
#include "oddirs.h"
#include "envvars.h"
#include "dirlist.h"
#include "errh.h"
#include <iostream>



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
			    if ( !strcmp("FileRead",s) )
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

				BatchProgInfo( const char* nm )
				: name(nm), issys(false), inxterm(true) {}
				~BatchProgInfo()	 { deepErase(args); }

    BufferString		name;
    ObjectSet<BatchProgPar>	args;
    BufferString		comments;
    BufferString		exampleinput;
    bool			issys;
    bool			inxterm;

};


class BatchProgInfoList : public ManagedObjectSet<BatchProgInfo>
{
public:

			BatchProgInfoList();

    void		getEntries(const char*);
};


BatchProgInfoList::BatchProgInfoList()
    : ManagedObjectSet<BatchProgInfo>(false)
{
    const char* fromenv = GetEnvVar( "OD_BATCH_PROGRAMS_FILE" );
    if ( fromenv && *fromenv )
	getEntries( fromenv );
    else
    {
	BufferString dirnm = mGetApplSetupDataDir();
	if ( !dirnm.isEmpty() )
	{
	    DirList dlsite( FilePath(dirnm,"data").fullPath(),
		    	    DirList::FilesOnly,"BatchPrograms*");
	    for ( int idx=0; idx<dlsite.size(); idx++ )
		getEntries( dlsite.fullPath(idx) );
	}

	dirnm = mGetSWDirDataDir();
	DirList dlrel( dirnm, DirList::FilesOnly, "BatchPrograms*" );
	for ( int idx=0; idx<dlrel.size(); idx++ )
	    getEntries( dlrel.fullPath(idx) );
    }
}


void BatchProgInfoList::getEntries( const char* fnm )
{
    if ( File::isEmpty(fnm) ) return;
    StreamData sd = StreamProvider(fnm).makeIStream();
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm, true );

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
	    else if ( astrm.hasKeyword("Xterm") )
		bpi->inxterm = astrm.getYN();
	}

	if ( bpi ) *this += bpi;
    }

    sd.close();
}


uiBatchProgLaunch::uiBatchProgLaunch( uiParent* p )
        : uiDialog(p,uiDialog::Setup("Run batch program",
		   "Specify batch program and parameters","0.1.5"))
	, pil(*new BatchProgInfoList)
	, progfld(0)
	, browser(0)
	, exbut(0)
{
    if ( pil.size() < 1 )
    {
	setCtrlStyle( LeaveOnly );
	new uiLabel( this, "Not found any BatchPrograms.* file in application");
	return;
    }
    setCtrlStyle( DoAndStay );

    progfld = new uiLabeledComboBox( this, "Batch program" );
    for ( int idx=0; idx<pil.size(); idx++ )
	progfld->box()->addItem( pil[idx]->name );
    progfld->box()->setCurrentItem( 0 );
    progfld->box()->selectionChanged.notify(
	    		mCB(this,uiBatchProgLaunch,progSel) );

    commfld = new uiTextEdit( this, "Comments" );
    commfld->attach( centeredBelow, progfld );
    commfld->setPrefHeightInChar( 5 );
    commfld->setPrefWidth( 400 );

    for ( int ibpi=0; ibpi<pil.size(); ibpi++ )
    {
	const BatchProgInfo& bpi = *pil[ibpi];
	ObjectSet<uiGenInput>* inplst = new ObjectSet<uiGenInput>;
	inps += inplst;
	for ( int iarg=0; iarg<bpi.args.size(); iarg++ )
	{
	    BufferString txt;
	    const BatchProgPar& bpp = *bpi.args[iarg];
	    if ( !bpp.mandatory ) txt += "[";
	    txt += bpp.desc;
	    if ( !bpp.mandatory ) txt += "]";

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
		newinp->attach( alignedBelow, progfld );
		newinp->attach( ensureBelow, commfld );
	    }
	    (*inplst) += newinp;
	}
	if ( !bpi.exampleinput.isEmpty() )
	{
	    if ( !exbut )
	    {
		exbut = new uiPushButton( this, "&Show example input",
				mCB(this,uiBatchProgLaunch,exButPush), false );
		if ( inplst->size() )
		    exbut->attach( alignedBelow, (*inplst)[inplst->size()-1] );
	    }
	    else if ( inplst->size() )
		exbut->attach( ensureBelow, (*inplst)[inplst->size()-1] );
	}
    }


    postFinalise().notify( mCB(this,uiBatchProgLaunch,progSel) );
}


uiBatchProgLaunch::~uiBatchProgLaunch()
{
    if ( browser ) browser->reject(0);
    delete &pil;
}


void uiBatchProgLaunch::progSel( CallBacker* )
{
    const int selidx = progfld->box()->currentItem();
    const BatchProgInfo& bpi = *pil[selidx];
    commfld->setText( bpi.comments );

    for ( int ilst=0; ilst<inps.size(); ilst++ )
    {
	ObjectSet<uiGenInput>& inplst = *inps[ilst];
	for ( int iinp=0; iinp<inplst.size(); iinp++ )
	    inplst[iinp]->display( ilst == selidx );
    }

    if ( exbut )
	exbut->display( !bpi.exampleinput.isEmpty() );
}


void uiBatchProgLaunch::exButPush( CallBacker* )
{
    const int selidx = progfld->box()->currentItem();
    const BatchProgInfo& bpi = *pil[selidx];
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

    if ( browser )
	browser->setFileName( targetex );
    else
    {
	browser = new uiTextFileDlg( this, false, false, targetex );
	browser->editor()->fileNmChg.notify(
				mCB(this,uiBatchProgLaunch,filenmUpd) );
    }
    browser->show();
}


bool uiBatchProgLaunch::acceptOK( CallBacker* )
{
    if ( !progfld ) return true;

    const int selidx = progfld->box()->currentItem();
    const BatchProgInfo& bpi = *pil[selidx];
    
#ifndef __msvc__

    BufferString comm( "@" );
    comm += mGetExecScript();

    if ( bpi.inxterm )
	comm += " --inxterm+askclose ";
    if ( bpi.issys )
	comm += "--sys ";

    const char* progtxt = progfld->box()->text();
    if ( progtxt && *progtxt && *progtxt != '[' )
    {
	comm += "\"";
	FilePath progfp( progfld->box()->text() );
	comm += progfp.fullPath( FilePath::Unix );
	comm += "\"";
    }

    ObjectSet<uiGenInput>& inplst = *inps[selidx];
    for ( int iinp=0; iinp<inplst.size(); iinp++ )
    {
	uiGenInput* inp = inplst[iinp];
	mDynamicCastGet(uiFileInput*,finp,inp)
	BufferString val;
	if ( finp )
	{
	    val = "\"";

	    FilePath argfp( finp->fileName() );
	    BufferString arg = argfp.fullPath( FilePath::Unix );
	    replaceCharacter(arg.buf(),' ','%');
	    val += arg;

	    val += "\"";
	}
	else if ( bpi.args[iinp]->type != BatchProgPar::QWord )
	    val = inp->text();
	else
	    { val = "'"; val += inp->text(); val += "'"; }

	comm += " ";
	comm += val;
    }

#else

    FilePath fp( GetBinPlfDir() );

    const char* progtxt = progfld->box()->text();
    if ( progtxt && *progtxt && *progtxt != '[' )
	fp.add( progfld->box()->text() );

    const bool isanotherprog = progtxt && *progtxt == '[';
    ObjectSet<uiGenInput>& inplst = *inps[selidx];
    int startfldidx = 0;
    if ( isanotherprog && !inplst.isEmpty() )
    {
	fp.add( inplst[0]->text() );
	startfldidx = 1;
    }

    BufferString comm = fp.fullPath();
    for ( int iinp=startfldidx; iinp<inplst.size(); iinp++ )
    {
	uiGenInput* inp = inplst[iinp];
	mDynamicCastGet(uiFileInput*,finp,inp)
	BufferString val;
	if ( finp )
	{
	    val = "\"";

	    FilePath argfp( finp->fileName() );
	    BufferString arg = argfp.fullPath( FilePath::Windows );
	    val += arg;

	    val += "\"";
	}
	else if ( bpi.args[iinp]->type != BatchProgPar::QWord )
		  val = inp->text();
	else
	    { val = "'"; val += inp->text(); val += "'"; }

	if ( !val.isEmpty() )
	{ comm += " "; comm += val; }
    }

#endif

    StreamProvider sp( comm );
    if ( !sp.executeCommand(true,true) )
	uiMSG().error( "Could not execute command:\n", comm );

    return false;
}


void uiBatchProgLaunch::filenmUpd( CallBacker* cb )
{
    mDynamicCastGet(uiTextFile*,uitf,cb)
    if ( !uitf ) return;

    const int selidx = progfld->box()->currentItem();

    ObjectSet<uiGenInput>& inplst = *inps[selidx];
    for ( int iinp=0; iinp<inplst.size(); iinp++ )
    {
	uiGenInput* inp = inplst[iinp];
	mDynamicCastGet(uiFileInput*,finp,inp)
	BufferString val;
	if ( finp )
	    finp->setText( uitf->fileName() );
    }
}
