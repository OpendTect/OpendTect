/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvllist.cc,v 1.3 2010-09-08 07:07:22 cvsbruno Exp $";

#include "uistratlvllist.h"

#include "bufstringset.h"
#include "uimenu.h"
#include "uimsg.h"
#include "stratunitrepos.h"
#include "uistratutildlgs.h"

static const char* sNoLevelTxt      = "--- Empty ---";

uiStratLvlList::uiStratLvlList( uiParent* p )
    : uiLabeledListBox(p,"Markers",false,uiLabeledListBox::AboveMid)
    , unitrepos_(Strat::eUnRepo())
{
    box()->setStretch( 2, 2 );
    box()->setFieldWidth( 10 );
    box()->rightButtonClicked.notify( mCB(this,uiStratLvlList,rClickLvlCB));
    unitrepos_.levelChanged.notify( mCB(this,uiStratLvlList,fill) );

    fill(0);
}


uiStratLvlList::~uiStratLvlList()
{
    unitrepos_.levelChanged.remove( mCB(this,uiStratLvlList,fill) );
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
	unitrepos_.removeLevel( box()->getText() );
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
	    unitrepos_.removeAllLevels();
    }
}


void uiStratLvlList::fill( CallBacker* )
{
    box()->empty();
    BufferStringSet lvlnms;
    TypeSet<Color> lvlcolors;
    unitrepos_.getLvlsPars( lvlnms, lvlcolors );
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	box()->addItem( lvlnms[idx]->buf(), lvlcolors[idx] );

    if ( !lvlnms.size() )
	box()->addItem( sNoLevelTxt );
}


void uiStratLvlList::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this );
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
    unitrepos_.getLvlPars( lvlidx, lvlnm, lvlcol );
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


