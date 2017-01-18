/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
________________________________________________________________________

-*/

#include "uimarkerstyle.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"
#include "uistrings.h"
#include "color.h"
#include "draw.h"


uiMarkerStyle::uiMarkerStyle( uiParent* p )
    : uiGroup(p)
    , colselfld_(0)
{
}


void uiMarkerStyle::createFlds( const uiStringSet& typnms, bool wcol,
				   const Interval<int>& szrg )
{
    typefld_ = new uiGenInput( this, tr("Marker Shape"),
				StringListInpSpec(typnms) );

    sizefld_ = new uiSlider( this,
	uiSlider::Setup(uiStrings::sSize()).withedit(true), "Size" );
    sizefld_->setInterval( szrg );
    sizefld_->setValue( 3 );
    sizefld_->attach( alignedBelow, typefld_ );

    if ( wcol )
    {
	colselfld_ = new uiColorInput( this,
		    uiColorInput::Setup(Color::White())
			.lbltxt(uiStrings::sColor()) );
	colselfld_->attach( alignedBelow, sizefld_ );
    }

    setHAlignObj( sizefld_ );
}


void uiMarkerStyle::enableColorSelection( bool yn )
{
    if ( colselfld_ )
	colselfld_->setSensitive( yn );
}


void uiMarkerStyle::setMStyle( int typ, int sz, const Color& col )
{
    const int idx = types_.indexOf( typ );
    if ( idx < 0 )
	return;

    typefld_->setValue( idx );
    sizefld_->setValue( sz );
    if ( colselfld_ )
	colselfld_->setColor( col );
}


NotifierAccess* uiMarkerStyle::sizeChange()
{
    return &sizefld_->valueChanged;
}


NotifierAccess* uiMarkerStyle::typeSel()
{
    return &typefld_->valuechanged;
}


NotifierAccess* uiMarkerStyle::colSel()
{
    return colselfld_ ? &colselfld_->colorChanged : 0;
}


Color uiMarkerStyle::getColor() const
{
    return colselfld_ ? colselfld_->color() : Color::White();
}


int uiMarkerStyle::getSize() const
{
    return sizefld_->getIntValue();
}



// uiMarkerStyle2D

uiMarkerStyle2D::uiMarkerStyle2D( uiParent* p, bool wcol, Interval<int> rg,
				  const TypeSet<OD::MarkerStyle2D::Type>* excl )
    : uiMarkerStyle(p)
{
    const EnumDefImpl<OD::MarkerStyle2D::Type>& def
				= OD::MarkerStyle2D::TypeDef();
    uiStringSet nms;
    for ( int idx=0; idx<def.size(); idx++ )
    {
	const OD::MarkerStyle2D::Type mtyp = def.getEnumForIndex( idx );
	if ( !excl || !excl->isPresent(mtyp) )
	{
	    types_.add( (int)mtyp );
	    nms.add( def.toUiString( mtyp ) );
	}
    }
    createFlds( nms, wcol, rg );
}


void uiMarkerStyle2D::getMarkerStyle( OD::MarkerStyle2D& st ) const
{
    st.type_ = getType(); st.size_ = getSize();
    if ( colselfld_ )
	st.color_ = colselfld_->color();
}


OD::MarkerStyle2D::Type uiMarkerStyle2D::getType() const
{
    return (OD::MarkerStyle2D::Type)types_[ typefld_->getIntValue() ];
}


void uiMarkerStyle2D::setMarkerStyle( const OD::MarkerStyle2D& st )
{
    setMStyle( (int)st.type_, st.size_, st.color_ );
}



// uiMarkerStyle3D

uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, bool wcol, Interval<int> rg,
				  const TypeSet<OD::MarkerStyle3D::Type>* excl )
    : uiMarkerStyle(p)
{
    const EnumDefImpl<OD::MarkerStyle3D::Type>& def
				= OD::MarkerStyle3D::TypeDef();
    uiStringSet nms;
    for ( int idx=0; idx<def.size(); idx++ )
    {
	const OD::MarkerStyle3D::Type mtyp = def.getEnumForIndex( idx );
	if ( !excl || !excl->isPresent(mtyp) )
	{
	    types_.add( (int)mtyp );
	    nms.add( def.toUiString( mtyp ) );
	}
    }
    createFlds( nms, wcol, rg );
}


void uiMarkerStyle3D::getMarkerStyle( OD::MarkerStyle3D& st ) const
{
    st.type_ = getType(); st.size_ = getSize();
    if ( colselfld_ )
	st.color_ = colselfld_->color();
}


OD::MarkerStyle3D::Type uiMarkerStyle3D::getType() const
{
    return (OD::MarkerStyle3D::Type)types_[ typefld_->getIntValue() ];
}


void uiMarkerStyle3D::setMarkerStyle( const OD::MarkerStyle3D& st )
{
    setMStyle( (int)st.type_, st.size_, st.color_ );
}
