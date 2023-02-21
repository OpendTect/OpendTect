/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprobdenfunvarsel.h"

#include "ranges.h"
#include "datainpspec.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrings.h"
#include <math.h>


uiPrDenFunVarSel::DataColInfo::DataColInfo(
				const BufferStringSet& colnames,
				const TypeSet<int>& colids)
    : colnms_(colnames)
    , colids_(colids)
{}


uiPrDenFunVarSel::DataColInfo::~DataColInfo()
{}



uiPrDenFunVarSel::uiPrDenFunVarSel( uiParent* p, const DataColInfo& colinfos )
    : uiGroup( p )
    , attrSelChanged(this)
    , colinfos_(colinfos)
{
    auto* cbx = new uiLabeledComboBox( this, colinfos_.colnms_,
				       uiStrings::sAttribute() );
    attrsel_ = cbx->box();
    attrsel_->selectionChanged.notify(
	    mCB(this,uiPrDenFunVarSel,attrChanged) );

    createGUI( cbx->attachObj() );
}


uiPrDenFunVarSel::uiPrDenFunVarSel( uiParent* p, const DataColInfo& colinfos,
				    const uiString& lbl )
    : uiGroup( p )
    , attrSelChanged(this)
    , colinfos_(colinfos)
{
    auto* cbx = new uiLabeledComboBox( this, colinfos_.colnms_, lbl );
    attrsel_ = cbx->box();
    attrsel_->selectionChanged.notify(
	    mCB(this,uiPrDenFunVarSel,attrChanged) );

    createGUI( cbx->attachObj() );
}


uiPrDenFunVarSel::~uiPrDenFunVarSel()
{}


void uiPrDenFunVarSel::createGUI( uiObject* attachobj )
{
    rangesel_ = new uiGenInput( this, tr("Range"), FloatInpIntervalSpec() );
    rangesel_->valueChanged.notify( mCB(this,uiPrDenFunVarSel,rangeChanged) );
    rangesel_->attach( rightTo, attachobj );

    nrbinsel_ = new uiGenInput( this, tr("Nr of Bins"), IntInpSpec() );
    nrbinsel_->setElemSzPol( uiObject::Small );
    nrbinsel_->setValue( 25 );
    nrbinsel_->valueChanged.notify( mCB(this,uiPrDenFunVarSel,nrBinChanged) );
    nrbinsel_->attach( rightTo, rangesel_ );

    setHAlignObj( attrsel_ );

    attrChanged( nullptr );
    nrBinChanged( nullptr );
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
