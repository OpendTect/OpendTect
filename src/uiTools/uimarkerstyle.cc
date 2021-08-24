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
#include "uispinbox.h"
#include "uistrings.h"
#include "color.h"
#include "draw.h"


// uiMarkerStyle2D
uiMarkerStyle2D::uiMarkerStyle2D( uiParent* p, const Setup& su )
    : uiGroup(p)
    , changed(this)
    , typefld_(0)
    , colorfld_(0)
    , szfld_(0)
{
    StringListInpSpec str;
    const BufferStringSet stylenms( MarkerStyle2D::TypeNames() );
    for ( int idx=0; idx<stylenms.size(); idx++ )
    {
	const BufferString& stylenm = stylenms.get( idx );
	const MarkerStyle2D::Type tp =
		MarkerStyle2D::parseEnumType( stylenm.buf() );
	if ( su.toexclude_.isPresent(tp) )
	    continue;

	types_.add( tp );
	str.addString( toUiString(stylenm) );
    }

    uiString lbltxt( su.lbltxt_ );
    if ( lbltxt.isEmpty() )
	lbltxt = tr("Marker style");
    else if ( lbltxt.getFullString() == "-" )
	lbltxt.setEmpty();

    uiObject* alignobj = 0;
    if ( su.shape_ )
    {
	typefld_ = new uiGenInput( this, lbltxt, str );
	typefld_->valuechanged.notify( mCB(this,uiMarkerStyle2D,changeCB) );
	alignobj = typefld_->attachObj();
    }

    if ( su.color_ )
    {
	uiColorInput::Setup csu( Color::White(), su.transparency_ ?
		uiColorInput::Setup::InSelector : uiColorInput::Setup::None );
	csu.lbltxt( typefld_ ? uiStrings::sColor() : tr("Marker color") )
	    .withdesc(false);
	colorfld_ = new uiColorInput( this,  csu );
	colorfld_->colorChanged.notify( mCB(this,uiMarkerStyle2D,changeCB) );
	if ( typefld_ )
	    colorfld_->attach( rightTo, typefld_ );
	else
	    alignobj = colorfld_->attachObj();

    }

    if ( su.sz_ )
    {
	uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
		su.color_||su.shape_ ? uiStrings::sSize() : tr("Marker size") );
	szfld_ = lsb->box();
	szfld_->setMinValue( 1 );
	szfld_->valueChanging.notify( mCB(this,uiMarkerStyle2D,changeCB) );
	if ( colorfld_ )
	    lsb->attach( rightTo, colorfld_ );
	else if ( typefld_ )
	    lsb->attach( rightTo, typefld_ );
	else
	    alignobj = lsb->attachObj();
    }

    setHAlignObj( alignobj );
}


uiMarkerStyle2D::~uiMarkerStyle2D()
{}


void uiMarkerStyle2D::setMarkerStyle( const MarkerStyle2D& st )
{
    setMarkerType( st.type_ );
    setMarkerColor( st.color_ );
    setMarkerSize( st.size_ );
}


MarkerStyle2D uiMarkerStyle2D::getMarkerStyle() const
{
    MarkerStyle2D st;
    st.type_ = getMarkerType();
    st.size_ = getMarkerSize();
    st.color_ = getMarkerColor();
    return st;
}


void uiMarkerStyle2D::setMarkerType( MarkerStyle2D::Type tp )
{
    if ( !typefld_ )
	return;

    int idx = types_.indexOf( tp );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
}


MarkerStyle2D::Type uiMarkerStyle2D::getMarkerType() const
{
    const int idx = typefld_ ? typefld_->getIntValue() : 0;
    return types_.validIdx(idx) ? types_[idx] : MarkerStyle2D::Circle;
}


void uiMarkerStyle2D::setMarkerColor( const Color& col )
{
    if ( colorfld_ )
	colorfld_->setColor( col );
}


Color uiMarkerStyle2D::getMarkerColor() const
{
    return colorfld_ ? colorfld_->color() : Color::Black();
}


void uiMarkerStyle2D::setMarkerSize( int sz )
{
    if ( szfld_ )
	szfld_->setValue(sz);
}


int uiMarkerStyle2D::getMarkerSize() const
{
    return szfld_ ? szfld_->getIntValue() : 1;
}


void uiMarkerStyle2D::changeCB( CallBacker* )
{ changed.trigger(); }



// uiMarkerStyle3D
uiMarkerStyle3D::uiMarkerStyle3D( uiParent* p, bool withcolor,
	    const Interval<int>& rg, int nrexcluded,
	    const MarkerStyle3D::Type* excluded )
    : uiGroup(p)
    , colselfld_( nullptr )
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

	str.addString( mToUiStringTodo(MarkerStyle3D::TypeNames()[idx]) );
	types_ += type;
    }

    uiObject* alignobj = nullptr;

    typefld_ = new uiGenInput( this, tr("Marker Shape"), str );
    alignobj = typefld_->attachObj();

    if ( withcolor )
    {
	colselfld_ = new uiColorInput( this,
				uiColorInput::Setup(Color::White())
				.lbltxt(uiStrings::sColor()).withdesc(false) );
	if ( typefld_ )
	    colselfld_->attach( rightTo, typefld_ );
	else
	    alignobj = colselfld_->attachObj();
    }

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, tr("Size") );
    szfld_ = lsb->box();
    szfld_->setMinValue( 1 );
    if ( colselfld_ )
	lsb->attach( rightTo, colselfld_ );
    else if ( typefld_ )
	lsb->attach( rightTo, typefld_ );
    else
	alignobj = lsb->attachObj();

    setHAlignObj( alignobj );
}


NotifierAccess* uiMarkerStyle3D::sliderMove()
{ return &szfld_->valueChanging; }


NotifierAccess* uiMarkerStyle3D::typeSel()
{ return &typefld_->valuechanged; }


NotifierAccess* uiMarkerStyle3D::colSel()
{ return colselfld_ ? &colselfld_->colorChanged : nullptr; }


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
{ return szfld_->getIntValue(); }


void uiMarkerStyle3D::setMarkerStyle( const MarkerStyle3D& st )
{
    int idx = types_.indexOf( st.type_ );
    if ( idx<0 )
	idx = 0;

    typefld_->setValue( idx );
    if ( colselfld_ )
	colselfld_->setColor( st.color_ );
    szfld_->setValue( st.size_ );
}


void uiMarkerStyle3D::enableColorSelection( bool yn )
{
    if ( colselfld_ )
	colselfld_->setSensitive(yn);
}
