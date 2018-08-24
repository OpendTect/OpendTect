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
#include "uimsg.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ioobjctxt.h"
#include "keystrs.h"
#include "survinfo.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(tr("Attribute Sets")),
				     mNoDlgTitle,
				     mODHelpKey(mAttrSetManHelpID) )
			       .nrstatusflds(1).modal(false),
		   AttribDescSetTranslatorGroup::ioContext())
{
    ctxt_.toselect_.dontallow_.set( sKey::Type(), is2d ? "3D" : "2D" );
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


static void fillAttribList( uiListBox* attribfld,
			    const Attrib::DescSet& attrset )
{
    BufferStringSet nms;
    attrset.getAttribNames( nms, false );
    attribfld->addItems( nms );
}


void uiAttrSetMan::ownSelChg()
{
    attribfld_->setEmpty();
    if ( curioobj_ )
    {
	Attrib::DescSet attrset( SI().has2D() );
	uiRetVal uirv = attrset.load( curioobj_->key() );
	if ( uirv.isOK() )
	    fillAttribList( attribfld_, attrset );
    }
}


bool uiAttrSetMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    Attrib::DescSet attrset( SI().has2D() );
    uiRetVal errs = attrset.load( ioobj.key() );
    if ( !errs.isOK() )
	{ inf = errs; return false; }

    addObjInfo( inf, uiStrings::sType(), attrset.is2D() ? uiStrings::s2D()
							: uiStrings::s3D() );
    BufferStringSet nms; attrset.getStoredNames( nms );
    if ( !nms.isEmpty() )
	addObjInfo( inf, uiStrings::sInput(), nms.getDispString() );

    return true;
}
