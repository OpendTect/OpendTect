/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:		$Id: uistratlvlsel.cc,v 1.5 2007-09-10 12:55:15 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratlvlsel.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uistrattreewin.h"


static const char* sNoLevelTxt	= "--Undefined--";

uiStratLevelSel::uiStratLevelSel( uiParent* p )
    : uiGroup(p)
    , selchanged_(this)
{
    BufferStringSet bfset;
    fillLvlList( bfset );
    CallBack cb = mCB(this,uiStratLevelSel,selLvlCB);
    lvlnmfld_ = new uiGenInput( this,"Tied to level",StringListInpSpec(bfset) );
    lvlnmfld_->valuechanged.notify( cb );
    CallBack cb2 = mCB(this,uiStratLevelSel,defineLvlCB);
    deflvlbut_ = new uiPushButton( this, "&Define Level", cb2, true );
    deflvlbut_->setHSzPol(uiPushButton::Medium);
    deflvlbut_->attach( rightOf, lvlnmfld_ );
    setHAlignObj( lvlnmfld_ );
}


void uiStratLevelSel::fillLvlList( BufferStringSet& bfset ) const
{
    bfset.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::RT().nrLevels(); idx++ )
	bfset.add( Strat::RT().level(idx)->name() );
}


void uiStratLevelSel::defineLvlCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratLevelSel::defineLvlCB");
}


const char* uiStratLevelSel::getLvlName() const
{
    if ( !strcmp( sNoLevelTxt, lvlnmfld_->text() ) )
	return 0;
    
    return lvlnmfld_->text();
}


const Color* uiStratLevelSel::getLvlColor() const
{
    if ( !strcmp( sNoLevelTxt, lvlnmfld_->text() ) )
	return 0;

    const Strat::Level* lvl = Strat::RT().getLevel( lvlnmfld_->text() );
    if ( ! lvl ) return 0;

    return &lvl->color_;
}


void uiStratLevelSel::selLvlCB( CallBacker* )
{
    selchanged_.trigger();
}
