/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrsetman.h"

#include "uiioobjselgrp.h"
#include "uilistbox.h"

#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ctxtioobj.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "survinfo.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,Setup(uiStrings::phrManage(tr("Attribute Sets")),
			   mODHelpKey(mAttrSetManHelpID))
			.nrstatusflds(1).modal(false),
		   AttribDescSetTranslatorGroup::ioContext())
{
    ctxt_.toselect_.dontallow_.set( sKey::Type(), is2d ? "3D" : "2D" );
    createDefaultUI();
    setPrefWidth( 50 );

    uiListBox::Setup su( OD::ChooseNone, uiStrings::sAttribute(2),
			 uiListBox::AboveMid );
    attribfld_ = new uiListBox( listgrp_, su, "attributes" );
    attribfld_->attach( rightOf, selgrp_ );
    attribfld_->setHSzPol( uiObject::Wide );

    mTriggerInstanceCreatedNotifier();
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
    attribfld->addItems( nms );
}


void uiAttrSetMan::mkFileInfo()
{
    attribfld_->setEmpty();
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    uiString errmsg;
    Attrib::DescSet attrset(!SI().has3D());
    if ( AttribDescSetTranslator::retrieve(attrset,curioobj_,errmsg) )
    {
	if (!errmsg.isEmpty())
	    ErrMsg(errmsg.getFullString());

	txt = "Type: "; txt += attrset.is2D() ? "2D" : "3D";
	txt += "\nInput: ";
	addStoredNms( attrset, txt );

	fillAttribList( attribfld_, attrset );
	txt += "\n";
    }

    txt += getFileInfo();
    setInfo(txt);
}
