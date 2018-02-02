/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "odsession.h"

#include "uiodcontourtreeitem.h"
#include "uiodplanedatatreeitem.h"
#include "uiodrandlinetreeitem.h"
#include "uiodseis2dtreeitem.h"
#include "uiodstratlayermodelmgr.h"
#include "uiodusrinteractionsettings.h"
#include "uiodvolproctreeitem.h"
#include "uiodvolumetreeitem.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );

    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();
    VolProc::uiDataTreeItem::initClass();
    uiODInlineAttribTreeItem::initClass();
    uiODCrosslineAttribTreeItem::initClass();
    uiODZsliceAttribTreeItem::initClass();
    uiOD2DLineAttribTreeItem::initClass();
    uiODRandomLineAttribTreeItem::initClass();
    uiODVolumeAttribTreeItem::initClass();
    uiODContourTreeItem::initClass();
    uiStratLayerModelManager::initClass();
    uiKeyboardInteractionSettingsGroup::initClass();
    uiMouseInteractionSettingsGroup::initClass();
}
