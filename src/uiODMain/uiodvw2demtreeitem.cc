/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	DZH
 Date:		Apr 2016
________________________________________________________________________

-*/

#include "uiodvw2demtreeitem.h"

#include "uicolor.h"
#include "uiempartserv.h"
#include "uimpepartserv.h"
#include "uiodapplmgr.h"
#include "uiodviewer2dmgr.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "uitreeitem.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "mpeengine.h"
#include "commondefs.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioobj.h"
#include "ptrman.h"


uiODVw2DEMTreeItem::uiODVw2DEMTreeItem( const DBKey& emid )
    : uiODVw2DTreeItem(uiString::empty())
    , emid_( emid )
{
}


uiODVw2DEMTreeItem::~uiODVw2DEMTreeItem()
{}


void uiODVw2DEMTreeItem::doSave()
{
    bool savewithname = false;
    if ( emid_.isValid() )
    {
	PtrMan<IOObj> ioobj = emid_.getIOObj();
	savewithname = !ioobj;
    }
    doStoreObject( savewithname );

    uiMPEPartServer* mps = applMgr()->mpeServer();
    if ( MPE::engine().hasTracker(emid_) && mps )
	mps->saveSetup( emid_ );
}


void uiODVw2DEMTreeItem::doSaveAs()
{
    doStoreObject( true );

    uiMPEPartServer* mps = applMgr()->mpeServer();
    if ( MPE::engine().hasTracker(emid_) && mps )
    {
       mps->prepareSaveSetupAs( emid_ );
       mps->saveSetupAs( emid_ );
    }
}


void uiODVw2DEMTreeItem::doStoreObject( bool saveas )
{
    applMgr()->EMServer()->storeObject( emid_, saveas );
    renameVisObj();
}


void uiODVw2DEMTreeItem::renameVisObj()
{
    TypeSet<int> visobjids;
    applMgr()->visServer()->findObject( emid_, visobjids );
    name_ = ::toUiString( emid_.name() );
    for ( int idx = 0; idx<visobjids.size(); idx++ )
	applMgr()->visServer()->setUiObjectName( visobjids[idx], name_ );
    uiTreeItem::updateColumnText(uiODViewer2DMgr::cNameColumn());
    applMgr()->visServer()->triggerTreeUpdate();
}


void uiODVw2DEMTreeItem::displayMiniCtab()
{
    const EM::Object* emobj = EM::MGR().getObject( emid_ );
    if ( !emobj ) return;

    uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(),
				emobj->preferredColor() );
}


void uiODVw2DEMTreeItem::emobjChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(EM::ObjectCallbackData,cbdata,caller,cb);
    mDynamicCastGet(EM::Object*,emobj,caller);
    if ( !emobj ) return;

    if ( cbdata.changeType() == EM::Object::cPrefColorChange() )
	displayMiniCtab();
    else if ( cbdata.changeType() == EM::Object::cNameChange() )
    {
	name_ = toUiString( emid_ .name() );
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cNameColumn() );
    }
}


void uiODVw2DEMTreeItem::showPropDlg()
{
    const EM::Object* emobj = EM::MGR().getObject( emid_ );
    if ( !emobj ) return;

    uiDialog dlg( getUiParent(), uiDialog::Setup(uiStrings::sProperties(),
						mNoDlgTitle,mNoHelpKey) );
    dlg.setCtrlStyle( uiDialog::CloseOnly );
    uiSelLineStyle::Setup lssu;
    lssu.drawstyle(false);
    OD::LineStyle ls = emobj->preferredLineStyle();
    ls.color_ = emobj->preferredColor();
    uiSelLineStyle* lsfld = new uiSelLineStyle( &dlg, ls, lssu );
    lsfld->changed.notify( mCB(this,uiODVw2DEMTreeItem,propChgCB) );
    dlg.go();
}


void uiODVw2DEMTreeItem::propChgCB( CallBacker* cb )
{
    EM::Object* emobj = EM::MGR().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(uiColorInput*,colfld,cb)
    if ( colfld )
    {
	emobj->setPreferredColor( colfld->color() );
	return;
    }

    OD::LineStyle ls = emobj->preferredLineStyle();
    mDynamicCastGet(uiSpinBox*,szfld,cb)
    if ( szfld )
    {
	ls.width_ = szfld->getIntValue();
	emobj->setPreferredLineStyle( ls );
    }
}
