/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiautoattrdescset.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribdescsettr.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiattrdescseted.h"



using namespace Attrib;


uiAutoAttrSelDlg::uiAutoAttrSelDlg( uiParent* p, bool is2d )
        : uiDialog(p,uiDialog::Setup("Auto-load Attribute Set",
		                     "Set auto-load Attribute-Set",
				     "101.1.5"))
        , ctio_(*mMkCtxtIOObj(AttribDescSet))
	, is2d_(is2d)
{
    bool douse = false; MultiID id;
    Settings::common().getYN( uiAttribDescSetEd::sKeyUseAutoAttrSet, douse );
    id = is2d_ ? SI().pars().find( uiAttribDescSetEd::sKeyAuto2DAttrSetID )
	       : SI().pars().find( uiAttribDescSetEd::sKeyAuto3DAttrSetID );

    usefld_ = new uiGenInput( this, "Enable auto-load Attribute Set",
                                  BoolInpSpec(true) );
    usefld_->setValue( douse );
    usefld_->valuechanged.notify( mCB(this,uiAutoAttrSelDlg,useChg) );

    ctio_.setObj( id ); ctio_.ctxt.forread = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, usefld_ );
    lbl_ = new uiLabel( this, "Attribute Set to use" );
    lbl_->attach( centeredLeftOf, selgrp_ );
    
    loadbutton_ = new uiCheckBox( this, "Load Now" );
    loadbutton_->attach( alignedBelow, selgrp_ ); 

    postFinalise().notify( mCB(this,uiAutoAttrSelDlg,useChg) );
}


uiAutoAttrSelDlg::~uiAutoAttrSelDlg()
{   
    delete ctio_.ioobj; delete &ctio_;
}


IOObj* uiAutoAttrSelDlg::getObj()
{
    return ctio_.ioobj;
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
    const bool douse = usefld_->getBoolValue();
    selgrp_->display( douse );
    lbl_->display( douse );
    loadbutton_->display( douse );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiAutoAttrSelDlg::acceptOK( CallBacker* )
{   
    if ( !usefld_->getBoolValue() )
	return true;

    selgrp_->processInput();
    if ( selgrp_->nrSel() < 1 )
        mErrRet("Please select an Attribute Set")
    
    ctio_.setObj( selgrp_->selected() );
    Attrib::DescSet attrset( is2d_ );
    BufferString bs;
    if ( !AttribDescSetTranslator::retrieve(attrset,ctio_.ioobj,bs) )
	mErrRet( "Cannot read selected attribute set" )

    if ( attrset.is2D() != is2d_ )
    {
	bs = "Attribute Set "; bs += ctio_.ioobj->name();
	bs += " is of type "; bs += attrset.is2D() ? "2D" : "3D";
	uiMSG().error( bs.buf(), "Please select another attribute set" );
	return false;
    }

    return true;
}


uiAutoAttrSetOpen::uiAutoAttrSetOpen( uiParent* p, BufferStringSet& afl,
				BufferStringSet& anm)
	: uiDialog(p,uiDialog::Setup("Open Attribute Set",
	                             "Select an Attribute-Set to open",		                                      "100.0.0"))
        , ctio_(*mMkCtxtIOObj(AttribDescSet))
	, attribfiles_(afl)
	, attribnames_(anm)
{
    defselfld_ = new uiGenInput( this, "Select from", BoolInpSpec(true,
				"Survey-defined sets", "Default sets") );
    defselfld_->valuechanged.notify( mCB(this,uiAutoAttrSetOpen, setChg) );

    autoloadfld_ = new uiGenInput( this, "", BoolInpSpec(false,
		                "Set for Auto-Load\t", "Disable Auto-load") );
    autoloadfld_->attach( alignedBelow, defselfld_ );

    ctio_.ctxt.forread = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, autoloadfld_ );

    defattrlist_ = new uiLabeledListBox( this, attribnames_, "Deafault Sets" );
    defattrlist_->attach( alignedBelow, autoloadfld_ );

    lbl_ = new uiLabel( this, "Survey-defined sets" );
    lbl_->attach( leftTo, selgrp_ );

    postFinalise().notify( mCB(this,uiAutoAttrSetOpen,setChg) );
}


uiAutoAttrSetOpen::~uiAutoAttrSetOpen()
{
    delete ctio_.ioobj; delete &ctio_;
}


IOObj* uiAutoAttrSetOpen::getObj()
{
    return usrdef_ ? ctio_.ioobj : 0;
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
    if ( !usrdef_ ) autoloadfld_->setValue( false );
    autoloadfld_->setSensitive( usrdef_ );
}

bool uiAutoAttrSetOpen::acceptOK( CallBacker* )
{
    usrdef_ = defselfld_->getBoolValue();
    selgrp_->processInput();
    if ( usrdef_ && selgrp_->nrSel() < 1 )
	{ uiMSG().error("Please select an Attribute Set"); return false; }
    if ( selgrp_->nrSel() > 0 )
	ctio_.setObj( selgrp_->selected(0) );
    if ( !usrdef_ && defattrlist_->box()->nrSelected() < 1 )
        { uiMSG().error("Please select an Attribute Set"); return false; }
    if ( defattrlist_->box()->nrSelected() == 1 )
	defselid_ = defattrlist_->box()->currentItem();

    isauto_ = autoloadfld_->getBoolValue();
    return true;
}
