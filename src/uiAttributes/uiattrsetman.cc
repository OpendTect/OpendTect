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


static void getAttrNms( BufferStringSet& nms, const Attrib::DescSet& attrset,
			bool stor )
{
    const int totnrdescs = attrset.nrDescs( true, true );
    for ( int idx=0; idx<totnrdescs; idx++ )
    {
	const Attrib::Desc& desc = *attrset.desc( idx );
	if ( !desc.isHidden() && stor == desc.isStored() )
	    nms.add( desc.userRef() );
    }
}

static void addAttrNms( const Attrib::DescSet& attrset, BufferString& txt,
			bool stor )
{
    BufferStringSet nms;
    getAttrNms( nms, attrset, stor );
    txt.add( nms.getDispString(2) );
}


static void fillAttribList( uiListBox* attribfld,
			    const Attrib::DescSet& attrset )
{
    BufferStringSet nms;
    getAttrNms( nms, attrset, false );
    attribfld->addItems( nms );
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

	const int nrattrs = attrset.nrDescs( false, false );
	const int nrwithstor = attrset.nrDescs( true, false );
	const int nrstor = nrwithstor - nrattrs;
	txt = "Type: "; txt += attrset.is2D() ? "2D" : "3D";
	if ( nrstor > 0 )
	{
	    txt += "\nInput"; txt += nrstor == 1 ? ": " : "s: ";
	    addAttrNms( attrset, txt, true );
	}
	if ( nrattrs < 1 )
	    txt += "\nNo attributes defined";

	fillAttribList( attribfld_, attrset );
    }

    txt += "\n";
    txt += getFileInfo();
    setInfo(txt);
}
