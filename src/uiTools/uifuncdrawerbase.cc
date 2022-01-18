/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		January 2022
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
