/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisellinest.cc,v 1.34 2011-01-14 13:37:04 cvsjaap Exp $";

#include "uisellinest.h"
#include "draw.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uispinbox.h"
#include "bufstringset.h"


static const int cMinWidth = 1;
static const int cMaxWidth = 100;

uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& ls,
				const char* txt, bool wdraw, bool wcol, 
				bool wwidth )
    : uiGroup(p,"Line style selector")
    , linestyle(*new LineStyle(ls))
    , stylesel(0)
    , colinp(0)
    , widthbox(0)
    , changed(this)
{
    if ( wdraw )
    {
	BufferStringSet itms( LineStyle::TypeNames() );
	stylesel = new uiComboBox( this, itms, "Line Style" );
	stylesel->setCurrentItem( (int)linestyle.type_ );
	stylesel->selectionChanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	new uiLabel( this, txt, stylesel );
    }

    if ( wcol )
    {
	colinp = new uiColorInput( this,
				   uiColorInput::Setup(linestyle.color_).
				   lbltxt(wdraw ? "Color" : "Line color") );
	colinp->colorChanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	if ( stylesel )
	    colinp->attach( rightTo, stylesel );
    }

    if ( wwidth )
    {
	widthbox = new uiLabeledSpinBox( this, wcol || wdraw ? "Width" 
							     : "Line width" );
	widthbox->box()->setMinValue( mMIN(cMinWidth,linestyle.width_) );
  	widthbox->box()->setMaxValue( mMAX(cMaxWidth,linestyle.width_) );
	widthbox->box()->setValue( linestyle.width_ );
	if ( colinp )
	    widthbox->attach( rightTo, colinp );
	else if ( stylesel )
	    widthbox->attach( rightTo, stylesel );

	widthbox->box()->valueChanging.notify( 
					mCB(this,uiSelLineStyle,changeCB) );
    }

    setHAlignObj( stylesel ? (uiObject*)stylesel 
	    		   : ( colinp ? (uiObject*)colinp->attachObj() 
			       	      : (uiObject*)widthbox->attachObj() ) );
    setHCentreObj( stylesel ? (uiObject*)stylesel
	    		    : ( colinp ? (uiObject*)colinp->attachObj()
				       : (uiObject*)widthbox->attachObj() ) );
}


uiSelLineStyle::~uiSelLineStyle()
{
    delete &linestyle;
}


const LineStyle& uiSelLineStyle::getStyle() const
{
    return linestyle;
}


void uiSelLineStyle::setStyle( const LineStyle& ls )
{
    setColor( ls.color_ );
    setWidth( ls.width_ );
    setType( (int)ls.type_ );
    if ( colinp )
	colinp->setSensitive( linestyle.type_ != LineStyle::None );

    if ( widthbox )
	widthbox->setSensitive( linestyle.type_ != LineStyle::None );
}


void uiSelLineStyle::enableTransparency( bool yn )
{
    colinp->enableAlphaSetting( yn );
}


void uiSelLineStyle::setLineWidthBounds( int min, int max )
{
    widthbox->box()->setMinValue( min );
    widthbox->box()->setMaxValue( max );
}


void uiSelLineStyle::setColor( const Color& col )
{
    linestyle.color_ = col;
    if ( colinp ) colinp->setColor( col );
}


const Color& uiSelLineStyle::getColor() const
{
    return linestyle.color_;
}


void uiSelLineStyle::setWidth( int width )
{
    linestyle.width_ = width;
    if ( widthbox ) widthbox->box()->setValue( width );
}


int uiSelLineStyle::getWidth() const
{
    return linestyle.width_;
}


void uiSelLineStyle::setType( int tp )
{
    linestyle.type_ = (LineStyle::Type)tp;
    if ( stylesel ) stylesel->setCurrentItem( tp );
}


int uiSelLineStyle::getType() const
{
    return (int)linestyle.type_;
}


void uiSelLineStyle::changeCB( CallBacker* cb )
{
    if ( stylesel )
	linestyle.type_ = (LineStyle::Type)stylesel->currentItem();

    if ( colinp )
    {
	linestyle.color_ = colinp->color();
	colinp->setSensitive( linestyle.type_ != LineStyle::None );
    }

    if ( widthbox )
    {
	linestyle.width_ = widthbox->box()->getValue();
	widthbox->setSensitive( linestyle.type_ != LineStyle::None );
    }

    changed.trigger(cb);
}
