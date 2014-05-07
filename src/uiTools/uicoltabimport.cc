/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicoltabimport.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "ptrman.h"
#include "coltabsequence.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "settings.h"


static const char* getLabel( bool fromusr )
{ return fromusr ? "User's HOME directory" : "File"; }

static BufferString sHomePath;
static BufferString sFilePath;

uiColTabImport::uiColTabImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Color Tables",mNoDlgTitle,
				 mODHelpKey(mColTabImportHelpID)))
    , dirfld_(0)
    , dtectusrfld_(0)
{
    setOkText( uiStrings::sImport() );

    choicefld_ = new uiGenInput( this, "Import from",
	BoolInpSpec(true,"Other user","File") );
    choicefld_->valuechanged.notify( mCB(this,uiColTabImport,choiceSel) );

    sHomePath = sFilePath = GetPersonalDir();
    dirfld_ = new uiFileInput( this, getLabel(true),
			       uiFileInput::Setup(sHomePath)
			       .directories(true) );
    dirfld_->setReadOnly();
    dirfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );
    dirfld_->attach( alignedBelow, choicefld_ );

    dtectusrfld_ = new uiGenInput( this, "DTECT_USER (if any)" );
    dtectusrfld_->attach( alignedBelow, dirfld_ );
    dtectusrfld_->updateRequested.notify( mCB(this,uiColTabImport,usrSel) );

    listfld_ = new uiLabeledListBox( this, "Color table(s) to add",
				     uiListBox::AtLeastOne,
				     uiLabeledListBox::LeftTop );
    listfld_->attach( alignedBelow, dtectusrfld_ );

    messagelbl_ = new uiLabel( this, "" );
    messagelbl_->setTextColor( Color::Red() );
    messagelbl_->setHSzPol( uiObject::Wide );
    messagelbl_->attach( alignedBelow, dtectusrfld_ );
    messagelbl_->display( false );

    choiceSel( 0 );
}


uiColTabImport::~uiColTabImport()
{
    deepErase( seqs_ );
}


const char* uiColTabImport::getCurrentSelColTab() const
{
    return listfld_->box()->getText();
}


void uiColTabImport::choiceSel( CallBacker* )
{
    const bool fromuser = choicefld_->getBoolValue();
    dirfld_->setSelectMode(
	fromuser ? uiFileDialog::DirectoryOnly : uiFileDialog::ExistingFile );
    dirfld_->setTitleText( getLabel(fromuser) );
    dirfld_->setFileName( fromuser ? sHomePath : sFilePath );

    dtectusrfld_->display( fromuser );
    usrSel(0);
}


#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColTabImport::usrSel( CallBacker* )
{
    PtrMan<IOPar> ctabiop = 0;
    listfld_->box()->setEmpty();

    const bool fromuser = choicefld_->getBoolValue();

    FilePath fp( dirfld_->fileName() );
    if ( !File::exists(fp.fullPath()) )
    {
	uiMSG().error( "Please select an existing ",
		       fromuser ? "directory" : "file" );
	return;
    }

    if ( fromuser )
    {
	sHomePath = fp.fullPath();

	fp.add( ".od" );
	if ( !File::exists(fp.fullPath()) )
	{
	    showMessage( "No '.od' directory found in directory" );
	    return;
	}
	else
	    showList();

	BufferString settdir( fp.fullPath() );
	const char* dtusr = dtectusrfld_->text();
	ctabiop = Settings::fetchExternal( "coltabs", dtusr, settdir );
	if ( !ctabiop )
	{
	    showMessage( "No user-defined color tables found" );
	    return;
	}
	else
	    showList();
    }
    else
    {
	const BufferString fnm = fp.fullPath();
	if ( File::isDirectory(fnm) )
	{
	    showList();
	    return;
	}

	sFilePath = fnm;
	ctabiop = new IOPar;
	bool res = ctabiop->read( fnm, "Default settings" );
	if ( !res )
	{
	    res = ctabiop->read( fnm, 0 );
	    if ( !res )
	    {
		showMessage( "Cannot read color tables from selected file" );
		return;
	    }
	}
    }

    deepErase( seqs_ );
    int nrinvalididx = 0;
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = ctabiop->subselect( idx );
	if ( !subpar || !subpar->size() )
	{
	    delete subpar;
	    nrinvalididx++;
	    if ( nrinvalididx>1000 ) break;
	    else
		continue;
	}
	const char* nm = subpar->find( sKey::Name() );
	if ( !nm )
	    { delete subpar; nrinvalididx++; continue; }

	ColTab::Sequence* seq = new ColTab::Sequence;
	seq->usePar( *subpar );
	seqs_ += seq;
	ioPixmap coltabpix( *seq, 16, 10, true );
	listfld_->box()->addItem( nm, coltabpix );
    }

    if ( listfld_->box()->isEmpty() )
	showMessage( "Cannot read color tables from selected file" );
    else
	showList();
}


bool uiColTabImport::acceptOK( CallBacker* )
{
    bool oneadded = false;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->box()->size(); idx++ )
    {
	if ( listfld_->box()->isChosen(idx) )
	    tobeadded += seqs_[idx];
    }

    for ( int idx=0; idx<tobeadded.size(); idx++ )
    {
	ColTab::Sequence seq( *tobeadded[idx] );
	bool doset = true;
	const int seqidx = ColTab::SM().indexOf( seq.name() );
	if ( seqidx >= 0 )
	{
	    BufferString msg( "User colortable '" );
	    msg += seq.name();
	    msg += "' will replace the existing.\nOverwrite?";
	    doset = uiMSG().askOverwrite( msg );
	}

	if ( doset )
	{
	    oneadded = true;
	    seq.setType( ColTab::Sequence::User );
	    ColTab::SM().set( seq );
	}
    }

    if ( oneadded )
	ColTab::SM().write( false );
    return oneadded;
}


void uiColTabImport::showMessage( const char* msg )
{
    messagelbl_->setText( msg );
    messagelbl_->display( true );
    listfld_->display( false );
}


void uiColTabImport::showList()
{
    messagelbl_->display( false );
    listfld_->display( true );
}
