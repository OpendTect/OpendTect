/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimarkerstyle.cc,v 1.5 2012/07/10 13:06:08 cvskris Exp $";

#include "uimarkerstyle.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uislider.h"
#include "color.h"
#include "draw.h"


uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, bool withcolor,
	    const Interval<int>& rg, int nrexcluded,
	    const MarkerStyle3D::Type* excluded )
	: uiGroup(p)
	, colselfld_( 0 )
{
    StringListInpSpec str;
    for ( int idx=0; MarkerStyle3D::TypeNames()[idx]; idx++ )
    {
	int typenr = getIndexInStringArrCI(MarkerStyle3D::TypeNames()[idx],
					    MarkerStyle3D::TypeNames() ) - 1;
	MarkerStyle3D::Type type = (MarkerStyle3D::Type) typenr;

	bool exclude = false;
	for ( int idy=0; idy<nrexcluded; idy++ )
	{
	    if ( excluded[idy] == type )
	    {
		exclude = true;
		break;
	    }
	}

	if ( exclude ) continue;

	str.addString( MarkerStyle3D::TypeNames()[idx] );
	types_ += type;
    }

    typefld_ = new uiGenInput( this, "Marker Shape", str );

    sliderfld_ = new uiSliderExtra( this, 
	    			   uiSliderExtra::Setup("Size").withedit(true)
				   ,"Slider Size");
    sliderfld_->sldr()->setMinValue( rg.start );
    sliderfld_->sldr()->setMaxValue( rg.stop );
    sliderfld_->attach( alignedBelow, typefld_ );

    if ( withcolor )
    {
	colselfld_ = new uiColorInput( this,
		    uiColorInput::Setup(Color::White()).lbltxt("Color") );
	colselfld_->attach( alignedBelow, sliderfld_ );
    }

    setHAlignObj( sliderfld_ );
}


NotifierAccess* uiMarkerStyle3D::sliderMove()
{ return &sliderfld_->sldr()->valueChanged; }


NotifierAccess* uiMarkerStyle3D::typeSel()
{ return &typefld_->valuechanged; }


NotifierAccess* uiMarkerStyle3D::colSel()
{ return colselfld_ ? &colselfld_->colorChanged : 0; }


void uiMarkerStyle3D::getMarkerStyle( MarkerStyle3D& st ) const
{
    st.type_ = types_[typefld_->getIntValue()];
    st.size_ = getSize();
    if ( colselfld_ ) st.color_ = colselfld_->color();
}


MarkerStyle3D::Type uiMarkerStyle3D::getType() const
{ return types_[typefld_->getIntValue()]; }


Color uiMarkerStyle3D::getColor() const
{ return colselfld_ ? colselfld_->color() : Color::Black(); }


int uiMarkerStyle3D::getSize() const
{
    const int sz = mNINT32(sliderfld_->sldr()->getValue() );
    sliderfld_->processInput();
    const int res = mNINT32(sliderfld_->sldr()->getValue() );
    if ( res!=sz )
	sliderfld_->sldr()->valueChanged.trigger( 0 );

    return res;
}


void uiMarkerStyle3D::setMarkerStyle( const MarkerStyle3D& st )
{
    int idx = types_.indexOf( st.type_ );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
    sliderfld_->sldr()->setValue( st.size_ );
    if ( colselfld_ ) colselfld_->setColor( st.color_ );
}
