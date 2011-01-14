/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayseqattrsetbuild.cc,v 1.2 2011-01-14 14:44:09 cvsbert Exp $";

#include "uistratlayseqattrsetbuild.h"
#include "uilayseqattribed.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uiioobjsel.h"

#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "strattransl.h"
#include "strmprov.h"
#include "ioobj.h"


uiStratLaySeqAttribSetBuild::uiStratLaySeqAttribSetBuild( uiParent* p,
					    const Strat::LayerModel& lm )
    : uiGroup(p,"LaySeq attrib set build group")
    , attrset_(*new Strat::LaySeqAttribSet)
    , reftree_(lm.refTree())
    , usrchg_(false)
    , ctio_(*mMkCtxtIOObj(StratLayerSequenceAttribSet))
{
    propfld_ = new uiListBox( this, "Available properties" );
    fillPropFld( lm );

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
		"Add attribute", mCB(this,uiStratLaySeqAttribSetBuild,addReq) );
    addbut->attach( centeredRightOf, propfld_ );

    attrfld_ = new uiListBox( this, "Defined attributes" );
    attrfld_->attach( rightTo, propfld_ );
    attrfld_->attach( ensureRightOf, addbut );
    attrfld_->doubleClicked.notify(
	    		mCB(this,uiStratLaySeqAttribSetBuild,edReq) );

    edbut_ = new uiToolButton( this, "edit.png",
	    "Edit attribute", mCB(this,uiStratLaySeqAttribSetBuild,edReq) );
    edbut_->attach( rightOf, attrfld_ );
    rmbut_ = new uiToolButton( this, "trashcan.png",
	    "Remove attribute", mCB(this,uiStratLaySeqAttribSetBuild,rmReq) );
    rmbut_->attach( alignedBelow, edbut_ );
    uiToolButton* openbut = new uiToolButton( this, "openset.png",
	    "Open stored attribute set",
	    mCB(this,uiStratLaySeqAttribSetBuild,openReq) );
    openbut->attach( alignedBelow, rmbut_ );
    savebut_ = new uiToolButton( this, "save.png",
	    "Save attribute set",
	    mCB(this,uiStratLaySeqAttribSetBuild,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    updButStates();
}


uiStratLaySeqAttribSetBuild::~uiStratLaySeqAttribSetBuild()
{
    delete ctio_.ioobj;
    delete &attrset_;
    delete &ctio_;
}


void uiStratLaySeqAttribSetBuild::fillPropFld( const Strat::LayerModel& lm )
{
    BufferStringSet dispnms; ObjectSet<const PropertyRef> prs;
    for ( int idx=0; idx<lm.propertyRefs().size(); idx++ )
    {
	const PropertyRef* pr = lm.propertyRefs()[idx];
	dispnms.add( pr->name() );
	prs += pr;
    }
    if ( dispnms.isEmpty() ) return;

    int* idxs = dispnms.getSortIndexes();
    dispnms.useIndexes( idxs );
    for ( int idx=0; idx<prs.size(); idx ++ )
	props_ += prs[ idxs[idx] ];
    delete [] idxs;

    propfld_->addItems( dispnms );
    propfld_->doubleClicked.notify(
				mCB(this,uiStratLaySeqAttribSetBuild,addReq));
    propfld_->setCurrentItem( 0 );
}


void uiStratLaySeqAttribSetBuild::fillAttrFld()
{
    BufferString oldcur;
    if ( !attrfld_->isEmpty() )
	oldcur = attrfld_->getText();

    attrfld_->setEmpty();
    for ( int idx=0; idx<attrset_.size(); idx++ )
	attrfld_->addItem( attrset_.attr(idx).name() );

    if ( !attrfld_->isEmpty() )
    {
	int selidx = attrfld_->indexOf( oldcur );
	if ( selidx < 0 ) selidx = 0;
	attrfld_->setCurrentItem( selidx );
    }

    updButStates();
}


void uiStratLaySeqAttribSetBuild::updButStates()
{
    const bool havesel = attrfld_->currentItem() >= 0;
    edbut_->setSensitive( havesel );
    savebut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
}


bool uiStratLaySeqAttribSetBuild::doAttrEd( Strat::LaySeqAttrib& lsa,
					    bool isnew )
{
    uiLaySeqAttribEd dlg( this, lsa, reftree_, isnew );
    if ( !dlg.go() )
	return false;

    if ( dlg.anyChange() )
    {
	fillAttrFld();
	usrchg_ = true;
	return true;
    }

    return false;
}


void uiStratLaySeqAttribSetBuild::addReq( CallBacker* )
{
    const int selidx = propfld_->currentItem();
    if ( selidx < 0 ) return;

    const PropertyRef* prop = props_[selidx];
    Strat::LaySeqAttrib* attr = new Strat::LaySeqAttrib( attrset_,
	    						 *props_[selidx] );
    attrset_ += attr;

    if ( !doAttrEd(*attr,false) )
	attrset_-= attr;
}


void uiStratLaySeqAttribSetBuild::edReq( CallBacker* )
{
    const int selidx = attrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = attrfld_->textOfItem( selidx );
    Strat::LaySeqAttrib* attr = attrset_.attr( attrnm );
    if ( !attr ) { pErrMsg("Huh"); return; }
    doAttrEd( *attr, false );
}


void uiStratLaySeqAttribSetBuild::rmReq( CallBacker* )
{
    const int selidx = attrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = attrfld_->textOfItem( selidx );
    Strat::LaySeqAttrib* attr = attrset_.attr( attrnm );
    if ( attr )
	attrset_ -= attr;
    usrchg_ = true;

    int newselidx = selidx;
    if ( newselidx >= attrfld_->size()-1 )
	newselidx--;
    if ( newselidx >= 0 )
	attrfld_->setCurrentItem( newselidx );
    fillAttrFld();
}


void uiStratLaySeqAttribSetBuild::openReq( CallBacker* )
{
    if ( usrchg_ && !uiMSG().askGoOn("Current work not saved. Continue?") )
	return;

    if ( doSetIO(true) )
	fillAttrFld();
}


void uiStratLaySeqAttribSetBuild::saveReq( CallBacker* )
{
    doSetIO( false );
}


bool uiStratLaySeqAttribSetBuild::doSetIO( bool forread )
{
    ctio_.ctxt.forread = forread;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    ctio_.setObj( dlg.ioObj()->clone() );

    StreamProvider sp( ctio_.ioobj->fullUserExpr(false) );
    StreamData sd( forread ? sp.makeIStream() : sp.makeOStream() );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( "Cannot open ", forread ? "in" : "out", "put file" );
    else
    {
	rv = forread ? attrset_.getFrom(*sd.istrm) : attrset_.putTo(*sd.ostrm);
	if ( !rv )
	    uiMSG().error( "Error during ",
		    forread ? "read from input " : "write to output", " file" );
    }

    if ( rv )
	usrchg_ = false;

    return rv;
}
