/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.8 2002-05-31 10:26:51 nanne Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "draw.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uidset.h"
#include "uispinbox.h"


uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& l,
				const char* txt, bool wcol, bool wwidth )
	: uiGroup(p,"Line style selector")
	, ls(l)
	, colinp(0)
	, widthbox(0)
{
    UserIDSet itms( LineStyle::TypeNames );
    itms.setName( "Line Style" );
    stylesel = new uiComboBox( this, itms );
    stylesel->setCurrentItem( (int)ls.type );
    new uiLabel( this, txt, stylesel );
    if ( wcol )
    {
	colinp = new uiColorInput( this, ls.color );
	colinp->attach( rightOf, stylesel );
    }

    if ( wwidth )
    {
	widthbox = new uiLabeledSpinBox( this, "Width" );
	if ( wcol ) 	widthbox->attach( rightOf, colinp );
	else 		widthbox->attach( rightOf, stylesel );
	widthbox->box()->setValue( ls.width );
	widthbox->box()->setMinValue( 1 );
  	widthbox->box()->setMaxValue( 10 );
    }

    setHAlignObj( stylesel );
    setHCentreObj( stylesel );
}


LineStyle uiSelLineStyle::getStyle() const
{
    LineStyle ret = ls;
    ret.type = (LineStyle::Type)stylesel->currentItem();
    if ( colinp ) ret.color = colinp->color();
    if ( widthbox ) ret.width = widthbox->box()->getIntValue();
    return ret;
}


LineStyleDlg::LineStyleDlg( uiParent* p, const LineStyle& ls, const char* lbl,
			    bool wcol, bool wwidth )
        : uiDialog(p,uiDialog::Setup("Display","Select linestyle"))
{   
    lsfld = new uiSelLineStyle( this, ls, lbl ? lbl : "Line style", 
				wcol, wwidth ); 
}


LineStyle LineStyleDlg::getLineStyle() const
{   
    return lsfld->getStyle();
}

