/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          December 2009
________________________________________________________________________

-*/

#include "uiarray1dinterpol.h"

#include "array1dinterpol.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"


uiArray1DInterpolSel::uiArray1DInterpolSel( uiParent* p, bool doextrapolate,
					    bool maxgapsz )
    : uiDlgGroup( p, tr("Array2D Interpolation") )
    , polatefld_( 0 )
    , maxgapszfld_( 0 )
{
    uiObject* prevfld = 0;
    if ( doextrapolate )
    {
	BufferStringSet filltypnm; filltypnm.add( "Interpolate" );
	filltypnm.add( "Interpolate & Extrapolate" );
	polatefld_ =
	    new uiGenInput( this, tr("Scope"), BoolInpSpec(true,
			    tr("Interpolate"),tr("Interpolate & Extrapolate")));
	prevfld = polatefld_->attachObj();
    }

    if ( maxgapsz )
    {
	maxgapszfld_ = new uiGenInput( this, uiStrings::sEmptyString(),
				       FloatInpSpec() );
	maxgapszfld_->setWithCheck( true );
	if ( prevfld )
	    maxgapszfld_->attach( alignedBelow, prevfld );

	prevfld = maxgapszfld_->attachObj();
    }

    BufferStringSet algonms; algonms.add( "Linear Interpolation" );
    algonms.add( "Polynomial Interpolation" );
    uiLabeledComboBox* lcbbx =
	new uiLabeledComboBox( this, algonms, tr("Algorithm") );
    methodsel_ = lcbbx->box();
    setHAlignObj( methodsel_ );

    if ( prevfld )
	lcbbx->attach( alignedBelow, prevfld );
}


uiArray1DInterpolSel::~uiArray1DInterpolSel()
{
}


void uiArray1DInterpolSel::setDistanceUnit( const uiString& du )
{
    if ( maxgapszfld_ )
    {
	uiString res = tr("Keep holes larger than %1").arg( du );
	maxgapszfld_->setTitleText( res );
    }
}


void uiArray1DInterpolSel::setInterpolators( int totalnr )
{
    results_.erase();
    for ( int idx=0; idx<totalnr; idx++ )
    {
	if ( methodsel_->currentItem()==1 )
	    results_ += new PolyArray1DInterpol();
	else
	    results_ += new LinearArray1DInterpol();
    }
}


void uiArray1DInterpolSel::setArraySet( ObjectSet< Array1D<float> >& arrset )
{
    for ( int idx=0; idx<results_.size(); idx++ )
	results_[idx]->setArray( *arrset[idx] );
}


bool uiArray1DInterpolSel::acceptOK()
{
    if ( maxgapszfld_ && maxgapszfld_->isChecked() &&
	 (mIsUdf( maxgapszfld_->getFValue() ) ||
	  maxgapszfld_->getFValue()<=0 ) )
    {
	uiMSG().error(tr("Maximum hole size not set"
                         " or is less or equal to zero"));
	return false;
    }

    for ( int idx=0; idx<results_.size(); idx++ )
	results_[idx]->setMaxGapSize(
		mCast( float, maxgapszfld_ && maxgapszfld_->isChecked() 
		? maxgapszfld_->getIntValue() : mUdf(int) ) );

    return true;
}


Array1DInterpol* uiArray1DInterpolSel::getResult( int nr )
{
    Array1DInterpol* res = results_[nr];
    return res;
}
