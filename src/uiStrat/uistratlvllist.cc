/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvllist.cc,v 1.1 2010-08-05 11:50:33 cvsbruno Exp $";

#include "uistratlvllist.h"

#include "bufstringset.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistratutildlgs.h"
#include "uistratmgr.h"

static const char* sNoLevelTxt      = "--- Empty ---";

uiStratLvlList::uiStratLvlList( uiParent* p, uiStratMgr& mgr )
    : uiLabeledListBox(p,"Markers",false,uiLabeledListBox::AboveMid)
    , uistratmgr_(mgr)
{
    box()->setStretch( 2, 2 );
    box()->setFieldWidth( 10 );
    box()->rightButtonClicked.notify( mCB(this,uiStratLvlList,rClickLvlCB));
    uistratmgr_.lvlChanged.notify( mCB(this,uiStratLvlList,fill) );

    fill(0);
}


uiStratLvlList::~uiStratLvlList()
{
    uistratmgr_.lvlChanged.remove( mCB(this,uiStratLvlList,fill) );
}


void uiStratLvlList::rClickLvlCB( CallBacker* )
{
    int curit = box()->currentItem();
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Create &New ..."), 0 );
    if ( curit>-1 && !box()->isPresent( sNoLevelTxt ) )
    {
	mnu.insertItem( new uiMenuItem("&Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("&Remove"), 2 );
	mnu.insertItem( new uiMenuItem("&Remove All"), 3 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>3 ) return;
    if ( mnuid != 2 && mnuid != 3 )
	editLevel( mnuid ? false : true );
    else if ( mnuid == 2 )
    {
	uistratmgr_.removeLevel( box()->getText() );
	box()->removeItem( box()->currentItem() );
	if ( box()->isEmpty() )
	    box()->addItem( sNoLevelTxt );
    }
    else if ( mnuid == 3 )
    {
	BufferString msg ( "This will remove all the stratigraphic markers " ); 
	msg += "present in the list";
	msg += ", do you want to continue ?";
	if ( uiMSG().askGoOn(msg) )
	    uistratmgr_.removeAllLevels();
    }
}


void uiStratLvlList::fill( CallBacker* )
{
    box()->empty();
    BufferStringSet lvlnms;
    TypeSet<Color> lvlcolors;
    uistratmgr_.getLvlsProps( lvlnms, lvlcolors );
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	box()->addItem( lvlnms[idx]->buf(), lvlcolors[idx] );

    if ( !lvlnms.size() )
	box()->addItem( sNoLevelTxt );
}


void uiStratLvlList::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this, &uistratmgr_ );
    if ( !create )
	newlvldlg.setLvlInfo( box()->getText() );
     newlvldlg.go();
}


void uiStratLvlList::update( bool create )
{
    if ( create && box()->isPresent( sNoLevelTxt ) )
	box()->removeItem( 0 );

    BufferString lvlnm;
    Color lvlcol;
    int lvlidx = create ? box()->size() : box()->currentItem();
    uistratmgr_.getLvlProps( lvlidx, lvlnm, lvlcol );
    if ( create )
    {
	box()->addItem( lvlnm, lvlcol );
    }
    else
    {
	box()->setItemText( lvlidx, lvlnm );
	box()->setPixmap( lvlidx, lvlcol );
    }
}


