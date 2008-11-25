/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	R. K. Singh
 Date: 		Jan 2008
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodeditattribcolordlg.cc,v 1.13 2008-11-25 15:35:25 cvsbert Exp $";

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
    : uiDialog(p,uiDialog::Setup("Color Settings",mNoDlgTitle,mTODOHelpID))
    , items_(set)
    , itemusedineditor_(-1)
    , uicoltab_( 0 )
{
    setCtrlStyle( LeaveOnly );

    if ( attrnm && *attrnm )
    {
	BufferString titletext = "Color Settings : ";
	titletext += attrnm;
	setTitleText( titletext );
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();

    const ColTab::Sequence* colseq = 0;
    const ColTab::MapperSetup* colmapsetup = 0;
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,item,items_[idx])
	if ( !item ) continue;

	colseq =
	    visserv->getColTabSequence( item->displayID(), item->attribNr() );

	if ( !colseq )
	    continue;

	colmapsetup =
	    visserv->getColTabMapperSetup( item->displayID(), item->attribNr());

	if ( !colmapsetup )
	    continue;

	itemusedineditor_ = idx;
	break;
    }

    if ( !colseq || !colmapsetup )
    {
	pErrMsg( "Something is not as it should" );
	return;
    }

    uicoltab_ = new uiColorTable( this, *colseq, true );
    uicoltab_->setMapperSetup( colmapsetup );
}


void uiODEditAttribColorDlg::doApply( CallBacker* )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    const ColTab::Sequence& newcoltab = uicoltab_->colTabSeq();
    const ColTab::MapperSetup& newcolmapsetup = uicoltab_->colTabMapperSetup();

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,item,items_[idx])
	if ( !item ) continue;

	const int did = item->displayID();
	const int anr = item->attribNr();

	const ColTab::Sequence* colseq = visserv->getColTabSequence( did, anr );

	const ColTab::MapperSetup* colmapsetup =
	    visserv->getColTabMapperSetup( did, anr );

	if ( colseq && *colseq!=newcoltab &&
	     visserv->canSetColTabSequence(did) )
	    visserv->setColTabSequence( did, anr, newcoltab );

	if ( *colmapsetup!=newcolmapsetup )
	    visserv->setColTabMapperSetup( did, anr, newcolmapsetup );

	/*TODO: Raman I don't really understand what's happening here
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
	*/

	items_[idx]->updateColumnText( uiODSceneMgr::cColorColumn() );
    }
}


bool uiODEditAttribColorDlg::acceptOK( CallBacker* )
{
    doApply( 0 );
    return true;
}
