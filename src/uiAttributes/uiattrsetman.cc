/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uiattrsetman.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uitextedit.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ioobjctxt.h"
#include "survinfo.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(tr("Attribute Sets")),
				     mNoDlgTitle,
				     mODHelpKey(mAttrSetManHelpID) )
			       .nrstatusflds(1).modal(false),
		   AttribDescSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    setPrefWidth( 50 );

    uiListBox::Setup su( OD::ChooseNone, uiStrings::sAttribute(2),
			 uiListBox::AboveMid );
    attribfld_ = new uiListBox( listgrp_, su );
    attribfld_->attach( rightOf, selgrp_ );
    attribfld_->setHSzPol( uiObject::Wide );

    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiAttrSetMan::~uiAttrSetMan()
{
}


static void addStoredNms( const Attrib::DescSet& attrset, BufferString& txt )
{
    BufferStringSet nms;
    attrset.getStoredNames( nms );
    txt.add( nms.getDispString() );
}


static void fillAttribList( uiListBox* attribfld,
			    const Attrib::DescSet& attrset )
{
    BufferStringSet nms;
    attrset.getAttribNames( nms, false );
    attribfld->addItems( nms.getUiStringSet() );
    attribfld->addItems( nms.getUiStringSet() );
}


void uiAttrSetMan::mkFileInfo()
{
    attribfld_->setEmpty();
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    uiString errmsg;
    Attrib::DescSet attrset(!SI().has3D());
    if (!AttribDescSetTranslator::retrieve(attrset, curioobj_, errmsg))
    {
	BufferString msg("Read error: '"); msg += errmsg.getFullString();
	msg += "'"; txt = msg;
    }
    else
    {
	if (!errmsg.isEmpty())
	    ErrMsg(errmsg.getFullString());

	txt = "Type: "; txt += attrset.is2D() ? "2D" : "3D";
	txt += "\nInput: ";
	addStoredNms( attrset, txt );

	fillAttribList( attribfld_, attrset );
    }

    txt += "\n";
    txt += getFileInfo();
    setInfo(txt);
}
