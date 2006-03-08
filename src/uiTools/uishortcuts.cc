/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          December 2005
 RCS:           $Id: uishortcuts.cc,v 1.7 2006-03-08 13:35:43 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uishortcuts.h"

#include "uicombobox.h"
#include "uihandleshortcuts.h"
#include "uitable.h"
#include "keyenum.h"
#include "oddirs.h"
#include "settings.h"


static const char* sButtonStrs[] =
{ "NoButton", "ShiftButton", "ControlButton", 0 };


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
    shortcutskeys_->setNrRows( uiHandleShortcuts::SCLabelsDef().size() );
    fillTable();
    windowClosed.notify( mCB(this,uiShortcutsDlg, shortcutsDlgClosed) );
}


void uiShortcutsDlg::fillTable()
{
    UserInputObj* uiobj1;
    UserInputObj* uiobj2;
    const bool isdefault = readFileValues();
    const int tablesz = uiHandleShortcuts::SCLabelsDef().size();
    int val1, val2;
    for ( int idx=0; idx<tablesz; idx++ )
    {
	getKeyValues( idx, val1, val2, isdefault );
	if ( val1 == 5 )
	    val1 = 1;//remember that we don't use the entire ButtonState list
	if ( val1 == 6 )
	    val1 = 2;

	shortcutskeys_->setText( RowCol(idx,0),
				uiHandleShortcuts::SCLabelsDef().convert(idx) );
	uiComboBox* box1 = new uiComboBox(0);
	shortcutskeys_->setCellObject( RowCol(idx,1), box1 );
	box1->addItems( sButtonStrs );
	box1->setCurrentItem( val1 );

	uiComboBox* box2 = new uiComboBox(0);
	shortcutskeys_->setCellObject( RowCol(idx,2), box2 );
	box2->addItems( OD::KeyDef().names );
	box2->setCurrentItem( val2 );
    }
}


bool uiShortcutsDlg::readFileValues()
{
    BufferString firstsc;
    if ( !mSettUse(get,"Shortcuts.0","Name",firstsc) )
    {
	pars_.read( GetDataFileName("ShortCuts"),
		    uiHandleShortcuts::sKeyFileType );
	return true;
    }
    else
	pars_ = Settings::common();

    return false;
}


#define mIndexOf(KeySet, keystring, val)\
val = 0;\
for ( int idx=0; idx<KeySet##Def().size(); idx++ )\
{\
    if ( !strcmp( KeySet##Def().convert(idx), keystring.buf() ) )\
    { val = idx; break; }\
}\



void uiShortcutsDlg::getKeyValues( int scutidx, int& val1, 
				   int& val2, bool isdefault ) const
{
    BufferString c1,c2;
    BufferString scutidxstr = isdefault ? "" : "Shortcuts.";
    scutidxstr += scutidx;
    BufferString key = IOPar::compKey( scutidxstr, uiHandleShortcuts::keyStr());
    pars_.get( key.buf(), c1, c2 ); 
    mIndexOf( OD::ButtonState, c1, val1 );
    mIndexOf( OD::Key, c2, val2 );
}


int uiShortcutsDlg::getUIValue( int keyidx, int scutidx ) const
{
    mDynamicCastGet(uiComboBox*,box,
		    shortcutskeys_->getCellObject(RowCol(scutidx,keyidx)))
    if ( keyidx == 1 )
    {
	BufferString keytxt = box->text();
	if ( keytxt == "ShiftButton" )
	    return getEnum( "ShiftButton", OD::ButtonStateDef().names, 0, 0 );
	else if ( keytxt == "ControlButton" )
	    return getEnum( "ControlButton", OD::ButtonStateDef().names, 0, 0 );
    }

    return box->currentItem();
}


void uiShortcutsDlg::shortcutsDlgClosed( CallBacker* )
{
    if ( uiResult() == 0 )
	return;

    fillPar();
    SCList().init();
}


void uiShortcutsDlg::fillPar() const
{
    const int tablesz = uiHandleShortcuts::SCLabelsDef().size();
    
    for ( int idx=0; idx<tablesz; idx++ )
    {
	BufferString basekey = IOPar::compKey( "Shortcuts", idx );
	mSettUse(set,basekey,uiHandleShortcuts::nameStr(),
		 uiHandleShortcuts::SCLabelsDef().convert(idx) );
	Settings::common().set( IOPar::compKey(basekey,
		    		uiHandleShortcuts::keyStr()),
				OD::ButtonStateDef().convert(getUIValue(1,idx)),
				OD::KeyDef().convert( getUIValue(2,idx) ) );
    }
    Settings::common().write();
}

