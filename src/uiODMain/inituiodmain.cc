/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "odsession.h"

#include "uiodscenemgr.h"
#include "uiodplanedatatreeitem.h"
#include "uiodvolproctreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodvolrentreeitem.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );

    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();
    uiKeyBindingSettingsGroup::initClass();
    VolProc::uiDataTreeItem::initClass();
    uiODInlineAttribTreeItem::initClass();
    uiODCrosslineAttribTreeItem::initClass();
    uiODZsliceAttribTreeItem::initClass();
    uiOD2DLineAttribTreeItem::initClass();
    uiODRandomLineAttribTreeItem::initClass();
    uiODVolrenAttribTreeItem::initClass();
}
