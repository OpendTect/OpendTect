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
#include "uiunitsel.h"

#include <math.h>


// uiPrDenFunVarSel::DataColInfo

uiPrDenFunVarSel::DataColInfo::DataColInfo(
				const BufferStringSet& colnames,
				const TypeSet<int>& colids,
				const MnemonicSelection& mns,
				const ObjectSet<const UnitOfMeasure>& uoms )
    : colnms_(colnames)
    , colids_(colids)
    , mns_(*new MnemonicSelection(mns))
    , uoms_(uoms)
{}


uiPrDenFunVarSel::DataColInfo::DataColInfo( const DataColInfo& oth )
    : mns_(*new MnemonicSelection)
{
    *this = oth;
}


uiPrDenFunVarSel::DataColInfo::~DataColInfo()
{
    delete &mns_;
}


uiPrDenFunVarSel::DataColInfo&
uiPrDenFunVarSel::DataColInfo::operator=( const DataColInfo& oth )
{
    if ( &oth != this )
    {
	colnms_ = oth.colnms_;
	colids_ = oth.colids_;
	mns_ = oth.mns_;
	uoms_ = oth.uoms_;
    }

    return *this;
}


// uiPrDenFunVarSel

uiPrDenFunVarSel::uiPrDenFunVarSel( uiParent* p, const DataColInfo& colinfos,
				    const uiString& lbl, bool withunits )
    : uiGroup( p )
    , attrSelChanged(this)
    , colinfos_(colinfos)
{
    auto* cbx = new uiLabeledComboBox( this, colinfos_.colnms_, lbl );
    attrsel_ = cbx->box();
    mAttachCB( attrsel_->selectionChanged, uiPrDenFunVarSel::attrChanged );

    rangesel_ = new uiGenInput( this, tr("Range"), FloatInpIntervalSpec() );
    mAttachCB( rangesel_->valueChanged, uiPrDenFunVarSel::rangeChanged );
    rangesel_->attach( rightTo, cbx->attachObj() );

    nrbinsel_ = new uiGenInput( this, tr("Nr of Bins"), IntInpSpec() );
    nrbinsel_->setElemSzPol( uiObject::Small );
    nrbinsel_->setValue( 25 );
    mAttachCB( nrbinsel_->valueChanged, uiPrDenFunVarSel::nrBinChanged );
    nrbinsel_->attach( rightTo, rangesel_ );

    setHAlignObj( attrsel_ );

    if ( withunits )
    {
	uiUnitSel::Setup uiuss( Mnemonic::Other );
	uiuss.mode( uiUnitSel::Setup::SymbolsOnly ).withnone( true );
	unitfld_ = new uiUnitSel( this, uiuss );
	unitfld_->attach( rightTo, nrbinsel_ );
    }

    mAttachCB( postFinalize(), uiPrDenFunVarSel::initGrp );
}


uiPrDenFunVarSel::~uiPrDenFunVarSel()
{
    detachAllNotifiers();
}


void uiPrDenFunVarSel::initGrp( CallBacker* )
{
    attrChanged( nullptr );
    nrBinChanged( nullptr );
}


int uiPrDenFunVarSel::nrCols() const
{
    return attrsel_->size();
}


bool uiPrDenFunVarSel::hasAttrib( const char* nm ) const
{
    return attrsel_->isPresent( nm );
}


const UnitOfMeasure* uiPrDenFunVarSel::getUnit() const
{
    return unitfld_ ? unitfld_->getUnit() : nullptr;
}


void uiPrDenFunVarSel::setColNr( int nr )
{
    if ( nr>=0 && nr<attrsel_->size() )
    {
	attrsel_->setCurrentItem( nr );
	attrChanged( nullptr );
    }
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
	attrChanged( nullptr );
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
{
    if ( unitfld_ )
    {
	const int idx = attrsel_->currentItem();
	const Mnemonic* mn = colinfos_.mns_.validIdx( idx )
			   ? colinfos_.mns_.get( idx ) : nullptr;
	const UnitOfMeasure* uom = colinfos_.uoms_.validIdx( idx )
				 ? colinfos_.uoms_.get( idx ) : nullptr;
	if ( !uom )
	    unitfld_->setPropType( Mnemonic::Other );
	if ( mn )
	    unitfld_->setMnemonic( *mn );
	unitfld_->setUnit( uom );
    }

    attrSelChanged.trigger();
}


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
