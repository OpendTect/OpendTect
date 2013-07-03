/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthexport.h"

#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "stratsynth.h"
#include "zdomain.h"
#include "survinfo.h"
#include "picksettr.h"
#include "randomlinetr.h"


class uiStratSynthOutSel : public uiCheckedCompoundParSel
{
public:

uiStratSynthOutSel( uiParent* p, const char* seltxt, const BufferStringSet& nms )
    : uiCheckedCompoundParSel( p, seltxt, false, "&Select" )
    , nms_(nms)
    , nm_(seltxt)
{
    for ( int idx=0; idx<nms_.size(); idx++ )
	selidxs_ += idx;
    butPush.notify( mCB(this,uiStratSynthOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( BufferString("Select ",nm_), mNoDlgTitle, mTODOHelpID );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, nm_ );
    lb->addItems( nms_ );
    lb->setItemsCheckable( true );
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setItemChecked( selidxs_[idx], true );
    if ( dlg.go() )
    {
	selidxs_.erase();
	for ( int idx=0; idx<lb->size(); idx++ )
	    if ( lb->isItemChecked(idx) ) selidxs_ += idx;
    }
}

virtual BufferString getSummary() const
{
    BufferString ret;
    const int sz = nms_.size();
    const int selsz = selidxs_.size();

    if ( sz < 1 )
	ret = "<None available>";
    else if ( selsz == 0 )
	ret = "<None selected>";
    else
    {
	if ( selsz > 1 )
	{
	    ret.add( "<" ).add( selsz ).add( " selected" );
	    if ( sz == selsz )
		ret.add( " (all)" );
	    ret.add( ">: " );
	}
	ret.add( nms_.get( selidxs_[0] ) );
	if ( selsz > 1 )
	    ret.add( ", ..." );
	else if ( sz == selsz )
	    ret.add( " (all)" );
    }
    
    return ret;
}

    const BufferString	nm_;
    const BufferStringSet nms_;
    TypeSet<int>	selidxs_;

    uiListBox*		listfld_;

};


uiStratSynthExport::uiStratSynthExport( uiParent* p, const StratSynth& ss )
    : uiDialog(p,uiDialog::Setup("Save synthetic seismics and horizons",
				 getWinTitle(ss), mTODOHelpID) )
    , ss_(ss)
    , randlinesel_(0)
{
    crnewfld_ = new uiGenInput( this, "2D Line",
			     BoolInpSpec(true,"Create New","Use existing") );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );


    uiSeisSel::Setup sssu( Seis::Line );
    sssu.enabotherdomain( false ).zdomkey( ZDomain::sKeyTime() )
	.selattr( false ).allowsetdefault( false );
    linesetsel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
	    			sssu );
    linesetsel_->attach( alignedBelow, crnewfld_ );
    linesetsel_->selectionDone.notify( mCB(this,uiStratSynthExport,lsSel) );
    newlinenmsel_ = new uiSeis2DLineNameSel( this, false );
    newlinenmsel_->attach( alignedBelow, linesetsel_ );
    existlinenmsel_ = new uiSeis2DLineNameSel( this, true );
    existlinenmsel_->attach( alignedBelow, linesetsel_ );

    geomgrp_ = new uiGroup( this, "Geometry group" );
    fillGeomGroup();
    geomgrp_->attach( alignedBelow, existlinenmsel_ );

    BufferStringSet nms;
    nms.add( "Post-stack 1" ); nms.add( "Post-stack 2" );
    poststcksel_ = new uiStratSynthOutSel( this, "Post-stack line data", nms );
    poststcksel_->attach( alignedBelow, geomgrp_ );
    nms.erase(); nms.add( "Level 1" ); nms.add( "Level 2" ).add( "Level 3" );
    horsel_ = new uiStratSynthOutSel( this, "2D horizons", nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    nms.erase(); nms.add( "Pre-stack 1" );
    prestcksel_ = new uiStratSynthOutSel( this, "Pre-stack data", nms );
    prestcksel_->attach( alignedBelow, horsel_ );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
}


BufferString uiStratSynthExport::getWinTitle( const StratSynth& ss ) const
{
    BufferString ret( "" );
    return ret;
}


void uiStratSynthExport::fillGeomGroup()
{
    StringListInpSpec inpspec;
    inpspec.addString( "Straight line" ); inpspec.addString( "Polygon" );
    const bool haverl = SI().has3D();
    if ( haverl )
	inpspec.addString( "Random Line" );
    geomsel_ = new uiGenInput( geomgrp_, "Geometry for line", inpspec );
    geomsel_->valuechanged.notify( mCB(this,uiStratSynthExport,geomSel) );
    geomgrp_->setHAlignObj( geomsel_ );

    coord0fld_ = new uiGenInput( geomgrp_, "Coordinates: from",
					DoubleInpSpec(), DoubleInpSpec() );
    coord0fld_->attach( alignedBelow, geomsel_ );
    coord1fld_ = new uiGenInput( geomgrp_, "to",
					DoubleInpSpec(), DoubleInpSpec() );
    coord1fld_->attach( alignedBelow, coord0fld_ );

    IOObjContext psctxt( mIOObjContext(PickSet) );
    psctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );
    picksetsel_ = new uiIOObjSel( geomgrp_, psctxt, "Polygon" );
    picksetsel_->attach( alignedBelow, geomsel_ );
    if ( haverl )
    {
	randlinesel_ = new uiIOObjSel( geomgrp_, mIOObjContext(RandomLineSet) );
	randlinesel_->attach( alignedBelow, geomsel_ );
    }
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    newlinenmsel_->display( iscreate );
    existlinenmsel_->display( !iscreate );
    geomgrp_->display( iscreate );
    if ( iscreate )
	geomSel( 0 );
}


void uiStratSynthExport::geomSel( CallBacker* )
{
    const int selgeom = crnewfld_->getBoolValue() ? geomsel_->getIntValue(): -1;
    coord0fld_->display( selgeom == 0 );
    coord1fld_->display( selgeom == 0 );
    picksetsel_->display( selgeom == 1 );
    if ( randlinesel_ )
	randlinesel_->display( selgeom == 2 );
}


void uiStratSynthExport::lsSel( CallBacker* )
{
    newlinenmsel_->setLineSet( linesetsel_->key() );
}


bool uiStratSynthExport::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: complete and implement" );
    return false;
}
