/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattr2dsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "hilbertattrib.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"

using namespace Attrib;


uiAttr2DSelDlg::uiAttr2DSelDlg( uiParent* p, const DescSet* ds,
				const TypeSet<Pos::GeomID>& geomids,
				const NLAModel* nla,
				const char* curnm )
    : uiDialog(p,Setup( uiStrings::phrSelect( tr("Dataset") ),
			mNoDlgTitle,mNoHelpKey))
    , geomids_(geomids)
    , descid_(-1,true)
    , nla_(nla)
    , curnm_(curnm)
{
    attrinf_ = new SelInfo( ds, nla_, true );

    createSelectionButtons();
    createSelectionFields();

    if ( curnm && *curnm )
    {
	if ( storoutfld_ && storoutfld_->isPresent(curnm) )
	    storoutfld_->setCurrentItem( curnm );
	if ( steeroutfld_ && steeroutfld_->isPresent(curnm) )
	    steeroutfld_->setCurrentItem( curnm );

	else if ( attroutfld_ && attroutfld_->isPresent(curnm) )
	{
	    seltype_ = 1;
	    attroutfld_->setCurrentItem( curnm );
	}
    }

    selgrp_->selectButton( seltype_ );
    if ( ds )
	initFields( *ds );
    preFinalize().notify( mCB( this,uiAttr2DSelDlg,doFinalize) );
}


uiAttr2DSelDlg::uiAttr2DSelDlg( uiParent* p, const DescSet* ds,
				const TypeSet<Pos::GeomID>& geomids,
				const NLAModel* nla, ZDomain::Info& info,
				const char* curnm )
    : uiDialog(p,Setup( uiStrings::phrSelect( tr("Dataset") ),
			mNoDlgTitle,mNoHelpKey))
    , geomids_(geomids)
    , descid_(-1,true)
    , nla_(nla)
    , curnm_(curnm)
{
    attrinf_ = new SelInfo( ds, nla_, true );

    createSelectionButtons( info );
    createSelectionFields( info );

    if ( curnm && *curnm )
    {
	if ( storoutfld_ && storoutfld_->isPresent(curnm) )
	    storoutfld_->setCurrentItem( curnm );
	if ( steeroutfld_ && steeroutfld_->isPresent(curnm) )
	    steeroutfld_->setCurrentItem( curnm );

	else if ( attroutfld_ && attroutfld_->isPresent(curnm) )
	{
	    seltype_ = 1;
	    attroutfld_->setCurrentItem( curnm );
	}
    }

    selgrp_->selectButton( seltype_ );
    if ( ds )
	initFields( *ds );
    preFinalize().notify( mCB( this,uiAttr2DSelDlg,doFinalize) );
}


uiAttr2DSelDlg::~uiAttr2DSelDlg()
{
    delete attrfilter_;
    delete attrinf_;
}


void uiAttr2DSelDlg::doFinalize( CallBacker* )
{
    selDone( nullptr );
}


void uiAttr2DSelDlg::initFields( const Attrib::DescSet& ds )
{
    int seltyp = 0, curitm = -1;
    if ( nla_ )
    {
	seltyp = 3;
	curitm = 0;
    }
    else
    {
	for ( int idx=ds.size()-1; idx!=-1; idx-- )
	{
	    const DescID attrid = ds.getID( idx );
	    const Desc* ad = ds.getDesc( attrid );
	    if ( !ad || ad->isStored() )
		continue;

	    curitm = attrinf_->attrnms_.indexOf( ad->userRef() );
	    seltyp = 2;
	    if ( curitm!=-1 )
		break;
	}
    }

    if ( (seltyp==2 ||seltyp==3) && curitm!=-1 )
    {
	selgrp_->selectButton( seltyp );
	if ( seltyp==2 && attroutfld_ )
	    attroutfld_->setCurrentItem( curitm );
	else if ( nlaoutfld_ )
	    nlaoutfld_->setCurrentItem( curitm );
    }
}


static void getDataNames( const TypeSet<Pos::GeomID> ids,
			  SeisIOObjInfo::Opts2D o2d,
			  BufferStringSet& names )
{
    names.erase();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	BufferStringSet nms;
	SeisIOObjInfo::getDataSetNamesForLine( ids[idx], nms, o2d );
	for ( int nmidx=0; nmidx<nms.size(); nmidx++ )
	    names.addIfNew( nms.get(nmidx) );
    }

    names.sort();
}


void uiAttr2DSelDlg::createSelectionButtons()
{
    ZDomain::Info info( ZDomain::SI() );
    createSelectionButtons( info );
}


void uiAttr2DSelDlg::createSelectionFields()
{
    ZDomain::Info info( ZDomain::SI() );
    createSelectionFields( info );
}


void uiAttr2DSelDlg::createSelectionButtons( ZDomain::Info& info )
{
    selgrp_ = new uiButtonGroup( this, "Input selection", OD::Vertical );

    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = 0;
    BufferStringSet nms;
    o2d.zdomky_ = info.key();
    getDataNames( geomids_, o2d, nms );

    storfld_ = new uiRadioButton( selgrp_, uiStrings::sStored() );
    storfld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );
    storfld_->setSensitive( nms.size() );

    o2d.steerpol_ = 1;
    nms.erase();
    getDataNames( geomids_, o2d, nms );
    const bool havesteer = !nms.isEmpty();
    if ( havesteer )
    {
	steerfld_ = new uiRadioButton( selgrp_, uiStrings::sSteering() );
	steerfld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );
    }

    const bool haveattribs = attrinf_->attrnms_.size();
    attrfld_ = new uiRadioButton( selgrp_, uiStrings::sAttribute(mPlural) );
    attrfld_->setSensitive( haveattribs );
    attrfld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );

    if ( !nla_ ) return;

    nlafld_ = new uiRadioButton( selgrp_, toUiString(nla_->nlaType(false)) );
    nlafld_->setSensitive( attrinf_->nlaoutnms_.size() );
    nlafld_->activated.notify( mCB(this,uiAttr2DSelDlg,selDone) );
}


void uiAttr2DSelDlg::createSelectionFields( ZDomain::Info& info )
{
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = 0;
    o2d.zdomky_ = info.key();
    BufferStringSet nms;
    getDataNames( geomids_, o2d, nms );

    storoutfld_ = new uiListBox( this, "Stored cubes" );
    storoutfld_->addItems( nms );
    storoutfld_->setHSzPol( uiObject::Wide );
    storoutfld_->setCurrentItem( 0 );
    storoutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
    storoutfld_->attach( rightOf, selgrp_ );

    storsteerfilter_ = new uiGenInput( this, uiStrings::sFilter(), "*" );
    storsteerfilter_->attach( centeredAbove, storoutfld_ );
    storsteerfilter_->valuechanged.notify( mCB(this,uiAttr2DSelDlg,filtChg) );

    o2d.steerpol_ = 1;
    nms.erase();
    getDataNames( geomids_, o2d, nms );
    const bool havesteer = !nms.isEmpty();
    if ( havesteer )
    {
	nms.sort();
	steeroutfld_ = new uiListBox( this, "Steering" );
	steeroutfld_->addItems( nms );
	steeroutfld_->setHSzPol( uiObject::Wide );
	steeroutfld_->setCurrentItem( 0 );
	steeroutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
	steeroutfld_->attach( rightOf, selgrp_ );
    }

    const bool haveattribs = !attrinf_->attrnms_.isEmpty();
    if ( haveattribs )
    {
	attroutfld_ = new uiListBox( this, "Attributes" );
	attroutfld_->addItems( attrinf_->attrnms_ );
	attroutfld_->setHSzPol( uiObject::Wide );
	attroutfld_->setCurrentItem( 0 );
	attroutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
	attroutfld_->attach( ensureRightOf, selgrp_ );

	attrfilter_ = new uiListBoxFilter( *attroutfld_ );
	attrfilter_->setItems( attrinf_->attrnms_ );
    }

    if ( !attrinf_->nlaoutnms_.isEmpty() )
    {
	nlaoutfld_ = new uiListBox( this, "NLAs" );
	nlaoutfld_->addItems( attrinf_->nlaoutnms_ );
	nlaoutfld_->setHSzPol( uiObject::Wide );
	nlaoutfld_->setCurrentItem( 0 );
	nlaoutfld_->doubleClicked.notify( mCB(this,uiAttr2DSelDlg,accept) );
	nlaoutfld_->attach( rightOf, selgrp_ );
    }
}


int uiAttr2DSelDlg::selType() const
{
    if ( storfld_->isChecked() )
	return 0;
    if ( steerfld_ && steerfld_->isChecked() )
	return 1;
    if ( attrfld_->isChecked() )
	return 2;
    return 3;
}


void uiAttr2DSelDlg::selDone( CallBacker* )
{
    const int seltyp = selType();
    if ( storoutfld_ ) storoutfld_->display( seltyp == 0 );
    if ( steeroutfld_ ) steeroutfld_->display( seltyp == 1 );
    if ( attroutfld_ ) attroutfld_->display( seltyp == 2 );
    if ( nlaoutfld_ ) nlaoutfld_->display( seltyp == 3 );
    storsteerfilter_->display( seltyp==0 || seltyp==1 );
}


void uiAttr2DSelDlg::filtChg( CallBacker* )
{
    if ( !storoutfld_ )
	return;

    const bool issteersel = selType() == 1;
    uiListBox* outfld = issteersel ? steeroutfld_ : storoutfld_;
    BufferStringSet& nms = issteersel ? attrinf_->steernms_
				      : attrinf_->ioobjnms_;
    outfld->setEmpty();
    attrinf_->fillStored( issteersel, storsteerfilter_->text() );
    if ( nms.isEmpty() )
	return;

    outfld->addItems( nms );
    outfld->setCurrentItem( 0 );
}


bool uiAttr2DSelDlg::acceptOK( CallBacker* )
{
    int selidx = -1;
    const int newseltype = selType();
    uiListBox* attrfld = nullptr;
    if ( newseltype==1 )	attrfld = steeroutfld_;
    else if ( newseltype==2 )	attrfld = attroutfld_;
    else if ( newseltype==3 )	attrfld = nlaoutfld_;
    else			attrfld = storoutfld_;

    if ( seltype_ == newseltype )
    {
	BufferString selnm = attrfld->getText();
	if ( selnm==curnm_ )
	{
	    uiString msg = tr("Do you want to reload the attribute %1?")
			   .arg(selnm);
	    if ( !uiMSG().askGoOn(msg) )
	    {
		seltype_ = -1;
		return false;
	    }
	}
    }

    seltype_ = newseltype;
    selidx = attrfld->currentItem();

    if ( selidx < 0 )
	return false;

    if ( seltype_ == 0 )
	storednm_ = storoutfld_->getText();
    else if ( seltype_ == 1 )
    {
	storednm_ = steeroutfld_->getText();
	compnr_ = 1;
    }
    else if ( seltype_ == 2 )
    {
	selidx = attrfilter_->getCurrent();
	descid_ = attrinf_->attrids_[selidx];
    }
    else if ( seltype_ == 3 )
    {
	descid_ = DescID::undef();
	outputnr_ = selidx;
    }

    return true;
}
