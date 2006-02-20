/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          December 2005
 RCS:           $Id: uishortcuts.cc,v 1.5 2006-02-20 18:49:49 cvsbert Exp $
________________________________________________________________________

-*/


#include "uishortcuts.h"
#include "uihandleshortcuts.h"
#include "keyenum.h"
#include "uitable.h"
#include "userinputobj.h"
#include "oddirs.h"
#include "settings.h"


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


#define mRC(row,col) RowCol(row,col)

#define mFillRow(rowidx,text,val1,val2)\
	shortcutskeys_->setText( mRC(rowidx, 0), text );\
	uiobj1 = shortcutskeys_->mkUsrInputObj( mRC(rowidx,1) );\
	uiobj1->addItem( "NoButton" );\
	uiobj1->addItem( "ShiftButton" );\
	uiobj1->addItem( "ControlButton" );\
	uiobj1->setValue( val1 );\
	uiobj2 = shortcutskeys_->mkUsrInputObj( mRC(rowidx,2) );\
	uiobj2->addItems( OD::KeyDef().names );\
	uiobj2->setValue( val2 );


void uiShortcutsDlg::fillTable()
{
    UserInputObj* uiobj1;
    UserInputObj* uiobj2;
    bool isdefault = readFileValues();
    const int tablesz = uiHandleShortcuts::SCLabelsDef().size();
    int val1, val2;
    for ( int idx=0; idx<tablesz; idx++ )
    {
	getKeyValues( idx, val1, val2, isdefault );
	if ( val1 == 5 )
	    val1 = 1;//remember that we don't use the entire ButtonState list
	if ( val1 == 6 )
	    val1 = 2;
	mFillRow(idx,uiHandleShortcuts::SCLabelsDef().convert(idx),val1,val2);
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
    if ( keyidx == 1 )
    {
	if ( !strcmp( shortcutskeys_->text(mRC(scutidx,keyidx)),"ShiftButton") )
	    return getEnum( "ShiftButton", OD::ButtonStateDef().names, 0, 0 );
	else if ( !strcmp( shortcutskeys_->text(mRC(scutidx,keyidx)),
							"ControlButton") )
	    return getEnum( "ControlButton", OD::ButtonStateDef().names, 0, 0 );
    }
    
    return shortcutskeys_->getIntValue( mRC(scutidx,keyidx) );
}


void uiShortcutsDlg::shortcutsDlgClosed( CallBacker* )
{
    if ( uiResult() == 0 )
	return;

    fillPar();
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

