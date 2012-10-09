/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	R. K. Singh
 Date: 		Jan 2008
___________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiodeditattribcolordlg.h"

#include "coltab.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
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
    : uiDialog(p,uiDialog::Setup("Color Settings",mNoDlgTitle,"50.1.5"))
    , items_(set)
    , uicoltab_( 0 )
{
    setCtrlStyle( LeaveOnly );

    if ( attrnm && *attrnm )
    {
	BufferString titletext = "Color Settings: ";
	titletext += attrnm;
	setTitleText( titletext );
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();

    const ColTab::Sequence* colseq = 0;
    const ColTab::MapperSetup* colmapsetup = 0;
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int did = item->displayID();
	const int anr = item->attribNr();
	colseq = visserv->getColTabSequence( did, anr );
	if ( !colseq ) continue;

	colmapsetup = visserv->getColTabMapperSetup( did, anr );
	if ( !colmapsetup ) continue;

	break;
    }

    if ( !colseq || !colmapsetup )
    {
	pErrMsg( "Something is not as it should" );
	return;
    }

    uicoltab_ = new uiColorTable( this, *colseq, true );
    uicoltab_->setMapperSetup( colmapsetup );
    uicoltab_->seqChanged.notify( mCB(this,uiODEditAttribColorDlg,seqChg) );
    uicoltab_->scaleChanged.notify( mCB(this,uiODEditAttribColorDlg,mapperChg));
}


void uiODEditAttribColorDlg::seqChg( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const ColTab::Sequence& newcolseq = uicoltab_->colTabSeq();

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int did = item->displayID();
	const int anr = item->attribNr();
	const ColTab::Sequence* colseq = visserv->getColTabSequence( did, anr );
	if ( colseq && *colseq!=newcolseq && visserv->canSetColTabSequence(did))
	    visserv->setColTabSequence( did, anr, newcolseq );

	items_[idx]->updateColumnText( uiODSceneMgr::cColorColumn() );
    }
}


void uiODEditAttribColorDlg::mapperChg( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const ColTab::MapperSetup& newcolmapsetup = uicoltab_->colTabMapperSetup();

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int did = item->displayID();
	const int anr = item->attribNr();
	const ColTab::MapperSetup* colmapsetup =
	    visserv->getColTabMapperSetup( did, anr );
	if ( colmapsetup && *colmapsetup!=newcolmapsetup )
	    visserv->setColTabMapperSetup( did, anr, newcolmapsetup );
    }
}


bool uiODEditAttribColorDlg::acceptOK( CallBacker* )
{
    return true;
}
