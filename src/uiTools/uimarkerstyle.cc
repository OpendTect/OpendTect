/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimarkerstyle.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"
#include "uistrings.h"
#include "color.h"
#include "draw.h"


uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, bool withcolor,
	    const Interval<int>& rg, int nrexcluded,
	    const OD::MarkerStyle3D::Type* excluded )
    : uiGroup(p)
    , colselfld_( 0 )
    , markertypedef_( OD::MarkerStyle3D::TypeDef() )
{
    for ( int idx=markertypedef_.size()-1; idx>=0; idx-- )
    {
	const OD::MarkerStyle3D::Type type=markertypedef_.getEnumForIndex(idx);

	bool exclude = false;
	for ( int idy=0; idy<nrexcluded; idy++ )
	{
	    if ( excluded[idy] == type )
	    {
		exclude = true;
		break;
	    }
	}

	if ( exclude )
	    markertypedef_.remove( markertypedef_.getKey(type) );
    }

    StringListInpSpec str( markertypedef_ );
    typefld_ = new uiGenInput( this, tr("Marker Shape"), str );

    sliderfld_ = new uiSlider( this,
	uiSlider::Setup(uiStrings::sSize()).withedit(true), "Slider Size" );
    sliderfld_->setInterval( rg );
    sliderfld_->attach( alignedBelow, typefld_ );

    if ( withcolor )
    {
	colselfld_ = new uiColorInput( this,
		    uiColorInput::Setup(Color::White())
                    .lbltxt(uiStrings::sColor()) );
	colselfld_->attach( alignedBelow, sliderfld_ );
    }

    setHAlignObj( sliderfld_ );
}


NotifierAccess* uiMarkerStyle3D::sliderMove()
{ return &sliderfld_->valueChanged; }


NotifierAccess* uiMarkerStyle3D::typeSel()
{ return &typefld_->valuechanged; }


NotifierAccess* uiMarkerStyle3D::colSel()
{ return colselfld_ ? &colselfld_->colorChanged : 0; }


void uiMarkerStyle3D::getMarkerStyle( OD::MarkerStyle3D& st ) const
{
    st.type_ = getType();
    st.size_ = getSize();
    if ( colselfld_ ) st.color_ = colselfld_->color();
}


OD::MarkerStyle3D::Type uiMarkerStyle3D::getType() const
{ return markertypedef_.getEnumForIndex( typefld_->getIntValue() ); }


Color uiMarkerStyle3D::getColor() const
{ return colselfld_ ? colselfld_->color() : Color::Black(); }


int uiMarkerStyle3D::getSize() const
{ return sliderfld_->getIntValue(); }


void uiMarkerStyle3D::setMarkerStyle( const OD::MarkerStyle3D& st )
{
    int idx = markertypedef_.indexOf( st.type_ );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
    if ( colselfld_ )
	colselfld_->setColor( st.color_ );
    sliderfld_->setValue( st.size_ );
}
