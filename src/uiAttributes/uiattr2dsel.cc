/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Nov 2007
 RCS:           $Id: uiattr2dsel.cc,v 1.3 2007-12-10 05:25:22 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiattr2dsel.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "colortab.h"
#include "hilbertattrib.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "pixmap.h"
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
	, curnm_(curnm)
	, seltype_(0)
	, usecoltab_(false)
	, selgrp_(0)
	, storoutfld_(0)
	, attroutfld_(0)
	, coltabsel_(0)
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
    createColorFields();
    finaliseStart.notify( mCB( this,uiAttr2DSelDlg,doFinalise) );
}


uiAttr2DSelDlg::~uiAttr2DSelDlg()
{
    delete selgrp_;
    delete attrinf_;
}


static int sPixmapWidth = 16;
static int sPixmapHeight = 10;

void uiAttr2DSelDlg::createColorFields()
{
    coltabfld_ = new uiCheckBox( this, "Set Color Table for this attribute");
    coltabfld_->activated.notify( mCB(this,uiAttr2DSelDlg,colTabSel) );
    coltabfld_->attach( alignedBelow, storoutfld_ );
    if ( attroutfld_ ) coltabfld_->attach( ensureBelow, attroutfld_ );

    NamedBufferStringSet ctabs( "Color table" );
    ColorTable::getNames( ctabs );
    ctabs.sort();
    coltabsel_ = new uiComboBox( this, ctabs, "Select Color Table" );
    const int nrcoltabs = ctabs.size();
    for ( int idx=0; idx<nrcoltabs; idx++ )
    {
	const char* ctname = ctabs.get( idx );
	ioPixmap pm( ctname, sPixmapWidth, sPixmapHeight );
	coltabsel_->setPixmap( pm, idx );
    }

    coltabsel_->attach( rightTo, coltabfld_ );
}



void uiAttr2DSelDlg::doFinalise( CallBacker* )
{
    selDone(0);
    colTabSel(0);
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
    const int seltyp = selType();
    if ( attroutfld_ ) attroutfld_->display( seltyp == 1 );
    if ( storoutfld_ )
	storoutfld_->display( seltyp == 0 );
}


void uiAttr2DSelDlg::colTabSel( CallBacker* c )
{
    usecoltab_ = coltabfld_->isChecked();
    coltabsel_->setSensitive( usecoltab_ );
}


bool uiAttr2DSelDlg::acceptOK( CallBacker* )
{
    int selidx = -1;
    if ( seltype_ == selType() )
    {
	BufferString selnm = seltype_ ? attroutfld_->getText()
	    			      : storoutfld_->getText();
	if ( selnm==curnm_ )
	{
	    BufferString msg = "Do you want to reload the attribute ";
	    msg += selnm;
	    if ( !uiMSG().askGoOn(msg) ) seltype_ = -1;
	}
    }

    seltype_ = selType();
    if ( seltype_==1 )		selidx = attroutfld_->currentItem();
    else if ( seltype_==0 )	selidx = storoutfld_->currentItem();

    if ( seltype_>=0 && selidx < 0 )
	return false;

    if ( seltype_ == 1 )
	descid_ = attrinf_->attrids[selidx];
    else if ( seltype_ == 0 )
	storednm_ = storoutfld_->getText();

    if ( usecoltab_ ) coltabnm_ = coltabsel_->text();
    return true;
}

