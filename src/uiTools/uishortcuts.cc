/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          December 2005
 RCS:           $Id: uishortcuts.cc,v 1.9 2006-05-08 07:06:31 cvshelene Exp $
________________________________________________________________________

-*/


#include "uishortcuts.h"

#include "uicombobox.h"
#include "uishortcutsmgr.h"
#include "uitable.h"
#include "keyenum.h"
#include "keystrs.h"
#include "oddirs.h"
#include "settings.h"


static const char* sSCNamesList[] =
{ "Move slice forward", "Move slice backward", 0 };

static const char* sButtonStrs[] =
{ "NoButton", "ShiftButton", "ControlButton", 0 };

static const char* sKeyStrs[] =
{ "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S",
  "T","U","V","W","X","Y","Z","Plus","Minus","Asterisk","Slash","Up","Down",
  "Left","Right","Delete","PageUp","PageDown", 0 };


#define mArraySize( arrayname,size )\
{\
    size = 0;\
    while ( arrayname[size] != 0 )\
    	size++;\
}\

uiShortcutsDlg::uiShortcutsDlg( uiParent* p )
    : uiDialog( p,uiDialog::Setup( "Set up shortcuts", "", "0.2.4" ) )
{
    BufferString title( "Select keys used as shortcuts" );
    setTitleText( title );

    shortcutskeys_ = new uiTable( this, uiTable::Setup().selmode(
						uiTable::SelectionMode(3) ) );
    shortcutskeys_->setNrCols( 3 );
    shortcutskeys_->setColumnStretchable( 0, true );
    shortcutskeys_->setColumnWidth( 0, 35 );
    shortcutskeys_->setColumnLabel( 0, "action" );
    shortcutskeys_->setColumnLabel( 1, "key 1" );
    shortcutskeys_->setColumnLabel( 2, "key 2" );
    int nrrows;
    mArraySize(sSCNamesList,nrrows);
    shortcutskeys_->setNrRows( nrrows );
    fillTable();
}


void uiShortcutsDlg::fillTable()
{
    UserInputObj* uiobj1;
    UserInputObj* uiobj2;
    IOPar* pars = SCMgr().readShortcutsFile( uiShortcutsList::ODSceneStr() );
    if ( !pars ) return;
    
    int tablesz;
    mArraySize(sSCNamesList,tablesz);
    int val1, val2;
    for ( int idx=0; idx<tablesz; idx++ )
    {
	getKeyValues( idx, val1, val2, pars );

	shortcutskeys_->setText( RowCol(idx,0), sSCNamesList[idx] );
	uiComboBox* box1 = new uiComboBox(0);
	shortcutskeys_->setCellObject( RowCol(idx,1), box1 );
	box1->addItems( sButtonStrs );
	box1->setCurrentItem( val1 );

	uiComboBox* box2 = new uiComboBox(0);
	shortcutskeys_->setCellObject( RowCol(idx,2), box2 );
	box2->addItems( sKeyStrs );
	box2->setCurrentItem( val2 );
    }

    delete pars;
}


#define mIndexOf(KeySet, keystring, val)\
{\
    val = 0;\
    int size;\
    mArraySize(KeySet,size);\
    for ( int idx=0; idx<size; idx++ )\
    {\
	if ( !strcmp( KeySet[idx], keystring.buf() ) )\
	{ val = idx; break; }\
    }\
}\



void uiShortcutsDlg::getKeyValues( int scutidx, int& val1, int& val2, 
				   IOPar* pars ) const
{
    BufferString c1,c2;
    BufferString scutidxstr = scutidx;
    BufferString key = IOPar::compKey( scutidxstr, sKey::Keys );
    pars->get( key.buf(), c1, c2 ); 
    mIndexOf( sButtonStrs, c1, val1 );
    mIndexOf( sKeyStrs, c2, val2 );
}


int uiShortcutsDlg::getUIValue( int keyidx, int scutidx ) const
{
    mDynamicCastGet(uiComboBox*,box,
		    shortcutskeys_->getCellObject(RowCol(scutidx,keyidx)))

    return box->currentItem();
}


bool uiShortcutsDlg::acceptOK( CallBacker* )
{
    writeToSettings();
    uiShortcutsList* list = SCMgr().getList( uiShortcutsList::ODSceneStr() );
    list->init(uiShortcutsList::ODSceneStr());

    return true;
}


void uiShortcutsDlg::writeToSettings()
{
    BufferString basekey = 
	    IOPar::compKey( sKey::Shortcuts, uiShortcutsList::ODSceneStr() );
    for ( int idx=0; idx<shortcutskeys_->nrRows(); idx++ )
    {
	BufferString key = IOPar::compKey( basekey, idx );
	mSettUse(set,key,sKey::Name, sSCNamesList[idx] );
	Settings::common().set( IOPar::compKey(key, sKey::Keys),
				sButtonStrs[getUIValue(1,idx)],
				sKeyStrs[getUIValue(2,idx)] );
    }
    removeShortcutsOldStyle(); //compat with old files...
    Settings::common().write( false );
}


void uiShortcutsDlg::removeShortcutsOldStyle()
{
    IOPar& par = Settings::common();
    BufferString basekey = IOPar::compKey( sKey::Shortcuts, 0 );
    par.removeWithKey( IOPar::compKey(basekey.buf(),sKey::Name) );
    par.removeWithKey( IOPar::compKey(basekey.buf(),sKey::Keys) );
    basekey = IOPar::compKey( sKey::Shortcuts, 1 );
    par.removeWithKey( IOPar::compKey(basekey.buf(),sKey::Name) );
    par.removeWithKey( IOPar::compKey(basekey.buf(),sKey::Keys) );
}

