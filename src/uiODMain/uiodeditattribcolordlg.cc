/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	R. K. Singh
 Date: 		Jan 2008
 RCS:		$Id: uiodeditattribcolordlg.cc,v 1.10 2008-09-15 10:10:36 cvsbert Exp $
___________________________________________________________________

-*/

#include "uiodeditattribcolordlg.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "uicolortable.h"
#include "uibutton.h"
#include "mousecursor.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "viscolortab.h"


uiODEditAttribColorDlg::uiODEditAttribColorDlg( uiParent* p,
						ObjectSet<uiTreeItem>& set,
						const char* attrnm )
    : uiDialog(p,uiDialog::Setup("Color Settings",mNoDlgTitle,mTODOHelpID))
    , items_(set)
    , itemusedineditor_(-1)
{
    setCtrlStyle( LeaveOnly );

    if ( attrnm && *attrnm )
    {
	BufferString titletext = "Color Settings : ";
	titletext += attrnm;
	setTitleText( titletext );
    }

    visBase::VisColorTab* ctabobj = 0;
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int id =
	    ODMainWin()->applMgr().visServer()->getColTabId( item->displayID(),
							    item->attribNr() );
	if ( id < 0 ) continue;
	mDynamicCastGet(visBase::VisColorTab*,obj,visBase::DM().getObject(id))
	if ( !obj ) continue;

	ctabobj = obj;
	itemusedineditor_ = idx;
	break;
    }

    ColTab::Sequence ctab = ctabobj->colorSeq().colors();
    uicoltab_ = new uiColorTable( this, ctab, true );
    uicoltab_->setAutoScale( ctabobj->autoScale() );
    uicoltab_->setClipRate( ctabobj->clipRate() );
    uicoltab_->setSymMidval( ctabobj->symMidval() );
    uicoltab_->setInterval( ctabobj->getInterval() );
    uicoltab_->seqChanged.notify( mCB(this,uiODEditAttribColorDlg,doApply) );
    uicoltab_->scaleChanged.notify( mCB(this,uiODEditAttribColorDlg,doApply) );
}


void uiODEditAttribColorDlg::doApply( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    ColTab::Sequence coltab = uicoltab_->colTabSeq();
    const bool autoscale = uicoltab_->autoScale();
    const Interval<float> intv = uicoltab_->getInterval();
    const float cliprate = uicoltab_->getClipRate();
    const float symval = uicoltab_->getSymMidval();
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int id = 
	    ODMainWin()->applMgr().visServer()->getColTabId( item->displayID(),
							    item->attribNr() );
	if ( id < 0 ) continue;
	mDynamicCastGet(visBase::VisColorTab*,obj,visBase::DM().getObject(id))
	if ( !obj ) continue;

	if ( !(coltab == obj->colorSeq().colors()) )
	{
	    obj->colorSeq().colors() = coltab;
	    obj->colorSeq().colorsChanged();
	    obj->triggerSeqChange();
	}
	if ( obj->autoScale()!=autoscale || obj->clipRate()!=cliprate
	  				 || obj->symMidval()!=symval )
	{
	    obj->autoscalechange.enable( false );
	    obj->setAutoScale( autoscale );
	    obj->setClipRate( cliprate );
	    obj->setSymMidval( symval );
	    obj->autoscalechange.enable( true );
	    obj->triggerAutoScaleChange();
	    if ( autoscale && idx == itemusedineditor_ )
	    {
		ColTab::Sequence newcoltab = obj->colorSeq().colors();
		uicoltab_->setTable( newcoltab );
	    }
	}
	if ( !autoscale && obj->getInterval() != intv )
	    obj->scaleTo( intv );

	items_[idx]->updateColumnText( uiODSceneMgr::cColorColumn() );
    }
}


bool uiODEditAttribColorDlg::acceptOK( CallBacker* )
{
    doApply( 0 );
    return true;
}
