/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/

#include "uicolseqimport.h"
#include "uicolseqdisp.h"

#include "uifilesel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabseqmgr.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "settings.h"




static BufferString sHomePath;
static BufferString sFilePath;

uiColSeqImport::uiColSeqImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(
			uiStrings::phrImport(uiStrings::sColorTable()),
			mNoDlgTitle, mODHelpKey(mColTabImportHelpID))
			.modal(false))
    , dirfld_(0)
    , dtectusrfld_(0)
{
    setOkText( uiStrings::sImport() );

    choicefld_ = new uiGenInput( this, tr("Import from"),
	BoolInpSpec(true,tr("Other user"), uiStrings::sFile()) );
    choicefld_->valuechanged.notify( mCB(this,uiColSeqImport,choiceSel) );

    sHomePath = sFilePath = GetPersonalDir();
    uiFileSel::Setup fssu( sHomePath );
    fssu.selectDirectory();
    dirfld_ = new uiFileSel( this, getLabelText(true), fssu );
    dirfld_->newSelection.notify( mCB(this,uiColSeqImport,usrSel) );
    dirfld_->attach( alignedBelow, choicefld_ );

    dtectusrfld_ = new uiGenInput( this, tr("DTECT_USER (if any)") );
    dtectusrfld_->attach( alignedBelow, dirfld_ );
    dtectusrfld_->updateRequested.notify( mCB(this,uiColSeqImport,usrSel) );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Color table(s) to add") );
    su.lblpos( uiListBox::LeftTop );
    listfld_ = new uiListBox( this, su );
    listfld_->attach( alignedBelow, dtectusrfld_ );

    messagelbl_ = new uiLabel( this, uiString::empty() );
    messagelbl_->setTextColor( Color::Red() );
    messagelbl_->setHSzPol( uiObject::Wide );
    messagelbl_->attach( alignedBelow, dtectusrfld_ );
    messagelbl_->display( false );

    choiceSel( 0 );
}


uiColSeqImport::~uiColSeqImport()
{
    deepUnRef( seqs_ );
}


const char* uiColSeqImport::currentSeqName() const
{
    return listfld_->getText();
}


void uiColSeqImport::choiceSel( CallBacker* )
{
    const bool fromuser = choicefld_->getBoolValue();
    dirfld_->setSelectionMode( fromuser ? OD::SelectDirectory
					: OD::SelectFileForRead );
    dirfld_->setLabelText( getLabelText(fromuser) );
    dirfld_->setFileName( fromuser ? sHomePath : sFilePath );

    dtectusrfld_->display( fromuser );
    usrSel(0);
}


uiString uiColSeqImport::getLabelText( bool fromusr )
{
    uiString ret = fromusr ?
                    tr("User's HOME directory") : uiStrings::sFile();
    return ret;
}

#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColSeqImport::usrSel( CallBacker* )
{
    PtrMan<IOPar> cseqiop = 0;
    listfld_->setEmpty();

    const bool fromuser = choicefld_->getBoolValue();

    File::Path fp( dirfld_->fileName() );
    if ( !File::exists(fp.fullPath()) )
    {
	uiMSG().error(uiStrings::phrSelect(tr("an existing %1")
		    .arg(fromuser ? uiStrings::sDirectory().toLower()
			          : uiStrings::sFile().toLower() )));
	return;
    }

    uiPhrase cannotreadclrtbl = uiStrings::phrCannotRead(
				    tr("ColorTable from Selected File"));

    if ( fromuser )
    {
	sHomePath = fp.fullPath();

	fp.add( ".od" );
	if ( !File::exists(fp.fullPath()) )
	{
	    showMessage( tr("No '.od' directory found in directory") );
	    return;
	}
	else
	    showList();

	BufferString settdir( fp.fullPath() );
	const char* dtusr = dtectusrfld_->text();
	cseqiop = Settings::fetchExternal( "coltabs", dtusr, settdir );
	if ( !cseqiop )
	{
	    showMessage( tr("No user-defined color tables found") );
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
	cseqiop = new IOPar;
	bool res = cseqiop->read( fnm, "Default settings" );
	if ( !res )
	{
	    res = cseqiop->read( fnm, 0 );
	    if ( !res )
	    {
		showMessage(cannotreadclrtbl);
		return;
	    }
	}
    }

    deepUnRef( seqs_ );
    int nrinvalididx = 0;
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = cseqiop->subselect( idx );
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
	uiPixmap* pm = ColTab::getuiPixmap( *seq, 16, 10 );
	listfld_->addItem( toUiString(nm), *pm );
	delete pm;
    }

    if ( listfld_->isEmpty() )
	showMessage(cannotreadclrtbl);
    else
	showList();
}


bool uiColSeqImport::acceptOK()
{
    int nrdone = 0;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    tobeadded += seqs_[idx];
    }

    for ( int idx=0; idx<tobeadded.size(); idx++ )
    {
	RefMan<ColTab::Sequence> seq = new ColTab::Sequence( *tobeadded[idx] );
	ConstRefMan<ColTab::Sequence> existseq
			= ColTab::SeqMGR().getByName( seq->name() );
	bool doimp = true;
	if ( existseq )
	{
	    uiString msg = tr("There already is a colortable '%1'."
			      "\nDo you want to replace it?").arg(seq->name());
	    doimp = uiMSG().askOverwrite( msg );
	}

	if ( doimp )
	{
	    nrdone++;
	    if ( existseq )
		*const_cast<ColTab::Sequence*>(existseq.ptr()) = *seq;
	    else
		ColTab::SeqMGR4Edit().add( seq );
	}
    }

    bool stay = true;
    if ( nrdone > 0 )
    {
	uiString msg = tr("Successfully obtained %1 %2."
			  "\n\nDo you want to import more Color tables?")
	    .arg( nrdone ).arg( uiStrings::sColorTable(nrdone) );
	stay = uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window"));
    }

    return !stay;
}


void uiColSeqImport::showMessage( const uiString& msg )
{
    messagelbl_->setText( msg );
    messagelbl_->display( true );
    listfld_->display( false );
}


void uiColSeqImport::showList()
{
    messagelbl_->display( false );
    listfld_->display( true );
}
