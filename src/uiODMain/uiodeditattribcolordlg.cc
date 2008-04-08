/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	R. K. Singh
 Date: 		Jan 2008
 RCS:		$Id: uiodeditattribcolordlg.cc,v 1.7 2008-04-08 05:50:52 cvssatyaki Exp $
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
    : uiDialog(p,uiDialog::Setup("Color Settings","",""))
    , items_(set)
    , itemusedineditor_(-1)
{
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

    const Interval<float> intv = ctabobj->getInterval();
    ColTab::Sequence ctab = ctabobj->colorSeq().colors();
    uicoltab_ = new uiColorTable( this, ctab, true );
    uicoltab_->setPrefHeight( 400 );
    const bool autoscale = ctabobj->autoScale();
    uicoltab_->setAutoScale( autoscale );
    uicoltab_->setClipRate( ctabobj->clipRate() );
    uicoltab_->setSymmetry( ctabobj->getSymmetry() );

    uiPushButton* applybut = new uiPushButton( this, "Apply", true );
    applybut->attach( centeredBelow, uicoltab_ );
    applybut->activated.notify( mCB(this,uiODEditAttribColorDlg,doApply) );
}


void uiODEditAttribColorDlg::doApply( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    ColTab::Sequence coltab = uicoltab_->colTabSeq();
    const bool autoscale = uicoltab_->autoScale();
    const Interval<float> intv = uicoltab_->getInterval();
    const float cliprate = uicoltab_->getClipRate();
    const bool symm = uicoltab_->getSymmetry();
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
	    obj->triggerSeqChange();
	}
	if ( obj->autoScale()!=autoscale || obj->clipRate()!=cliprate
	  				 || obj->getSymmetry()!=symm )
	{
	    obj->autoscalechange.enable( false );
	    obj->setAutoScale( autoscale );
	    obj->setClipRate( cliprate );
	    obj->setSymmetry( symm );
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

