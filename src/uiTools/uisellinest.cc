/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uisellinest.cc,v 1.5 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uisellinest.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uidset.h"


uiSelLineStyle::uiSelLineStyle( uiParent* p, const LineStyle& l,
				const char* txt )
	: uiGroup( p, "Line style selector" )
	, ls( l )
{
    UserIDSet itms( LineStyle::TypeNames );
    itms.setName( "Line Style" );
    stylesel = new uiComboBox( this, itms );
    stylesel->setCurrentItem( (int)ls.type );
    new uiLabel( this, txt, stylesel );
    colinp = new uiColorInput( this, ls.color );
    colinp->attach( rightOf, stylesel );

    setHAlignObj( stylesel );
    setHCentreObj( stylesel );
}


LineStyle uiSelLineStyle::getStyle() const
{
    LineStyle ret = ls;
    ret.type = (LineStyle::Type)stylesel->currentItem();
    ret.color = colinp->color();
    return ret;
}
