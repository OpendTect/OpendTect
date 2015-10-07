/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyimpparsdlg.h"
#include "uibutton.h"
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
    listfld_->selectionChanged.notify( mCB(this,uiSEGYImpParsDlg,selChgCB) );

    renbut_ = uiButton::getStd( this, uiButton::Rename,
				mCB(this,uiSEGYImpParsDlg,renCB), true );
    renbut_->attach( rightOf, listfld_ );
    delbut_ = uiButton::getStd( this, uiButton::Remove,
				mCB(this,uiSEGYImpParsDlg,delCB), true );
    delbut_->attach( alignedBelow, renbut_ );

    updateButtons();
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


void uiSEGYImpParsDlg::renCB( CallBacker* )
{
    const BufferString oldnm( listfld_->getText() );
    const int paridx = parset_.find( oldnm );
    if ( paridx < 0 )
	{ pErrMsg("Huh rename"); return; }

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
    const BufferString curitm( listfld_->getText() );
    bool isactive = !curitm.isEmpty();
    if ( isactive )
    {
	const int paridx = parset_.find( curitm );
	if ( paridx < 0 )
	    { pErrMsg("Huh update"); isactive = false; }
	else
	{
	    Repos::IOPar& iop = *parset_[paridx];
	    if ( iop.src_ != cSrcToManage )
		isactive = false;
	}
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
		tr("Could not write changes.\nPlease check permissions on %1.")
		     .arg( parset_.fileName(cSrcToManage) ) );
	return false;
    }

    return true;
}


uiSEGYReadImpParsDlg::uiSEGYReadImpParsDlg( uiParent* p, const char* defnm )
    : uiSEGYImpParsDlg(p,true,defnm)
{
    setHelpKey( mTODOHelpKey );
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
    setHelpKey( mTODOHelpKey );

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
	mErrRet( tr("Please enter a name for this entry") )

    parstostore_->setName( parnm );
    parstostore_->src_ = cSrcToManage;
    parset_.add( parstostore_ );
    parstostore_ = 0;
    setchgd_ = true;

    return true;
}
