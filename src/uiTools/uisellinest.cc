/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.12 2004-02-25 14:52:31 nanne Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "draw.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uispinbox.h"
#include "bufstringset.h"


static const int sMinWidth = 1;
static const int sMaxWidth = 10;

uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& l,
				const char* txt, bool wdraw, bool wcol, 
				bool wwidth )
    : uiGroup(p,"Line style selector")
    , ls(l)
    , stylesel(0)
    , colinp(0)
    , widthbox(0)
    , changed(this)
{
    if ( wdraw )
    {
	BufferStringSet itms( LineStyle::TypeNames );
	stylesel = new uiComboBox( this, itms, "Line Style" );
	stylesel->setCurrentItem( (int)ls.type );
	stylesel->selectionChanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	new uiLabel( this, txt, stylesel );
    }

    if ( wcol )
    {
	colinp = new uiColorInput( this, ls.color );
	colinp->colorchanged.notify( mCB(this,uiSelLineStyle,changeCB) );
	if ( stylesel ) colinp->attach( rightTo, stylesel );
    }

    if ( wwidth )
    {
	widthbox = new uiLabeledSpinBox( this, "Width" );
	widthbox->box()->valueChanged.notify( 
					mCB(this,uiSelLineStyle,changeCB) );
	widthbox->box()->setValue( ls.width );
	widthbox->box()->setMinValue( sMinWidth );
  	widthbox->box()->setMaxValue( sMaxWidth );
	if ( colinp )
	    widthbox->attach( rightTo, colinp );
	else if ( stylesel )
	    widthbox->attach( rightTo, stylesel );
    }

    setHAlignObj( stylesel ? (uiObject*)stylesel 
	    		   : ( colinp ? (uiObject*)colinp 
			       	      : (uiObject*)widthbox ) );
    setHCentreObj( stylesel ? (uiObject*)stylesel
	    		    : ( colinp ? (uiObject*)colinp
				       : (uiObject*)widthbox ) );
}


LineStyle uiSelLineStyle::getStyle() const
{
    LineStyle ret = ls;
    if ( stylesel )
	ret.type = (LineStyle::Type)stylesel->currentItem();
    if ( colinp ) 
	ret.color = colinp->color();
    if ( widthbox ) 
	ret.width = widthbox->box()->getValue();
    return ret;
}


void uiSelLineStyle::changeCB( CallBacker* cb )
{
    changed.trigger(cb);
}


// ========================================================

LineStyleDlg::LineStyleDlg( uiParent* p, const LineStyle& ls, const char* lbl,
			    bool wdraw, bool wcol, bool wwidth )
        : uiDialog(p,uiDialog::Setup("Display","Select linestyle"))
{   
    lsfld = new uiSelLineStyle( this, ls, lbl ? lbl : "Line style", 
				wdraw, wcol, wwidth ); 
}


LineStyle LineStyleDlg::getLineStyle() const
{   
    return lsfld->getStyle();
}

