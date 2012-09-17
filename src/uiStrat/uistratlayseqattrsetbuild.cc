/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayseqattrsetbuild.cc,v 1.11 2012/04/10 14:29:40 cvsbert Exp $";

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
				    const Strat::LayerModel& lm,
				    uiStratLaySeqAttribSetBuild::SetTypeSel sts,
				    Strat::LaySeqAttribSet* as )
    : uiBuildListFromList(p,
		  uiBuildListFromList::Setup(false,"property","attribute"),
		  "Layer Sequence Attrib Set build group")
    , attrset_(as ? *as : *new Strat::LaySeqAttribSet)
    , reftree_(lm.refTree())
    , ctio_(*mMkCtxtIOObj(StratLayerSequenceAttribSet))
    , typesel_(sts)
    , setismine_(!as)
{
    BufferStringSet dispnms;
    for ( int idx=0; idx<lm.propertyRefs().size(); idx++ )
    {
	const PropertyRef* pr = lm.propertyRefs()[idx];
	dispnms.add( pr->name() );
	props_ += pr;
    }

    dispnms.sort();
    setAvailable( dispnms );
    if ( !attrset_.isEmpty() )
    {
	for ( int idx=0; idx<attrset_.size(); idx++ )
	    deffld_->addItem( attrset_.attr(idx).name() );
	deffld_->setCurrentItem( 0 );
	defSelChg();
    }
}


uiStratLaySeqAttribSetBuild::~uiStratLaySeqAttribSetBuild()
{
    delete ctio_.ioobj;
    if ( setismine_ )
	delete &attrset_;
    delete &ctio_;
}


const char* uiStratLaySeqAttribSetBuild::avFromDef( const char* attrnm ) const
{
    const Strat::LaySeqAttrib* attr = attrset_.attr( attrnm );
    if ( !attr ) return 0;
    return attr->prop_.name();
}


void uiStratLaySeqAttribSetBuild::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm ) return;

    Strat::LaySeqAttrib* attr = 0;
    if ( !isadd )
	attr = attrset_.attr( nm );
    else
    {
	const PropertyRef* prop = props_.get( nm );
	if ( !prop ) return;
	attr = new Strat::LaySeqAttrib( attrset_, *prop );
	attrset_ += attr;
    }
    if ( !attr ) { pErrMsg("Huh"); return; }

    uiLaySeqAttribEd::Setup su( isadd );
    su.allowlocal( typesel_ != OnlyIntegrated );
    su.allowintegr( typesel_ != OnlyLocal );
    uiLaySeqAttribEd dlg( this, *attr, reftree_, su );
    if ( dlg.go() )
	handleSuccessfullEdit( isadd, attr->name() );
    else if ( isadd )
	attrset_-= attr;
}


void uiStratLaySeqAttribSetBuild::removeReq()
{
    const char* attrnm = curDefSel();
    if ( attrnm && *attrnm )
    {
	Strat::LaySeqAttrib* attr = attrset_.attr( attrnm );
	if ( attr )
	{
	    attrset_ -= attr;
	    removeItem();
	}
    }
}


bool uiStratLaySeqAttribSetBuild::ioReq( bool forsave )
{
    ctio_.ctxt.forread = !forsave;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    ctio_.setObj( dlg.ioObj()->clone() );

    StreamProvider sp( ctio_.ioobj->fullUserExpr(false) );
    StreamData sd( forsave ? sp.makeOStream() : sp.makeIStream() );
    bool rv = false;
    if ( !sd.usable() )
	uiMSG().error( "Cannot open ", forsave ? "out" : "in", "put file" );
    else
    {
	rv = forsave ? attrset_.putTo(*sd.ostrm) : attrset_.getFrom(*sd.istrm);
	if ( !rv )
	    uiMSG().error( "Error during ",
		    forsave ? "write to output" : "read from input ", " file" );
    }

    if ( rv )
    {
	usrchg_ = false;
	if ( !forsave )
	{
	    removeAll();
	    if ( typesel_ != AllTypes )
	    {
		const bool needlocal = typesel_ == OnlyLocal;
		for ( int idx=0; idx<attrset_.size(); idx++ )
		{
		    if ( attrset_[idx]->islocal_ != needlocal )
			{ attrset_.remove( idx ); idx--; }
		}
	    }
	    for ( int idx=0; idx<attrset_.size(); idx++ )
		addItem( attrset_.attr(idx).name() );
	}
    }

    return rv;
}
