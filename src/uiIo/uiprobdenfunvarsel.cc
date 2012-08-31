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
#include <math.h>

uiPrDenFunVarSel::uiPrDenFunVarSel( uiParent* p,const DataColInfo& colinfos )
    : uiGroup( p )
    , colinfos_( colinfos )
    , attrSelChanged( this )
{
    uiLabeledComboBox* cbx =
	new uiLabeledComboBox( this, colinfos_.colnms_, "Attribute" );
    attrsel_ = cbx->box();
    attrsel_->selectionChanged.notify(
	    mCB(this,uiPrDenFunVarSel,attrChanged) );

    rangesel_ = new uiGenInput( this, "Range", FloatInpIntervalSpec() );
    rangesel_->valuechanged.notify( mCB(this,uiPrDenFunVarSel,rangeChanged) );
    rangesel_->attach( rightTo, cbx );
    
    nrbinsel_ = new uiGenInput( this, "Nr of Bins", IntInpSpec() );
    nrbinsel_->setElemSzPol( uiObject::Small );
    nrbinsel_->setValue( 25 );
    nrbinsel_->valuechanged.notify( mCB(this,uiPrDenFunVarSel,nrBinChanged) );
    nrbinsel_->attach( rightTo, rangesel_ );

    setHAlignObj( attrsel_ );

    attrChanged( 0 );
    nrBinChanged( 0 );
}


int uiPrDenFunVarSel::nrCols() const
{ return attrsel_->size(); }


void uiPrDenFunVarSel::setColNr( int nr )
{
    attrsel_->setCurrentItem( nr );
    attrChanged( 0 );
}


const char* uiPrDenFunVarSel::colName( int idx ) const
{ return attrsel_->textOfItem( idx ); }


void uiPrDenFunVarSel::setPrefCol( const char* nm )
{
    BufferStringSet attrnms;
    for ( int idx=0; idx<attrsel_->size(); idx++ )
	attrnms.add( attrsel_->textOfItem(idx) );
    const int prefidx = attrnms.nearestMatch( nm );
    if ( attrnms.validIdx(prefidx) )
    {
	attrsel_->setCurrentItem( prefidx );
	attrChanged( 0 );
    }
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
{
    StepInterval<float> range = rangesel_->getFInterval();
    const float step = fabs( range.stop - range.start ) /
		       nrbinsel_->getIntValue();
    range.step = step;
    return range;
}


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
