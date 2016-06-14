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


uiMarkerStyle::uiMarkerStyle( uiParent* p, bool withcolor,
			      const Interval<int>& rg )
    : uiGroup(p)
    , colselfld_(0)
{
    typefld_ = new uiGenInput( this, tr("Marker Shape"), StringListInpSpec() );

    sizefld_ = new uiSlider( this,
	uiSlider::Setup(uiStrings::sSize()).withedit(true), "Size" );
    sizefld_->setInterval( rg );
    sizefld_->setValue( 3 );
    sizefld_->attach( alignedBelow, typefld_ );

    if ( withcolor )
    {
	colselfld_ = new uiColorInput( this,
		    uiColorInput::Setup(Color::White())
			.lbltxt(uiStrings::sColor()) );
	colselfld_->attach( alignedBelow, sizefld_ );
    }

    setHAlignObj( sizefld_ );
}


NotifierAccess* uiMarkerStyle::sizeChange()
{ return &sizefld_->valueChanged; }


NotifierAccess* uiMarkerStyle::typeSel()
{ return &typefld_->valuechanged; }


NotifierAccess* uiMarkerStyle::colSel()
{ return colselfld_ ? &colselfld_->colorChanged : 0; }


Color uiMarkerStyle::getColor() const
{ return colselfld_ ? colselfld_->color() : Color::Black(); }


int uiMarkerStyle::getSize() const
{ return sizefld_->getIntValue(); }



// uiMarkerStyle2D
uiMarkerStyle2D::uiMarkerStyle2D( uiParent* p, bool withcolor,
	const Interval<int>& rg, int nrexcluded,
	const OD::MarkerStyle2D::Type* excluded )
    : uiMarkerStyle(p,withcolor,rg)
    , markertypedef_(OD::MarkerStyle2D::TypeDef())
{
    for ( int idx=markertypedef_.size()-1; idx>=0; idx-- )
    {
	const OD::MarkerStyle2D::Type type=markertypedef_.getEnumForIndex(idx);

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

    postFinalise().notify( mCB(this,uiMarkerStyle2D,finalizeDone) );
}


void uiMarkerStyle2D::finalizeDone( CallBacker* )
{
    typefld_->newSpec( StringListInpSpec(markertypedef_), 0 );
}


void uiMarkerStyle2D::getMarkerStyle( OD::MarkerStyle2D& st ) const
{
    st.type_ = getType();
    st.size_ = getSize();
    if ( colselfld_ ) st.color_ = colselfld_->color();
}


OD::MarkerStyle2D::Type uiMarkerStyle2D::getType() const
{
    return markertypedef_.getEnumForIndex( typefld_->getIntValue() );
}


void uiMarkerStyle2D::setMarkerStyle( const OD::MarkerStyle2D& st )
{
    int idx = markertypedef_.indexOf( st.type_ );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
    if ( colselfld_ )
	colselfld_->setColor( st.color_ );
    sizefld_->setValue( st.size_ );
}


// uiMarkerStyle3D
uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, bool withcolor,
				  const Interval<int>& rg, int nrexcluded,
				  const OD::MarkerStyle3D::Type* excluded )
    : uiMarkerStyle(p,withcolor,rg)
    , markertypedef_(OD::MarkerStyle3D::TypeDef())
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

    postFinalise().notify( mCB(this,uiMarkerStyle3D,finalizeDone) );
}


void uiMarkerStyle3D::finalizeDone( CallBacker* )
{
    typefld_->newSpec( StringListInpSpec(markertypedef_), 0 );
}


void uiMarkerStyle3D::getMarkerStyle( OD::MarkerStyle3D& st ) const
{
    st.type_ = getType();
    st.size_ = getSize();
    if ( colselfld_ ) st.color_ = colselfld_->color();
}


OD::MarkerStyle3D::Type uiMarkerStyle3D::getType() const
{
    return markertypedef_.getEnumForIndex( typefld_->getIntValue() );
}


void uiMarkerStyle3D::setMarkerStyle( const OD::MarkerStyle3D& st )
{
    int idx = markertypedef_.indexOf( st.type_ );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
    if ( colselfld_ )
	colselfld_->setColor( st.color_ );
    sizefld_->setValue( st.size_ );
}
