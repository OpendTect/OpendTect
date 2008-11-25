/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvlsel.cc,v 1.10 2008-11-25 15:35:26 cvsbert Exp $";

#include "uistratlvlsel.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrattreewin.h"


static const char* sNoLevelTxt	= "--Undefined--";

uiStratLevelSel::uiStratLevelSel( uiParent* p, bool wlbl, bool wdefine )
    : uiGroup(p)
    , levelChanged(this)
{
    BufferStringSet bfset;
    bfset.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::RT().nrLevels(); idx++ )
	bfset.add( Strat::RT().levelFromIdx(idx)->name() );

    uiObject* attachobj = 0;
    if ( wlbl )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, bfset,
					         	"Tied to level",
							 "Tied to level" );
	lvlnmfld_ = lcb->box();
	attachobj = lcb->attachObj();
    }
    else
    {
	lvlnmfld_ = new uiComboBox( this, bfset, "Tied to level" );
	lvlnmfld_->setStretch( 2, 2 );
	attachobj = lvlnmfld_;
    }
    lvlnmfld_->selectionChanged.notify( mCB(this,uiStratLevelSel,selLvlCB) );

    if ( wdefine )
    {
	uiPushButton* deflvlbut = new uiPushButton( this, "&Define Level",
			mCB(this,uiStratLevelSel,defineLvlCB), true );
	deflvlbut->setHSzPol( uiPushButton::Medium );
	if ( attachobj )
	    deflvlbut->attach( rightOf, attachobj );
    }

    setHAlignObj( lvlnmfld_ );
}


const Strat::Level* uiStratLevelSel::selectedLevel() const
{
    const int selidx = lvlnmfld_->currentItem();
    if ( selidx == 0 ) return 0;
    const Strat::Level* lvl = Strat::RT().getLevel( lvlnmfld_->text() );
    return lvl;
}


const Color* uiStratLevelSel::getLevelColor() const
{
    const Strat::Level* lvl = selectedLevel();
    return lvl ? &lvl->color_ : 0;
}


int uiStratLevelSel::getLevelID() const
{
    const Strat::Level* lvl = selectedLevel();
    return lvl ? lvl->id_ : -1;
}


void uiStratLevelSel::setLevelID( int id )
{
    const Strat::Level* lvl = Strat::RT().levelFromID( id );
    if ( !lvl )
	lvlnmfld_->setCurrentItem( (int)0 );
    else
	lvlnmfld_->setCurrentItem( lvl->name() );
}


void uiStratLevelSel::selLvlCB( CallBacker* )
{
    levelChanged.trigger();
}


void uiStratLevelSel::defineLvlCB( CallBacker* )
{
    const_cast<uiStratTreeWin&>(StratTWin()).show();
}
