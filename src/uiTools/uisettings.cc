/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id: uisettings.cc,v 1.2 2002-01-04 15:50:49 nanne Exp $
________________________________________________________________________

-*/

#include "uisettings.h"
#include "settings.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"


uiSettings::uiSettings( uiParent* p, const char* nm )
	: uiDialog(p,nm,true,true,false,false,false,"0.2.1")
	, setts(*new Settings)
	, items("Item")
	, curidx(0)
{
    setTitleText( "Specify GDI settings" );

    for ( int idx=0; idx<setts.size(); idx++ )
    {
	const char* nm = setts.getKey(idx);
	if ( !strchr(nm,'.') ) items.add( nm );
    }

    itemfld = new uiLabeledComboBox( this, "Item", "", true );
    itemfld->box()->addItems( items );
    itemfld->box()->setCurrentItem( 0 );
    itemfld->box()->selectionChanged.notify( mCB(this,uiSettings,itemSel) );

    valfld = new uiGenInput( this, "", StringInpSpec() );
    valfld->attach( alignedBelow, itemfld );

    ynfld = new uiGenInput( this, "", BoolInpSpec() );
    ynfld->attach( alignedBelow, itemfld );

    uiLabel* lbl = new uiLabel( this, "" );
    lbl->attach( alignedBelow, valfld );

    setNew();
}


void uiSettings::itemSel( CallBacker* )
{
    commitPrev();
    setNew();
}


void uiSettings::commitPrev()
{
    const char* keystr = (const char*)items[curidx]->name();
    const char* valstr = setts[keystr];
    if ( !valstr || !*valstr ) 
    { 
	pErrMsg("key not in iopar"); 
	setts.set( keystr, valfld->text() );
	return;
    }

    bool isyn = caseInsensitiveEqual( valstr, "yes", 0 )
	     || caseInsensitiveEqual( valstr, "no", 0 );
    if ( isyn )
	setts.set( keystr, getYesNoString(ynfld->getBoolValue()) );
    else
	setts.set( keystr, valfld->text() );
}


void uiSettings::setNew()
{
    curidx = itemfld->box()->currentItem();
    const char* keystr = (const char*)items[curidx]->name();
    const char* valstr = setts[keystr];
    if ( !valstr || !*valstr ) 
    { 
	pErrMsg("key not in iopar"); 
	valfld->setText( valstr ); 
	return;
    }

    bool isyn = caseInsensitiveEqual( valstr, "yes", 0 )
	     || caseInsensitiveEqual( valstr, "no", 0 );
    if ( isyn )
	ynfld->setValue( *valstr == 'y' || *valstr == 'Y' );
    else
	valfld->setText( valstr );

    ynfld->display( isyn );
    valfld->display( !isyn );
}


bool uiSettings::acceptOK( CallBacker* )
{
    commitPrev();

    if ( !setts.write() )
    {
	uiMSG().error( "Cannot write settings" );
	return false;
    }

    return true;
}
