/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwindowfuncseldlg.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uilistbox.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "randcolor.h"
#include "scaler.h"
#include "windowfunction.h"


#define mTransHeight	250
#define mTransWidth	500

uiFuncDrawerBase::uiFuncDrawerBase( const Setup& su )
    : setup_(su)
    , funcrg_(su.funcrg_)
{
}


uiFuncDrawerBase::~uiFuncDrawerBase()
{
    clearFunctions();

}


void uiFuncDrawerBase::clearFunction( int idx )
{
    delete functions_.removeSingle(idx);
}
