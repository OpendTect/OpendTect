/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          June 2007
________________________________________________________________________

-*/

#include "uiautoattrdescset.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uiattrdescseted.h"
#include "od_helpids.h"



using namespace Attrib;


uiAutoAttrSelDlg::uiAutoAttrSelDlg( uiParent* p, bool is2d )
        : uiDialog(p,uiDialog::Setup(tr("Auto-load Attribute Set"),
		                     tr("Set auto-load Attribute-Set"),
				     mODHelpKey(mAutoAttrSelDlgHelpID) ))
        , ctio_(*mMkCtxtIOObj(AttribDescSet))
	, is2d_(is2d)
{
    bool douse = false;
    Settings::common().getYN( Attrib::DescSet::sKeyUseAutoAttrSet, douse );
    BufferString idstr = is2d_
	? SI().getDefaultPars().find(Attrib::DescSet::sKeyAuto2DAttrSetID)
	: SI().getDefaultPars().find(Attrib::DescSet::sKeyAuto3DAttrSetID);

    usefld_ = new uiGenInput( this, tr("Enable auto-load Attribute Set"),
                                  BoolInpSpec(true) );
    usefld_->setValue( douse );
    usefld_->valuechanged.notify( mCB(this,uiAutoAttrSelDlg,useChg) );

    ctio_.setObj( DBKey(idstr) );
    ctio_.ctxt_.forread_ = true;
    ctio_.ctxt_.toselect_.dontallow_.set( sKey::Type(), is2d ? "3D" : "2D" );
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, usefld_ );
    lbl_ = new uiLabel( this, tr("Attribute Set to use") );
    lbl_->attach( centeredLeftOf, selgrp_ );

    loadbutton_ = new uiCheckBox( this, tr("Load Now") );
    loadbutton_->attach( alignedBelow, selgrp_ );

    postFinalise().notify( mCB(this,uiAutoAttrSelDlg,useChg) );
}


uiAutoAttrSelDlg::~uiAutoAttrSelDlg()
{
    delete ctio_.ioobj_; delete &ctio_;
}


IOObj* uiAutoAttrSelDlg::getObj()
{
    return ctio_.ioobj_;
}


bool uiAutoAttrSelDlg::useAuto()
{
    return usefld_->getBoolValue();
}


bool uiAutoAttrSelDlg::loadAuto()
{
    return useAuto() && loadbutton_->isChecked();
}


void uiAutoAttrSelDlg::useChg( CallBacker* )
{
    const bool douse = useAuto();
    selgrp_->display( douse );
    lbl_->display( douse );
    loadbutton_->display( douse );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiAutoAttrSelDlg::acceptOK()
{
    if ( !useAuto() )
	return true;

    if ( selgrp_->nrChosen() < 1 )
        mErrRet( tr("No Attribute Sets available") )

    ctio_.setObj( selgrp_->chosenID() );
    Attrib::DescSet attrset( is2d_ );
    uiRetVal uirv = attrset.load( selgrp_->chosenID() );
    if ( !uirv.isOK() )
	mErrRet( uirv )

    return true;
}


uiAutoAttrSetOpen::uiAutoAttrSetOpen( uiParent* p, BufferStringSet& afl,
				BufferStringSet& anm)
	: uiDialog(p,uiDialog::Setup(tr("Open Attribute Set"),
	                   uiStrings::phrSelect(tr("an Attribute-Set to open")),
                           mNoHelpKey))
        , ctio_(*mMkCtxtIOObj(AttribDescSet))
	, attribfiles_(afl)
	, attribnames_(anm)
{
    uiString defaultsets = tr("Default Sets");
    defselfld_ = new uiGenInput( this, uiStrings::phrSelect(tr("from")),
		     BoolInpSpec(true,tr("Survey-defined sets"),defaultsets) );
    defselfld_->valuechanged.notify( mCB(this,uiAutoAttrSetOpen, setChg) );

    autoloadfld_ = new uiGenInput( this, uiString::empty(),
                                   BoolInpSpec(false, tr("Set for Auto-Load\t"),
                                   tr("Disable Auto-load")) );
    autoloadfld_->attach( alignedBelow, defselfld_ );

    ctio_.ctxt_.forread_ = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, autoloadfld_ );

    uiListBox::Setup su( OD::ChooseOnlyOne, defaultsets );
    defattrlist_ = new uiListBox( this, su );
    defattrlist_->addItems( attribnames_ );
    defattrlist_->attach( alignedBelow, autoloadfld_ );

    lbl_ = new uiLabel( this, tr("Survey-defined sets") );
    lbl_->attach( leftTo, selgrp_ );

    postFinalise().notify( mCB(this,uiAutoAttrSetOpen,setChg) );
}


uiAutoAttrSetOpen::~uiAutoAttrSetOpen()
{
    delete ctio_.ioobj_; delete &ctio_;
}


IOObj* uiAutoAttrSetOpen::getObj()
{
    return usrdef_ ? ctio_.ioobj_ : 0;
}


const char* uiAutoAttrSetOpen::getAttribname()
{
    return usrdef_ ? "" : attribnames_[defselid_]->buf();
}


const char* uiAutoAttrSetOpen::getAttribfile()
{
    return usrdef_ ? 0 : attribfiles_[defselid_]->buf();
}


void uiAutoAttrSetOpen::setChg( CallBacker* )
{
    usrdef_ = defselfld_->getBoolValue();
    selgrp_->display( usrdef_ );
    lbl_->display( usrdef_ );
    defattrlist_->display( !usrdef_ );
    if ( !usrdef_ )
	autoloadfld_->setValue( false );
    autoloadfld_->setSensitive( usrdef_ );
}

bool uiAutoAttrSetOpen::acceptOK()
{
    usrdef_ = defselfld_->getBoolValue();
    if ( usrdef_ && selgrp_->nrChosen() < 1 )
	{ uiMSG().error(tr("No Attribute Set available")); return false; }

    ctio_.setObj( selgrp_->chosenID() );
    defselid_ = defattrlist_->currentItem();

    isauto_ = autoloadfld_->getBoolValue();
    return true;
}
