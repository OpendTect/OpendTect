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
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uilabel.h"
#include "uimsg.h"
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

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();

    if ( attrnm && *attrnm )
    {
	uiString titletext = tr("Color Settings: %1").arg( attrnm );
	setTitleText( titletext );
    }

    const ColTab::Sequence* colseq = 0;
    const ColTab::Mapper* mapper = 0;
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,item,items_[idx])
	if ( !item )
	    continue;

	const int did = item->displayID();
	const int anr = item->attribNr();
	colseq = &visserv->getColTabSequence( did, anr );
	mapper = &visserv->getColTabMapper( did, anr );
	break;
    }
    if ( !colseq )
	{ new uiLabel( this, tr("No defined attributes") ); return; }


    coltabsel_ = new uiColTabSel( this, OD::Vertical );
    coltabsel_->setSequence( *colseq );
    coltabsel_->setMapper( const_cast<ColTab::Mapper&>(*mapper) );
    coltabsel_->seqChanged.notify( mCB(this,uiODEditAttribColorDlg,seqChg) );
    mapper->setup().objectChanged().notify(
			mCB(this,uiODEditAttribColorDlg,mapperChg));
}


void uiODEditAttribColorDlg::seqChg( CallBacker* )
{
    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    const ColTab::Sequence& newcolseq = coltabsel_->sequence();

    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,attritem,items_[idx])
	if ( attritem )
	    attritem->attribProbeLayer()->setSequence( newcolseq );
    }
}


void uiODEditAttribColorDlg::mapperChg( CallBacker* )
{
    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );

    const ColTab::Mapper& mapper = coltabsel_->mapper();
    for ( int idx=0; idx<items_.size(); idx++ )
    {
	mDynamicCastGet(uiODAttribTreeItem*,attritem,items_[idx])
	if ( !attritem )
	    continue;

	attritem->attribProbeLayer()->mapper().setup() = mapper.setup();
    }
}


bool uiODEditAttribColorDlg::acceptOK()
{
    return true;
}
