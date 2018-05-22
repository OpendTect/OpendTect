/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/

#include "uistratlayseqattrsetbuild.h"
#include "uilayseqattribed.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uiioobjseldlg.h"
#include "uistrings.h"

#include "stratreftree.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "strattransl.h"
#include "od_iostream.h"
#include "ioobj.h"


uiStratLaySeqAttribSetBuild::uiStratLaySeqAttribSetBuild( uiParent* p,
				    const Strat::LayerModel& lm,
				    uiStratLaySeqAttribSetBuild::SetTypeSel sts,
				    Strat::LaySeqAttribSet* as )
    : uiBuildListFromList(p, uiBuildListFromList::Setup(false,
	uiStrings::sProperty().toLower(), uiStrings::sAttribute().toLower()),
			    "Layer Sequence Attrib Set build group")
    , attrset_(as ? *as : *new Strat::LaySeqAttribSet)
    , reftree_(lm.refTree())
    , ctio_(*mMkCtxtIOObj(StratLayerSequenceAttribSet))
    , typesel_(sts)
    , setismine_(!as)
    , anychg_(false)
{
    uiStringSet dispnms;
    for ( int idx=0; idx<lm.propertyRefs().size(); idx++ )
    {
	const PropertyRef* pr = lm.propertyRefs()[idx];
	dispnms.add( toUiString(pr->name()) );
	props_ += pr;
    }

    dispnms.sort();
    setAvailable( dispnms );
    if ( !attrset_.isEmpty() )
    {
	for ( int idx=0; idx<attrset_.size(); idx++ )
	    deffld_->addItem( toUiString(attrset_.attr(idx).name()) );
	deffld_->setCurrentItem( 0 );
	defSelChg();
    }
}


uiStratLaySeqAttribSetBuild::~uiStratLaySeqAttribSetBuild()
{
    delete ctio_.ioobj_;
    if ( setismine_ )
	delete &attrset_;
    delete &ctio_;
}


bool uiStratLaySeqAttribSetBuild::handleUnsaved()
{
    if ( !anychg_ && !usrchg_ ) return true;

    const int res = uiMSG().question(tr("Well Attribute Set not saved.\n\n"
	                                "Do you want to save it now?"),
                                     tr("Yes (store)"), tr("No (discard)"),
                                        uiStrings::sCancel() );

    return res < 1 ? res == 0 : ioReq( true );
}


uiString uiStratLaySeqAttribSetBuild::avFromDef( const char* attrnm ) const
{
    const Strat::LaySeqAttrib* attr = attrset_.attr( attrnm );
    if ( !attr )
	return uiString::empty();
    return toUiString( attr->prop_.name() );
}


void uiStratLaySeqAttribSetBuild::editReq( bool isadd )
{
    uiString propnm; const char* attrnm = 0;
    if ( isadd )
    {
	const uiString* curavsel = curAvSel();
	if ( !curavsel || curavsel->isEmpty() )
	    return;
	propnm = *curavsel;
    }
    else
    {
	attrnm = curDefSel();
	if ( !attrnm || !*attrnm )
	    return;
    }

    Strat::LaySeqAttrib* attr = 0;
    if ( !isadd )
	attr = attrset_.attr( attrnm );
    else
    {
	const PropertyRef* prop = props_.getByName( toString(propnm) );
	if ( !prop )
	    { pErrMsg("Huh"); return; }
	attr = new Strat::LaySeqAttrib( attrset_, *prop );
	for ( int idx=0; idx<reftree_.lithologies().size(); idx++ )
	    attr->liths_.add( reftree_.lithologies().getLith(idx).name() );
	attrset_ += attr;
    }
    if ( !attr )
	{ pErrMsg("Huh"); return; }

    uiLaySeqAttribEd::Setup su( isadd );
    su.allowlocal( typesel_ != OnlyIntegrated );
    su.allowintegr( typesel_ != OnlyLocal );
    uiLaySeqAttribEd dlg( this, *attr, reftree_, su );
    if ( dlg.go() )
    {
	anychg_ = true;
	handleSuccessfullEdit( isadd, attr->name() );
    }
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
	    anychg_ = true;
	    attrset_ -= attr;
	    removeItem();
	}
    }
}


bool uiStratLaySeqAttribSetBuild::ioReq( bool forsave )
{
    ctio_.ctxt_.forread_ = !forsave;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;
    ctio_.setObj( dlg.ioObj()->clone() );

    const BufferString fnm( ctio_.ioobj_->mainFileName() );
    bool rv = false;
    uiUserShowWait usw( this, forsave ? uiStrings::sSavingData()
				      : uiStrings::sReadingData() );
    if ( forsave )
    {
	od_ostream strm( fnm );
	if ( !strm.isOK() )
	    uiMSG().error( uiStrings::phrCannotOpenOutpFile() );
	else
	    rv = attrset_.putTo( strm );
    }
    else
    {
	od_istream strm( fnm );
	if ( !strm.isOK() )
	    uiMSG().error( uiStrings::phrCannotOpenInpFile() );
	else
	    rv = attrset_.getFrom( strm );
    }
    usw.readyNow();

    if ( !rv )
	{ uiMSG().error(uiStrings::phrErrDuringIO(!forsave)); return false; }

    if ( !forsave )
    {
	removeAll();
	if ( typesel_ != AllTypes )
	{
	    const bool needlocal = typesel_ == OnlyLocal;
	    for ( int idx=0; idx<attrset_.size(); idx++ )
	    {
		if ( attrset_[idx]->islocal_ != needlocal )
		    { attrset_.removeSingle( idx ); idx--; }
	    }
	}
	uiUserShowWait usw2( this, uiStrings::sUpdatingDisplay() );
	for ( int idx=0; idx<attrset_.size(); idx++ )
	    addItem( attrset_.attr(idx).name() );
    }

    usrchg_ = anychg_ = false;
    return true;
}
