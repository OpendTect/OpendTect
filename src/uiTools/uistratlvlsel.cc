/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:		$Id: uistratlvlsel.cc,v 1.7 2008-02-20 04:42:44 cvsnanne Exp $
________________________________________________________________________

-*/

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
	bfset.add( Strat::RT().level(idx)->name() );

    uiObject* attachobj = 0;
    if ( wlbl )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this, bfset,
							"Tied to level" );
	lvlnmfld_ = lcb->box();
	attachobj = lcb->attachObj();
    }
    else
    {
	lvlnmfld_ = new uiComboBox( this, bfset );
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


void uiStratLevelSel::defineLvlCB( CallBacker* )
{
    const_cast<uiStratTreeWin&>(StratTWin()).show();
}


const char* uiStratLevelSel::getLvlName() const
{
    return strcmp(sNoLevelTxt,lvlnmfld_->text()) ? lvlnmfld_->text() : 0;
}


const Color* uiStratLevelSel::getLvlColor() const
{
    if ( !strcmp(sNoLevelTxt,lvlnmfld_->text()) )
	return 0;

    const Strat::Level* lvl = Strat::RT().getLevel( lvlnmfld_->text() );
    if ( !lvl ) return 0;

    return &lvl->color_;
}


void uiStratLevelSel::selLvlCB( CallBacker* )
{
    levelChanged.trigger();
}
