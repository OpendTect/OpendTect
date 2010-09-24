/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : August 2010
-*/

static const char* rcsID = "$Id Exp $";

#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"

#include "plugins.h"
#include "googletranslator.h"
#include "texttranslator.h"


mExternC int GetGoogleTranslatePluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetGoogleTranslatePluginInfo()
{
    static PluginInfo retpi = {
	"Google Translate",
	"dGB",
	"=od",
	"Translate Gui text with Google Translate" };
    return &retpi;
}


class GoogleTranslateMgr : public CallBacker
{
public:

    			GoogleTranslateMgr(uiODMain&);


    uiODMain&		appl_;

protected:

    void		fillMenu();
    void		handleMenu(CallBacker*);

    ObjectSet<uiMenuItem> langitems_;
    uiMenuItem*		enabitm_;

};


GoogleTranslateMgr::GoogleTranslateMgr( uiODMain& a )
    : appl_(a)
{
    fillMenu();
}


void GoogleTranslateMgr::fillMenu()
{
    uiPopupMenu* trmenu = new uiPopupMenu( &appl_, "T&ranslate" );
    appl_.menuMgr().utilMnu()->insertItem( trmenu );

    CallBack cb( mCB(this,GoogleTranslateMgr,handleMenu) );
    enabitm_ = new uiMenuItem( "Enable", cb );
    trmenu->insertItem( enabitm_, 0 );
    enabitm_->setCheckable( true );
    enabitm_->setChecked( false );

    uiPopupMenu* langmnu = new uiPopupMenu( &appl_, "&Language" );
    trmenu->insertItem( langmnu );
    const int nrlangs = TrMgr().tr()->nrSupportedLanguages();
    for ( int idx=0; idx<nrlangs; idx++ )
    {
	uiMenuItem* itm = new uiMenuItem( "", cb );
	langmnu->insertItem( itm, idx+1 );
	itm->setText( TrMgr().tr()->getLanguageUserName(idx) );
	itm->setCheckable( true );
	langitems_ += itm;
    }
}


void GoogleTranslateMgr::handleMenu( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return;

    if ( itm->id() == 0 )
	TrMgr().tr()->enable();
    else
    {
	const int langidx = itm->id() - 1;
	TrMgr().tr()->setToLanguage( TrMgr().tr()->getLanguageName(langidx) );
	appl_.translate();
	for ( int idx=0; idx<langitems_.size(); idx++ )
	    langitems_[idx]->setChecked( idx==langidx );
    }
}



mExternC const char* InitGoogleTranslatePlugin( int, char** )
{
    TrMgr().setTranslator( new GoogleTranslator );

    static GoogleTranslateMgr* mgr = 0;
    if ( !mgr )
	mgr = new GoogleTranslateMgr( *ODMainWin() );

    return 0;
}
