/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          December 2005
 RCS:           $Id: uishortcuts.cc,v 1.1 2005-12-22 15:57:48 cvshelene Exp $
________________________________________________________________________

-*/


#include "uishortcuts.h"
#include "uitable.h"
#include "userinputobj.h"
#include "oddirs.h"



static const char* sKeys1[] =
{
    "Ctrl",
    "Shift",
    "None",
    0
};


static const char* sKeys2[] =
{
    "Plus",
    "Minus",
    "UpArrow",
    "DownArrow",
    "LeftArrow",
    "RightArrow",
    "None",
    0
};


static const char* sShortcutsLabel[] =
{
    "Move slice forward",
    "Move slice backward",
    0
};


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
    shortcutskeys_->setNrRows( sizeof sShortcutsLabel / sizeof(char*) -1 );
    fillTable();
    windowClosed.notify( mCB(this,uiShortcutsDlg, shortcutsDlgClosed) );
}


#define mRC(row,col) uiTable::RowCol(row,col)

#define mFillRow(rowidx,text,val1,val2)\
	shortcutskeys_->setText( mRC(rowidx, 0), text );\
	uiobj1 = shortcutskeys_->mkUsrInputObj( mRC(rowidx,1) );\
	uiobj1->addItems( sKeys1 );\
	uiobj1->setValue( val1 );\
	uiobj2 = shortcutskeys_->mkUsrInputObj( mRC(rowidx,2) );\
	uiobj2->addItems( sKeys2 );\
	uiobj2->setValue( val2 );


void uiShortcutsDlg::fillTable()
{
    UserInputObj* uiobj1;
    UserInputObj* uiobj2;
    readFileValues();
    int tablesz = sizeof sShortcutsLabel / sizeof(char*) -1;
    int val1, val2;
    for ( int idx=0; idx<tablesz; idx++ )
    {
	getKeyValues(idx, val1, val2); 
	mFillRow(idx,sShortcutsLabel[idx],val1,val2);
    }
}


void uiShortcutsDlg::readFileValues()
{
    pars_.read( GetDataFileName("ShortCuts") );
}


#define mIndexOf(KeySet, keystring, val)\
val = 0;\
for ( int idx=0; idx<sizeof KeySet / sizeof(char*) -1; idx++ )\
{\
    if ( !strcmp( KeySet[idx], keystring.buf() ) )\
    { val = idx; break; }\
}\



void uiShortcutsDlg::getKeyValues( int scutidx, int& val1, int& val2 ) const
{
    BufferString c1,c2;
    BufferString key = IOPar::compKey( keyStr(), scutidx );
    pars_.get( key.buf(), c1, c2 ); 
    mIndexOf( sKeys1, c1, val1 );
    mIndexOf( sKeys2, c2, val2 );
}


int uiShortcutsDlg::getUIValue( int keyidx, int scutidx ) const
{
return shortcutskeys_->getIntValue( mRC(scutidx,keyidx) );
}


void uiShortcutsDlg::shortcutsDlgClosed( CallBacker* )
{
if ( uiResult() == 0 )
    return;

    IOPar scpars;
    fillPar( scpars );
    scpars.dump( GetDataFileName("ShortCuts") );
}


void uiShortcutsDlg::fillPar( IOPar& iopar ) const
{
    BufferString key;
    int tablesz = sizeof sShortcutsLabel / sizeof(char*) -1;
    
    for ( int idx=0; idx<tablesz; idx++ )
    {
	key = IOPar::compKey( nameStr(), idx );
	iopar.set( key, sShortcutsLabel[idx] );
	key = IOPar::compKey( keyStr(), idx );
	iopar.set( key, sKeys1[getUIValue(1,idx)], sKeys2[getUIValue(2,idx)] );
    }
}

