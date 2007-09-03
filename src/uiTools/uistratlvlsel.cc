/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          September 2007
 RCS:		$Id: uistratlvlsel.cc,v 1.1 2007-09-03 15:15:24 cvshelene Exp $
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
{
    BufferStringSet bfset;
    fillLvlList( bfset );
    lvlnmfld_ = new uiGenInput( p, "Tied to level", StringListInpSpec(bfset) );
    CallBack cb = mCB(this,uiStratLevelSel,defineLvlCB);
    deflvlbut_ = new uiPushButton( p, "&Define Level", cb, true );
    deflvlbut_->attach( rightOf, lvlnmfld_ );
}


void uiStratLevelSel::fillLvlList( BufferStringSet& bfset ) const
{
    bfset.add( sNoLevelTxt );
    for ( int idx=0; idx<Strat::RT().nrLevels(); idx++ )
	bfset.add( Strat::RT().level(idx)->name_ );
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

    return &lvl->color();
}
