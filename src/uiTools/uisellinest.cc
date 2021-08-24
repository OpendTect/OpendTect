/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/

#include "uisellinest.h"

#include "bufstringset.h"
#include "draw.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uistrings.h"


static const int cMinWidth = 1;
static const int cMaxWidth = 100;

uiSelLineStyle::uiSelLineStyle( uiParent* p, const OD::LineStyle& ls,
				const uiSelLineStyle::Setup& su )
    : uiGroup(p,"Line style selector")
    , linestyle_(*new OD::LineStyle(ls))
    , changed(this)
{
    init( su );
}

uiSelLineStyle::uiSelLineStyle( uiParent* p, const OD::LineStyle& ls,
				const uiString& ltxt )
    : uiGroup(p,"Line style selector")
    , linestyle_(*new OD::LineStyle(ls))
    , changed(this)
{
    init( Setup(ltxt) );
}


void uiSelLineStyle::init( const uiSelLineStyle::Setup& su )
{
    stylesel_ = nullptr; colinp_ = nullptr; widthbox_ = nullptr;

    uiString lbltxt( su.txt_ );
    if ( lbltxt.isEmpty() )
	lbltxt = tr("Line style");
    else if ( lbltxt.getFullString() == "-" )
	lbltxt.setEmpty();

    uiObject* alobj = nullptr;

    if ( su.drawstyle_ )
    {
	stylesel_ = new uiGenInput( this, lbltxt,
				StringListInpSpec(OD::LineStyle::TypeDef()) );
	stylesel_->setValue( linestyle_.type_ );
	mAttachCB( stylesel_->valuechanged, uiSelLineStyle::changeCB );
	alobj = stylesel_->attachObj();
    }

    if ( su.color_ )
    {
	uiColorInput::Setup csu( linestyle_.color_, su.transparency_
		? uiColorInput::Setup::InSelector : uiColorInput::Setup::None );
	csu.lbltxt( stylesel_ ? uiStrings::sColor()
			      : tr("Line color") ).withdesc( !su.width_ );
	colinp_ = new uiColorInput( this, csu );
	mAttachCB( colinp_->colorChanged,uiSelLineStyle::changeCB );
	if ( stylesel_ )
	    colinp_->attach( rightTo, stylesel_ );
	else if ( !alobj )
	    alobj = colinp_->attachObj();
    }

    if ( su.width_ )
    {
	widthbox_ = new uiLabeledSpinBox( this,
		    su.color_ || su.drawstyle_ ? tr("Width")
					       : tr("Line width") );
	widthbox_->box()->setMinValue( mMIN(cMinWidth,linestyle_.width_) );
	widthbox_->box()->setMaxValue( mMAX(cMaxWidth,linestyle_.width_) );
	widthbox_->box()->setValue( linestyle_.width_ );
	if ( colinp_ )
	    widthbox_->attach( rightTo, colinp_ );
	else if ( stylesel_ )
	    widthbox_->attach( rightTo, stylesel_ );
	else if ( !lbltxt.isEmpty() )
	{
	    new uiLabel( this, uiStrings::sEmptyString(), widthbox_ );
	    if ( !alobj )
		alobj = widthbox_->attachObj();
	}

	mAttachCB( widthbox_->box()->valueChanging,uiSelLineStyle::changeCB );
    }

    setHAlignObj( alobj );
}


uiSelLineStyle::~uiSelLineStyle()
{
    detachAllNotifiers();
    delete &linestyle_;
}


const OD::LineStyle& uiSelLineStyle::getStyle() const
{
    return linestyle_;
}


void uiSelLineStyle::setStyle( const OD::LineStyle& ls )
{
    setColor( ls.color_ );
    setWidth( ls.width_ );
    setType( (int)ls.type_ );
    if ( colinp_ )
	colinp_->setSensitive( linestyle_.type_ != OD::LineStyle::None );

    if ( widthbox_ )
	widthbox_->setSensitive( linestyle_.type_ != OD::LineStyle::None );
}


void uiSelLineStyle::setLineWidthBounds( int min, int max )
{
    widthbox_->box()->setMinValue( min );
    widthbox_->box()->setMaxValue( max );
    if ( min == -1 )
	widthbox_->box()->setSpecialValueText( "pixel" );
}


void uiSelLineStyle::setColor( const Color& col )
{
    linestyle_.color_ = col;
    if ( colinp_ ) colinp_->setColor( col );
}


const Color& uiSelLineStyle::getColor() const
{
    return linestyle_.color_;
}


void uiSelLineStyle::setWidth( int width )
{
    linestyle_.width_ = width;
    if ( widthbox_ ) widthbox_->box()->setValue( width );
}


int uiSelLineStyle::getWidth() const
{
    return linestyle_.width_;
}


void uiSelLineStyle::setType( int tp )
{
    linestyle_.type_ = (OD::LineStyle::Type)tp;
    if ( stylesel_ )
	stylesel_->setValue( tp );
}


int uiSelLineStyle::getType() const
{
    return (int)linestyle_.type_;
}


void uiSelLineStyle::changeCB( CallBacker* cb )
{
    if ( stylesel_ )
	linestyle_.type_ = (OD::LineStyle::Type)stylesel_->getIntValue();

    if ( colinp_ )
    {
	linestyle_.color_ = colinp_->color();
	colinp_->setSensitive( linestyle_.type_ != OD::LineStyle::None );
    }

    if ( widthbox_ )
    {
	linestyle_.width_ = widthbox_->box()->getIntValue();
	widthbox_->setSensitive( linestyle_.type_ != OD::LineStyle::None );
    }

    changed.trigger(cb);
}
