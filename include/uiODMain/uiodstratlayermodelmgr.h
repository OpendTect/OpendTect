#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		Jan 2017
________________________________________________________________________

-*/

#include "uistratlayermodel.h"

#include "dbman.h"
#include "separstr.h"
#include "settings.h"
#include "stratlayseqgendesc.h"
#include "stratlayermodel.h"
#include "stratlaygen.h"
#include "strattransl.h"
#include "stratlaymodgen.h"
#include "stratreftree.h"
#include "stratsynth.h"
#include "uichecklist.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiselsimple.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratlaymodtools.h"
#include "uistratsimplelaymoddisp.h"
#include "uistratsynthcrossplot.h"
#include "uistrattreewin.h"
#include "uistrings.h"


class uiStratLayerModelManager : public CallBacker
{ mODTextTranslationClass(uiStratLayerModelManager);
public:

uiStratLayerModelManager()
    : dlg_(0)
{
    DBM().surveyToBeChanged.notify(mCB(this,uiStratLayerModelManager,survChg));
    DBM().applicationClosing.notify(mCB(this,uiStratLayerModelManager,survChg));
}


void survChg( CallBacker* )
{
    if ( dlg_ )
	dlg_->saveGenDescIfNecessary( false );
    delete dlg_; dlg_ = 0;
}

void winClose( CallBacker* )
{
    uiStratTreeWin::makeEditable( true );
    dlg_ = 0;
}

void startCB( CallBacker* cb )
{
    if ( haveExistingDlg() )
	return;
    const uiStringSet& usrnms =
			uiLayerSequenceGenDesc::factory().getUserNames();
    const BufferStringSet& nms = uiLayerSequenceGenDesc::factory().getNames();
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( Strat::RT().isEmpty() || nms.isEmpty() )
	{ pErrMsg("Pre-condition not met"); return; }

    uiParent* par = tb ? tb->parent() : ODMainWin();
    const char* settres = Settings::common().find(
				uiStratLayerModel::sKeyModeler2Use());
    BufferString modnm( settres );
    int defmodnr = -1;
    bool givechoice = nms.size() > 1;
    if ( modnm.isEmpty() )
	modnm = *nms.last();
    else
    {
	FileMultiString fms( modnm );
	modnm = fms[0];
	defmodnr = nms.indexOf( modnm.buf() );
	if ( defmodnr < 0 )
	    modnm.setEmpty();
	else
	{
	    const bool alwayswant = fms.size() > 1 && *fms[1] == 'A';
	    givechoice = givechoice && !alwayswant;
	}
    }

    if ( givechoice )
    {
	uiSelectFromList::Setup sflsu( tr("Select modeling type"), usrnms );
	sflsu.current( defmodnr < 0 ? nms.size()-1 : defmodnr );
	uiSelectFromList dlg( par, sflsu );
	uiCheckList* defpol = new uiCheckList( &dlg, uiCheckList::Chain1st,
						OD::Horizontal );
	defpol->addItem( tr("Set as default") )
	       .addItem( tr("Always use this type") );
	defpol->setChecked( 0, defmodnr >= 0 );
	defpol->attach( centeredBelow, dlg.selFld() );
	if ( !dlg.go() ) return;

	const int sel = dlg.selection();
	if ( sel < 0 ) return;
	const BufferString newmodnm = nms.get( sel );
	int indic = defpol->isChecked(0) ? (defpol->isChecked(1) ? 2 : 1) : 0;
	bool needwrite = true;
	if ( indic == 0 )
	{
	    if ( defmodnr < 0 )
		needwrite = false;
	    else
		Settings::common().removeWithKey(
				    uiStratLayerModel::sKeyModeler2Use() );
	}
	else
	{
	    if ( indic == 2 || defmodnr < 0 || modnm != newmodnm )
	    {
		Settings::common().set( uiStratLayerModel::sKeyModeler2Use(),
			BufferString(newmodnm, indic == 2 ? "`Always" : "") );
	    }
	    else if ( defmodnr >= 0 )
		needwrite = false;
	}

	if ( needwrite )
	    Settings::common().write( false );
	modnm = newmodnm;
    }
    doLayerModel( modnm, 0 );
}

bool haveExistingDlg()
{
    if ( dlg_ )
    {
	uiMSG().error(tr("Please exit your other layer modeling window first"));
	dlg_->raise();
	return true;
    }
    return false;
}

void launchLayerModel( const char* modnm, int opt )
{
    if ( haveExistingDlg() || Strat::RT().isEmpty() )
	return;

    dlg_ = new uiStratLayerModel( ODMainWin(), modnm, opt );
    if ( !dlg_->moddisp_ )
	{ delete dlg_; dlg_ = 0; }
    else
    {
	uiStratTreeWin::makeEditable( false );
	dlg_->windowClosed.notify(mCB(this,uiStratLayerModelManager,winClose));
	dlg_->go();
    }
}

void addToTreeWin()
{
    uiToolButtonSetup* su = new uiToolButtonSetup( "stratlayermodeling",
			    tr("Start layer/synthetics modeling"),
			    mCB(this,uiStratLayerModelManager,startCB) );
    uiStratTreeWin::addTool( su );
}

    uiStratLayerModel*	dlg_;



static uiStratLayerModelManager& uislm_manager()
{
    mDefineStaticLocalObject( uiStratLayerModelManager, theinst, );
    return theinst;
}

static void initClass()
{
    uislm_manager().addToTreeWin();
}


static void doLayerModel( const char* modnm, int opt )
{
    if ( Strat::RT().isEmpty() )
	StratTreeWin().popUp();
    else
	uislm_manager().launchLayerModel( modnm, opt );
}

static void doBasicLayerModel()
{
    doLayerModel( uiBasicLayerSequenceGenDesc::typeStr(), 0 );
}

};
