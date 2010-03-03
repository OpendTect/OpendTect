/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2010
________________________________________________________________________

-*/

#include "ranges.h"
#include "datainpspec.h"
#include "uiprobdenfunvarsel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"

uiPrDenFunVarSel::uiPrDenFunVarSel( uiParent* p,const DataColInfo& colinfos )
    : uiGroup( p )
    , colinfos_( colinfos )
    , attrSelChanged( this )
{
    uiLabeledComboBox* cbx =
	new uiLabeledComboBox( this, colinfos_.colnms_, "Select Attribute" );
    attrsel_ = cbx->box();
    attrsel_->selectionChanged.notify(
	    mCB(this,uiPrDenFunVarSel,attrChanged) );

    rangesel_ = new uiGenInput( this, "Select Range",
	    FloatInpIntervalSpec(true).setName("Step",2) );
    rangesel_->valuechanged.notify( mCB(this,uiPrDenFunVarSel,rangeChanged) );
    rangesel_->attach( rightTo, cbx );
    
    nrbinsel_ = new uiGenInput( this, "Nr of Bins", IntInpSpec() );
    nrbinsel_->setValue( 100 );
    nrbinsel_->valuechanged.notify( mCB(this,uiPrDenFunVarSel,nrBinChanged) );
    nrbinsel_->attach( rightTo, rangesel_ );

    attrChanged( 0 );
    nrBinChanged( 0 );
}


void uiPrDenFunVarSel::setAttrRange( const StepInterval<float>& range )
{
    rangesel_->setValue( range );
    FloatInpIntervalSpec floatinpspec( range );
    floatinpspec.setLimits( range );
    rangesel_->newSpec( floatinpspec, 0 );
}


int uiPrDenFunVarSel::selNrBins() const
{ return nrbinsel_->getIntValue(); }


int uiPrDenFunVarSel::selColID() const
{
    if ( colinfos_.colids_.validIdx(attrsel_->currentItem()) )
	return colinfos_.colids_[attrsel_->currentItem()];
    return -1;
}


StepInterval<float> uiPrDenFunVarSel::selColRange() const
{ return rangesel_->getFInterval(); }


BufferString uiPrDenFunVarSel::selColName() const
{
    if ( colinfos_.colnms_.validIdx(attrsel_->currentItem()) )
	return colinfos_.colnms_.get( attrsel_->currentItem() );
    return BufferString();
}


void uiPrDenFunVarSel::attrChanged( CallBacker* )
{ attrSelChanged.trigger(); }


void uiPrDenFunVarSel::nrBinChanged( CallBacker* )
{
    StepInterval<float> range = rangesel_->getFInterval();
    const int nrbins = nrbinsel_->getIntValue();
    const float step = range.width()/nrbins;
    range.step = step;
    range.stop = range.start + step*nrbins;
    rangesel_->setValue( range );
}

void uiPrDenFunVarSel::rangeChanged( CallBacker* )
{
    StepInterval<float> range = rangesel_->getFInterval();
    const int nrbins = nrbinsel_->getIntValue();
    range.step = range.width()/nrbins;
    rangesel_->setValue( range );
}
