/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uisurvseissel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilistbox.h"
#include "uiioobjselgrp.h"
#include "uisurveyselect.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "keystrs.h"
#include "seisioobjinfo.h"


static IOObjContext& getCtxt( const uiSurvSeisSel::Setup& su )
{
    static IOObjContext* ret = 0;
    delete ret;
    ret = Seis::getIOObjContext( su.geomtype_, true );
    if ( su.steerflag_ != 0 )
    {
	IOPar& iop = su.steerflag_ < 0 ? ret->toselect_.dontallow_
				       : ret->toselect_.require_;
	iop.set( sKey::Type(), sKey::Steering() );
    }
    return *ret;
}


static uiString getLbl( const uiSurvSeisSel::Setup& su )
{
    if ( su.lbltxt_ == toUiString("-") )
	return uiString::empty();
    else if ( su.lbltxt_.isEmpty() )
	return Seis::dataName( su.geomtype_ );
    return su.lbltxt_;
}


uiSurvSeisSel::uiSurvSeisSel( uiParent* p, const Setup& su )
    : uiSurvIOObjSel(p,getCtxt(su),getLbl(su),su.fixedsurvey_)
    , setup_(su)
    , compSel(this)
{
    setup_.lbltxt_ = getLbl( su );
    compfld_ = new uiComboBox( this, "Components" );
    compfld_->setHSzPol( uiObject::SmallVar );
    if ( survselbut_ )
	compfld_->attach( rightOf, survselbut_ );
    else
	compfld_->attach( rightOf, objfld_ );

    mAttachCB( postFinalise(), uiSurvSeisSel::initSeisGrp );
}


uiSurvSeisSel::~uiSurvSeisSel()
{
    detachAllNotifiers();
}


void uiSurvSeisSel::initSeisGrp( CallBacker* )
{
    updateComps();
    mAttachCB( selChange, uiSurvSeisSel::inpSelChgCB );
    mAttachCB( compfld_->selectionChanged, uiSurvSeisSel::compSelCB );
}


void uiSurvSeisSel::inpSelChgCB( CallBacker* )
{
    updateComps();
}


void uiSurvSeisSel::compSelCB( CallBacker* )
{
    compSel.trigger();
}


void uiSurvSeisSel::setCompNr( int compnr )
{
    if ( !finalised() )
	updateComps();

    compfld_->setCurrentItem( compnr );
}


int uiSurvSeisSel::compNr() const
{
    const int selidx = compfld_->currentItem();
    return selidx < 0 ? 0 : selidx;
}


int uiSurvSeisSel::nrComps() const
{
    return compfld_->size();
}


const char* uiSurvSeisSel::compName( int idx ) const
{
    return compfld_->itemText( idx );
}


void uiSurvSeisSel::updateComps()
{
    compfld_->setEmpty();
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return;

    const SeisIOObjInfo ioobjinfo( *ioobj );
    if ( !ioobjinfo.isOK() )
	return;

    BufferStringSet compnms;
    ioobjinfo.getComponentNames( compnms );
    const int sz = compnms.size();
    compfld_->addItems( compnms );
    if ( sz > 0 )
	compfld_->setCurrentItem( 0 );
    compfld_->display( sz > 1 );
}


class uiSurvSeisSelGroupCompEntry
{
public:

uiSurvSeisSelGroupCompEntry( const IOObj& ioobj )
    : dbky_(ioobj.key())
{
    const SeisIOObjInfo ioobjinfo( ioobj );
    if ( ioobjinfo.isOK() )
	ioobjinfo.getComponentNames( nms_ );
}

BufferStringSet::idx_type size() const
{
    return nms_.size();
}

bool isSelected( int icomp ) const
{
    if ( selected_.isEmpty() )
	return size() < 2 && icomp == 0;
    return selected_.isPresent( icomp );
}

void setSelected( int icomp, bool yn )
{
    const bool issel = isSelected( icomp );
    if ( issel != yn )
    {
	if ( yn )
	    selected_ += icomp;
	else
	    selected_ -= icomp;
    }
}

    const DBKey		dbky_;
    BufferStringSet	nms_;
    TypeSet<int>	selected_;

};


uiSurvSeisSelGroupCompEntry& uiSurvSeisSelGroup::getCompEntry(
					    int objidx, bool selected ) const
{
    const IOObj& ioobj = selected ? *ioObj( objidx ) : *ioobjs_[objidx];;
    uiSurvSeisSelGroupCompEntry* ret = 0;
    for ( uiSurvSeisSelGroupCompEntry* entry : compentries_ )
	if ( entry->dbky_ == ioobj.key() )
	    { ret = entry; break; }
    if ( !ret )
    {
	ret = new uiSurvSeisSelGroupCompEntry( ioobj );
	compentries_ += ret;
    }
    return *ret;
}


uiSurvSeisSelGroup::uiSurvSeisSelGroup( uiParent* p, const Setup& su,
					bool ismulti, bool fixsurv,
					bool withinserters )
    : uiSurvIOObjSelGroup(p,getCtxt(su),ismulti,fixsurv,withinserters)
    , setup_(su)
{

    uiListBox::Setup lbsu( ismulti ? OD::ChooseAtLeastOne
				   : OD::ChooseOnlyOne );
    lbsu.prefnrlines( 4 ).lbl( uiStrings::sComponent() )
	.lblpos(uiListBox::AboveLeft);
    compfld_ = new uiListBox( this, lbsu );
    compfld_->setHSzPol( uiObject::SmallVar );
    compfld_->attach( rightOf, objfld_->attachObj() );

    mAttachCB( postFinalise(), uiSurvSeisSelGroup::initSeisGrp );
}


uiSurvSeisSelGroup::~uiSurvSeisSelGroup()
{
    detachAllNotifiers();
    deepErase( compentries_ );
}


void uiSurvSeisSelGroup::initSeisGrp( CallBacker* )
{
    seisSelChgCB( 0 );
    mAttachCB( selChange, uiSurvSeisSelGroup::seisSelChgCB );
}


void uiSurvSeisSelGroup::seisSelChgCB( CallBacker* )
{
    const int selidx = objfld_->getListField()->currentItem();
    if ( selidx >= 0 )
    {
	if ( prevselidx_ >= 0 )
	{
	    uiSurvSeisSelGroupCompEntry& preventry = getCompEntry( prevselidx_,
								   false );
	    TypeSet<int> chosenidxs;
	    compfld_->getChosen( chosenidxs );
	    for ( int icomp=0; icomp<preventry.size(); icomp++ )
		preventry.setSelected( icomp, chosenidxs.isPresent(icomp) );
	}
	const uiSurvSeisSelGroupCompEntry& compentry = getCompEntry( selidx,
								     false );
	const int sz = compentry.size();
	compfld_->setEmpty();
	compfld_->addItems( compentry.nms_ );
	for ( int icomp=0; icomp<sz; icomp++ )
	    compfld_->setChosen( icomp, compentry.isSelected(icomp) );
	compfld_->display( sz > 1 );
	prevselidx_ = selidx;
    }
}


void uiSurvSeisSelGroup::refresh()
{
    prevselidx_ = -1;
    updGrp( false );
}


void uiSurvSeisSelGroup::setSelected( const DBKey& dbky, int compnr, bool add )
{
    if ( survsel_ && survsel_->surveyDiskLocation()!=dbky.surveyDiskLocation() )
	setSurvey( dbky.surveyDiskLocation() );
    else if ( ioobjs_.isEmpty() )
	updateObjs();

    const int idxof = indexOf( dbky );
    if ( idxof < 0 )
	return;

    uiSurvIOObjSelGroup::setSelected( dbky, add );
    seisSelChgCB( 0 );

    uiSurvSeisSelGroupCompEntry& compentry = getCompEntry( idxof, false );
    if ( compnr < compentry.size() )
    {
	compentry.setSelected( compnr, true );
	compfld_->setChosen( compnr, true );
    }
}


bool uiSurvSeisSelGroup::evaluateInput()
{
    if ( !uiSurvIOObjSelGroup::evaluateInput() )
	return false;

    seisSelChgCB( 0 ); // record last user component selection actions
    return true;
}


int uiSurvSeisSelGroup::nrComps( int isel ) const
{
    if ( !chosennms_.validIdx(isel) )
	return 0;
    return getCompEntry( isel, true ).size();
}


bool uiSurvSeisSelGroup::isSelectedComp( int icomp, int isel ) const
{
    if ( !chosennms_.validIdx(isel) )
	return false;
    return getCompEntry( isel, true ).isSelected( icomp );
}


const char* uiSurvSeisSelGroup::compName( int icomp, int isel ) const
{
    if ( chosennms_.validIdx(isel) )
    {
	const auto& entry = getCompEntry( isel, true );
	if ( entry.nms_.validIdx(icomp) )
	    return entry.nms_.get( icomp ).str();
    }
    return toString(icomp);
}
