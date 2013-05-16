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
#include "uilistbox.h"
#include "uimsg.h"

#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "filepath.h"
#include "file.h"
#include "coltabsequence.h"
#include "settings.h"


static const char* getLabel( bool fromusr )
{ return fromusr ? "User's HOME directory" : "File"; }

uiColTabImport::uiColTabImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Color Table",mNoDlgTitle,"50.1.4"))
    , dirfld_(0)
    , dtectusrfld_(0)
{
    choicefld_ = new uiGenInput( this, "Import from",
	BoolInpSpec(true,"Other user","File") );
    choicefld_->valuechanged.notify( mCB(this,uiColTabImport,choiceSel) );

    FilePath fp( GetPersonalDir() ); //fp.setFileName( 0 );
    dirfld_ = new uiFileInput( this, getLabel(true),
			       uiFileInput::Setup(fp.fullPath())
			       .directories(true) );
    dirfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );
    dirfld_->attach( alignedBelow, choicefld_ );

    dtectusrfld_ = new uiGenInput( this, "DTECT_USER (if any)" );
    dtectusrfld_->attach( alignedBelow, dirfld_ );
    dtectusrfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );

    listfld_ = new uiLabeledListBox( this, "Color table(s) to add", true );
    listfld_->attach( alignedBelow, dtectusrfld_ );

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
    usrSel( 0 );
    const bool fromuser = choicefld_->getBoolValue();
    dirfld_->setSelectMode(
	fromuser ? uiFileDialog::DirectoryOnly : uiFileDialog::ExistingFile );
    dirfld_->setTitleText( getLabel(fromuser) );

    dtectusrfld_->display( fromuser );
}


#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColTabImport::usrSel( CallBacker* )
{
    PtrMan<IOPar> ctabiop = 0;
    listfld_->box()->setEmpty();

    FilePath fp( dirfld_->fileName() );
    if ( !File::exists(fp.fullPath()) )
	mErrRet( "Please select an existing directory" );

    if ( choicefld_->getBoolValue() )
    {
	fp.add( ".od" );
	if ( !File::exists(fp.fullPath()) )
	    mErrRet( "No '.od' directory found in directory" );

	BufferString settdir( fp.fullPath() );
	const char* dtusr = dtectusrfld_->text();
	ctabiop = Settings::fetchExternal( "coltabs", dtusr, settdir );
	if ( !ctabiop )
	    mErrRet( "No user-defined color tables found" );
    }
    else
    {
	if ( File::isDirectory(fp.fullPath()) )
	    return;

	ctabiop = new IOPar;
	bool res =
	    ctabiop->read( fp.fullPath(), "Default settings" );
	if ( !res )
	{
	    res = ctabiop->read( fp.fullPath(), 0 );
	    if ( !res )
		mErrRet( "Cannot read color tables from selected file" );
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
}


bool uiColTabImport::acceptOK( CallBacker* )
{
    bool oneadded = false;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->box()->size(); idx++ )
    {
	if ( listfld_->box()->isSelected(idx) )
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
