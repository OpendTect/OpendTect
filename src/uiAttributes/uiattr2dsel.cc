/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Nov 2007
 RCS:           $Id: uiattr2dsel.cc,v 1.1 2007-12-06 11:10:23 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiattr2dsel.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "hilbertattrib.h"

#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "linekey.h"
#include "cubesampling.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

using namespace Attrib;


uiAttr2DSelDlg::uiAttr2DSelDlg( uiParent* p, const DescSet* ds,
				const MultiID& lsid, const char* curnm )
	: uiDialog(p,Setup("Select Attribute","Select Attribute",0))
	, setid_(lsid)
	, descid_(-1,true)
	, seltype_(0)
	, selgrp_(0)
	, storoutfld_(0)
	, attroutfld_(0)
{
    attrinf_ = new SelInfo( ds, 0, true );

    const bool haveattribs = attrinf_->attrnms.size();

    createSelectionButtons();
    createSelectionFields();

    if ( curnm && *curnm )
    {
	if ( storoutfld_ && storoutfld_->isPresent(curnm) )
	    storoutfld_->setCurrentItem( curnm );

	else if ( attroutfld_ && attroutfld_->isPresent(curnm) )
	{
	    seltype_ = 1;
	    attroutfld_->setCurrentItem( curnm );
	}
    }

    selgrp_->selectButton( seltype_ );
    finaliseStart.notify( mCB( this,uiAttr2DSelDlg,doFinalise) );
}


uiAttr2DSelDlg::~uiAttr2DSelDlg()
{
    delete selgrp_;
    delete attrinf_;
}


void uiAttr2DSelDlg::doFinalise( CallBacker* )
{
    selDone(0);
}


void uiAttr2DSelDlg::createSelectionButtons()
{
    const bool haveattribs = attrinf_->attrnms.size();

    selgrp_ = new uiButtonGroup( this, "Input selection" );
    storfld_ = new uiRadioButton( selgrp_, "Stored" );
    storfld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );

    attrfld_ = new uiRadioButton( selgrp_, "Attributes" );
    attrfld_->setSensitive( haveattribs );
    attrfld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );
}


void uiAttr2DSelDlg::createSelectionFields()
{
    const bool haveattribs = attrinf_->attrnms.size();

    BufferStringSet nms;
    SelInfo::getAttrNames( setid_, nms );

    storoutfld_ = new uiListBox( this, nms, "Stored cubes", false );
    storoutfld_->setHSzPol( uiObject::Wide );
    storoutfld_->setCurrentItem( 0 );
    storoutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
    storoutfld_->attach( rightOf, selgrp_ );

    if ( haveattribs )
    {
	attroutfld_ = new uiListBox( this, attrinf_->attrnms,
				     "Attributes", false );
	attroutfld_->setHSzPol( uiObject::Wide );
	attroutfld_->setCurrentItem( 0 );
	attroutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
	attroutfld_->attach( rightOf, selgrp_ );
    }
}


int uiAttr2DSelDlg::selType() const
{
    if ( attrfld_->isChecked() )
	return 1;
    return 0;
}


void uiAttr2DSelDlg::selDone( CallBacker* c )
{
    if ( !selgrp_ ) return;

    mDynamicCastGet(uiRadioButton*,but,c);
   
    bool dosrc, docalc; 
    if ( but == storfld_ )
    { dosrc = true; docalc = false; }
    else if ( but == attrfld_ )
    { docalc = true; dosrc = false; }

    const int seltyp = selType();
    if ( attroutfld_ ) attroutfld_->display( seltyp == 1 );
    if ( storoutfld_ )
	storoutfld_->display( seltyp == 0 );
}


bool uiAttr2DSelDlg::acceptOK( CallBacker* )
{
    int selidx = -1;
    seltype_ = selType();
    if ( seltype_==1 )		selidx = attroutfld_->currentItem();
    else			selidx = storoutfld_->currentItem();

    if ( selidx < 0 )
	return false;

    if ( seltype_ == 1 )
	descid_ = attrinf_->attrids[selidx];
    else if ( seltype_ == 0 )
	storednm_ = storoutfld_->getText();

    return true;
}

