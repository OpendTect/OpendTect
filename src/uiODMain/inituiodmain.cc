/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "procdescdata.h"
#include "odsession.h"

#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiodvolproctreeitem.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );

    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();
    uiKeyBindingSettingsGroup::initClass();
    VolProc::uiDataTreeItem::initClass();

#ifdef __win__
    ePDD().add( ePDD().sKeyODExecNm(), uiODMain::sODDesc(),
						ProcDesc::DataEntry::ODv6 );
#endif
}
