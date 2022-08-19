/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uizrangeselect.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiwellmarkersel.h"
#include "uizrangeinput.h"
#include "unitofmeasure.h"


uiZRangeSelect::uiZRangeSelect( uiParent* p, const uiString& lbl, bool zintwt )
    : uiGroup(p, "Select Z Range")
    , zintwt_(zintwt && SI().zIsTime())

{
    BufferStringSet zchoices = getZChoiceStrings();
    zchoicefld_ = new uiGenInput( this, lbl, StringListInpSpec(zchoices) );
    mAttachCB(zchoicefld_->valuechanged, uiZRangeSelect::updateDisplayCB);
    setHAlignObj( zchoicefld_ );

    markersel_ = new uiWellMarkerSel( this, uiWellMarkerSel::Setup(false) );
    markersel_->attach( alignedBelow, zchoicefld_ );

    markeroffsetfld_ = new uiZRangeInput( this, zintwt_, false );
    markeroffsetfld_->attach( alignedBelow, markersel_ );

    zrangefld_ = new uiZRangeInput( this, zintwt_, false );
    zrangefld_->attach( alignedBelow, zchoicefld_ );

    setZinTime( zintwt_ );
}


uiZRangeSelect::~uiZRangeSelect()
{
    detachAllNotifiers();
}


void uiZRangeSelect::setMarkers( const BufferStringSet& mrks )
{
    markersel_->setMarkers( mrks );
}


void uiZRangeSelect::reset()
{
    zrangefld_->setEmpty();
    markeroffsetfld_->setEmpty();
    markersel_->reset();
    zchoicefld_->setValue( 0 );
    updateDisplayCB( nullptr );
}


BufferStringSet uiZRangeSelect::getZChoiceStrings() const
{
    BufferStringSet zstrings;
    zstrings.add( BufferString("Markers") );
    if ( zintwt_ )
	zstrings.add( BufferString("TWT range") );
    else
	zstrings.add( BufferString("Depth range") );

    return zstrings;
}


void uiZRangeSelect::updateDisplayCB( CallBacker* )
{
    const bool showmarkers = zchoicefld_->getIntValue()==0;
    markersel_->display( showmarkers );
    markeroffsetfld_->display( showmarkers );
    zrangefld_->display( !showmarkers );
}


void uiZRangeSelect::updateLabels()
{
    uiString unitlbl = UnitOfMeasure::zUnitAnnot( zintwt_, true, true );
    markeroffsetfld_->setTitleText( tr("Offset above/below %1 ").arg(unitlbl) );
    zrangefld_->setTitleText( tr("Start / Stop %1 ").arg(unitlbl) );
}


void uiZRangeSelect::setZinTime( bool yn )
{
    zintwt_ = yn;
    int sel = zchoicefld_->getIntValue();
    zchoicefld_->newSpec( StringListInpSpec(getZChoiceStrings()), 0 );
    zchoicefld_->setValue( sel );
    markeroffsetfld_->setIsDepth( !yn );
    zrangefld_->setIsDepth( !yn );
    updateLabels();
    updateDisplayCB( nullptr );
}


Well::ZRangeSelector uiZRangeSelect::zRangeSel()
{
    Well::ZRangeSelector params;
    if ( zchoicefld_->getIntValue()==0 )
    {
	const Interval<float> off = markeroffsetfld_->getFZRange();
	params.setTopMarker( markersel_->getText(true),
					 mIsUdf(off.start) ? 0.f : off.start );
	params.setBotMarker( markersel_->getText(false),
					 mIsUdf(off.stop) ? 0.f : off.stop );
    }
    else
    {
	Interval<float> zrng = zrangefld_->getFZRange();
	if ( !mIsUdf(zrng.start) || !mIsUdf(zrng.stop) )
	{
	    if ( mIsUdf(zrng.start) )
		zrng.start = 0.f;
	    if ( mIsUdf(zrng.stop) )
		zrng.stop = 0.f;
	}
	params.setFixedRange( zrng, zintwt_ );
    }

    return params;
}


void uiZRangeSelect::setSensitive( bool choice_yn, bool markersel_yn,
				   bool offset_yn, bool zrange_yn )
{
    zchoicefld_->setSensitive( choice_yn );
    markersel_->setSensitive( markersel_yn );
    markeroffsetfld_->setSensitive( offset_yn );
    zrangefld_->setSensitive( zrange_yn );
}


void uiZRangeSelect::setSelectedMarkers( const char* upper, const char* lower )
{
    auto* upper_sel = markersel_->getFld( true );
    auto* lower_sel = markersel_->getFld( false );

    if ( upper )
	upper_sel->setCurrentItem( upper );
    else
	upper_sel->setCurrentItem( 0 );

    if ( lower )
	lower_sel->setCurrentItem( lower );
    else
	lower_sel->setCurrentItem( lower_sel->size()-1 );
}


bool uiZRangeSelect::inMarkerMode() const
{
    return zchoicefld_->getIntValue()==0;
}
