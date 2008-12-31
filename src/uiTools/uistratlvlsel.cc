/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlvlsel.cc,v 1.11 2008-12-31 13:10:12 cvsbert Exp $";

#include "uistratlvlsel.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrattreewin.h"


static const char* sNoLevelTxt	= "--Undefined--";
static const char* sTiedToTxt	= "Tied to Level";

static void getLvlNms( BufferStringSet& bss )
{
    bss.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::RT().nrLevels(); idx++ )
	bss.add( Strat::RT().levelFromIdx(idx)->name() );
}


uiStratLevelSel::uiStratLevelSel( uiParent* p, bool wlbl, bool wdefine )
    : uiGroup(p)
    , levelChanged(this)
{
    BufferStringSet bss; getLvlNms( bss );

    uiObject* attachobj = 0;
    if ( wlbl )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, bss, sTiedToTxt,
							sTiedToTxt );
	lvlnmfld_ = lcb->box();
	attachobj = lcb->attachObj();
    }
    else
    {
	lvlnmfld_ = new uiComboBox( this, bss, sTiedToTxt );
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

    StratTWin().levelCreated.notify( mCB(this,uiStratLevelSel,lvlModif) );
    StratTWin().levelChanged.notify( mCB(this,uiStratLevelSel,lvlModif) );
    StratTWin().levelRemoved.notify( mCB(this,uiStratLevelSel,lvlModif) );

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
    StratTWin().popUp();
}


void uiStratLevelSel::lvlModif( CallBacker* )
{
    const Strat::Level* cursel = selectedLevel();
    lvlnmfld_->empty();
    BufferStringSet bss; getLvlNms( bss );
    lvlnmfld_->addItems( bss );
    lvlnmfld_->setCurrentItem( cursel ? cursel->name().buf() : sNoLevelTxt );
}
