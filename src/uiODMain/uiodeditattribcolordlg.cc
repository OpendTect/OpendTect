/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		Jan 2008
___________________________________________________________________

-*/

#include "uiodeditattribcolordlg.h"

#include "attribprobelayer.h"
#include "coltab.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "uicoltabsel.h"
#include "uibutton.h"
#include "mousecursor.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"
#include "od_helpids.h"


uiODEditAttribColorDlg::uiODEditAttribColorDlg( uiParent* p,
						ObjectSet<uiTreeItem>& set,
						const char* attrnm )
    : uiDialog(p,uiDialog::Setup(tr("Color Settings"),mNoDlgTitle,
                                 mODHelpKey(mODEditAttribColorDlgHelpID) ))
    , items_(set)
    , coltabsel_( 0 )
{
    setCtrlStyle( CloseOnly );

    if ( attrnm && *attrnm )
    {
	uiString titletext = tr("Color Settings: %1").arg( attrnm );
	setTitleText( titletext );
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();

    const ColTab::Sequence* colseq = 0;
    ConstRefMan<ColTab::MapperSetup> colmapsetup;
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
	{ pErrMsg( "Something is not as it should" ); return; }

    coltabsel_ = new uiColTabSel( this, OD::Vertical );
    coltabsel_->setSeqName( colseq->name() );
    coltabsel_->useMapperSetup( *colmapsetup );
    coltabsel_->seqChanged.notify( mCB(this,uiODEditAttribColorDlg,seqChg) );
    coltabsel_->mapperSetupChanged.notify(
			mCB(this,uiODEditAttribColorDlg,mapperSetupChg));
}


void uiODEditAttribColorDlg::seqChg( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    ConstRefMan<ColTab::Sequence> newcolseq = coltabsel_->sequence();

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,attritem,items_[idx])
	if ( !attritem )
	    attritem->attribProbeLayer()->setColSeq( newcolseq );
    }
}


void uiODEditAttribColorDlg::mapperSetupChg( CallBacker* )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    ConstRefMan<ColTab::MapperSetup> newcolmapsetup = coltabsel_->mapperSetup();
    if ( !newcolmapsetup )
	return;

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,attritem,items_[idx])
	if ( !attritem )
	    continue;

	attritem->attribProbeLayer()->setMapperSetup( *newcolmapsetup );
    }
}


bool uiODEditAttribColorDlg::acceptOK()
{
    return true;
}
