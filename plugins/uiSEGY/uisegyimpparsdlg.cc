/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyimpparsdlg.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "repos.h"


static const char* sNoSavedYet = "<No saved setups yet>";


uiSEGYImpParsDlg::uiSEGYImpParsDlg( uiParent* p, bool isread, const char* dfnm )
    : uiDialog(p,Setup(isread?tr("Read SEG-Y setup"):tr("Store SEG-Y setup"),
			mNoDlgTitle,mNoHelpKey))
    , parset_(*new Repos::IOParSet("SEGYSetup"))
    , parname_(dfnm)
{
    BufferStringSet nms;
    for ( int idx=0; idx<parset_.size(); idx++ )
	nms.add( parset_[idx]->name() );

    if ( nms.size() > 1 )
	nms.sort();
    else if ( nms.isEmpty() && isread )
	nms.add( sNoSavedYet );

    listfld_ = new uiListBox( this, "Stored Setups" );
    listfld_->addItems( nms );
    listfld_->selectionChanged.notify( mCB(this,uiSEGYImpParsDlg,selChgCB) );
}


uiSEGYImpParsDlg::~uiSEGYImpParsDlg()
{
    delete &parset_;
}


void uiSEGYImpParsDlg::selChgCB( CallBacker* )
{
    selectionChanged();
}


bool uiSEGYImpParsDlg::acceptOK( CallBacker* )
{
    return doIO();
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

    mErrRet( tr("TODO: Not impl yet") );
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

    const Repos::Source targetsrc = Repos::Data; //TODO? make user selectable?
    parstostore_->setName( parnm );
    parstostore_->src_ = targetsrc;
    parset_.add( parstostore_ );
    parstostore_ = 0;
    if ( !parset_.write(targetsrc) )
	mErrRet( tr("Cannot write to:\n%1").arg(parset_.fileName(targetsrc)) );

    return true;
}
