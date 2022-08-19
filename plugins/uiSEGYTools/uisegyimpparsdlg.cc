/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyimpparsdlg.h"

#include "uibutton.h"
#include "uitextedit.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "repos.h"


static const char* sNoSavedYet = "<No saved setups yet>";
static const Repos::Source cSrcToManage = Repos::Data;


uiSEGYImpParsDlg::uiSEGYImpParsDlg( uiParent* p, bool isread, const char* dfnm )
    : uiDialog(p,Setup(isread?tr("Read SEG-Y setup"):tr("Store SEG-Y setup"),
			mNoDlgTitle,mNoHelpKey))
    , parset_(*new Repos::IOParSet("SEGYSetups"))
    , parname_(dfnm)
    , setchgd_(false)
{
    listfld_ = new uiListBox( this, "Stored Setups" );
    fillList();
    listfld_->setPrefWidthInChar( 50 );
    listfld_->selectionChanged.notify( mCB(this,uiSEGYImpParsDlg,selChgCB) );

    renbut_ = uiButton::getStd( this, OD::Rename,
				mCB(this,uiSEGYImpParsDlg,renCB), true );
    renbut_->attach( rightOf, listfld_ );
    delbut_ = uiButton::getStd( this, OD::Remove,
				mCB(this,uiSEGYImpParsDlg,delCB), true );
    delbut_->attach( alignedBelow, renbut_ );

    updateButtons();
    postFinalize().notify( mCB(this,uiSEGYImpParsDlg,selChgCB) );
}


uiSEGYImpParsDlg::~uiSEGYImpParsDlg()
{
    delete &parset_;
}


void uiSEGYImpParsDlg::fillList()
{
    BufferStringSet nms;
    for ( int idx=0; idx<parset_.size(); idx++ )
	nms.add( parset_[idx]->name() );
    nms.sort();
    listfld_->addItems( nms );
}


int uiSEGYImpParsDlg::parIdx() const
{
    const BufferString nm( listfld_->getText() );
    return parset_.find( listfld_->getText() );
}


void uiSEGYImpParsDlg::renCB( CallBacker* )
{
    const int paridx = parIdx();
    if ( paridx < 0 )
	return;

    const BufferString oldnm( listfld_->getText() );
    uiString titl( uiStrings::phrRename(toUiString("'%1'")).arg( oldnm ) );
    uiGenInputDlg dlg( this, titl, mJoinUiStrs(sNew(),sName()),
			new StringInpSpec(oldnm) );
    if ( !dlg.go() )
	return;

    const BufferString newnm = dlg.text();
    if ( newnm.isEmpty() || newnm == oldnm )
	return;

    parset_[paridx]->setName( newnm );
    setchgd_ = true;

    update( newnm );
}


void uiSEGYImpParsDlg::delCB( CallBacker* )
{
    const int newselidx = listfld_->currentItem() - 1;
    BufferString newselnm;
    if ( newselidx >= 0 )
	newselnm = listfld_->textOfItem( newselidx );

    const int remidx = parset_.find( listfld_->getText() );
    if ( remidx < 0 )
	{ pErrMsg("Huh remove"); return; }

    parset_.removeSingle( remidx );
    setchgd_ = true;

    update( newselnm.isEmpty() ? 0 : newselnm.str() );
}



void uiSEGYImpParsDlg::selChgCB( CallBacker* )
{
    selectionChanged();
    updateButtons();
}


void uiSEGYImpParsDlg::update( const char* tosel )
{
    NotifyStopper stopper( listfld_->selectionChanged );

    listfld_->setEmpty(); fillList();
    if ( tosel )
	listfld_->setCurrentItem( tosel );

    updateButtons();
}


void uiSEGYImpParsDlg::updateButtons()
{
    const int paridx = parIdx();
    bool isactive = paridx >= 0;
    if ( isactive )
    {
	Repos::IOPar& iop = *parset_[paridx];
	if ( iop.src_ != cSrcToManage )
	    isactive = false;
    }

    renbut_->setSensitive( isactive );
    delbut_->setSensitive( isactive );
}


bool uiSEGYImpParsDlg::acceptOK( CallBacker* )
{
    if ( !doIO() )
	return false;

    if ( setchgd_ && !parset_.write(cSrcToManage) )
    {
	uiMSG().error(
		tr("Cannot write changes.\nPlease check permissions on %1.")
		     .arg( parset_.fileName(cSrcToManage) ) );
	return false;
    }

    return true;
}


uiSEGYReadImpParsDlg::uiSEGYReadImpParsDlg( uiParent* p, const char* defnm )
    : uiSEGYImpParsDlg(p,true,defnm)
{
    setHelpKey( mODHelpKey(mSEGYReadImpParsDlgHelpID) );

    detailsfld_ = new uiTextEdit( this, "Entry details", true );
    detailsfld_->setPrefHeightInChar( 5 );
    detailsfld_->attach( alignedBelow, listfld_ );
    detailsfld_->setStretch( 2, 1 );
}


void uiSEGYReadImpParsDlg::selectionChanged()
{
    const int paridx = parIdx();
    if ( paridx < 0 )
	detailsfld_->setText( "" );
    else
    {
	const Repos::IOPar& iop = *parset_[paridx];

	uiString todisp =
	    tr( "Origin: %1 (%2)"
		"\nSEG-Y Revision: %3"
		"\nData type: %4"
		"\nCreated by %5 at %6." )
		.arg( Repos::isUserDefined(iop.src_) ? tr("User-defined")
						     : tr("Internal") )
		.arg( Repos::descriptionOf(iop.src_) )
		.arg( iop.find(SEGY::FilePars::sKeyRevision()) )
		.arg( iop.find("Import.Type") )
		.arg( iop.find(sKey::CrBy()) )
		.arg( iop.find(sKey::CrAt()) );
	detailsfld_->setText( todisp );
    }
}


const IOPar* uiSEGYReadImpParsDlg::pars() const
{
    const int idx = parset_.find( parname_ );
    return idx < 0 ? 0 : parset_[idx];
}


#define mErrRet(s) { uiMSG().error( s ); return false; }


bool uiSEGYReadImpParsDlg::doIO()
{
    parname_ = listfld_->getText();
    if ( !pars() )
    {
	if ( parname_ != sNoSavedYet )
	    mErrRet( uiStrings::phrSelect(tr("SEG-Y Setup")) )
	else
	    return false;
    }

    return true;
}


uiSEGYStoreImpParsDlg::uiSEGYStoreImpParsDlg( uiParent* p, const IOPar& iop,
					      const char* defnm )
    : uiSEGYImpParsDlg(p,false,defnm)
    , parstostore_(new Repos::IOPar(iop))
{
    setHelpKey( mODHelpKey(mSEGYStoreImpParsDlgHelpID) );

    namefld_ = new uiGenInput( this, tr("Store as"), StringInpSpec(defnm) );
    namefld_->attach( alignedBelow, listfld_ );
}


uiSEGYStoreImpParsDlg::~uiSEGYStoreImpParsDlg()
{
    delete parstostore_;
}


void uiSEGYStoreImpParsDlg::selectionChanged()
{
    namefld_->setText( listfld_->getText() );
}


bool uiSEGYStoreImpParsDlg::doIO()
{
    const BufferString parnm( namefld_->text() );
    if ( parnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("a name for this entry")) )

    parstostore_->setName( parnm );
    parstostore_->setStdCreationEntries();
    parstostore_->src_ = cSrcToManage;
    parset_.add( parstostore_ );
    parstostore_ = 0;
    setchgd_ = true;

    return true;
}
