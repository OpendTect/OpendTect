/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.6 2002-05-22 11:03:52 nanne Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uidset.h"
#include "draw.h"


uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& l,
				const char* txt, bool wcol )
	: uiGroup(p,"Line style selector")
	, ls(l)
	, colinp(0)
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

    setHAlignObj( stylesel );
    setHCentreObj( stylesel );
}


LineStyle uiSelLineStyle::getStyle() const
{
    LineStyle ret = ls;
    ret.type = (LineStyle::Type)stylesel->currentItem();
    if ( colinp ) ret.color = colinp->color();
    return ret;
}
