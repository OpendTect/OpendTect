/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uichecklist.cc,v 1.4 2011-09-27 04:08:32 cvsraman Exp $";

#include "uichecklist.h"
#include "uibutton.h"
#include "bufstringset.h"


uiCheckList::uiCheckList( uiParent* p, const char* t1, const char* t2,
			  uiCheckList::Pol pl )
    : uiGroup(p,"CheckList")
    , pol_(pl)
    , changed(this)
{
    addBox( t1 );
    addBox( t2 );
    finaliseDone.notify( mCB(this,uiCheckList,initGrp) );
}


uiCheckList::uiCheckList( uiParent* p, const BufferStringSet& bss,
			  uiCheckList::Pol pl, bool forcehor )
    : uiGroup(p,"CheckList")
    , pol_(pl)
    , changed(this)
{
    for ( int idx=0; idx<bss.size(); idx++ )
	addBox( bss.get(idx), forcehor );

    finaliseDone.notify( mCB(this,uiCheckList,initGrp) );
}


void uiCheckList::addBox( const char* txt, bool hor )
{
    uiCheckBox* cb = new uiCheckBox( this, txt );
    if ( !boxs_.isEmpty() )
	cb->attach( hor ? rightOf : alignedBelow, boxs_[ boxs_.size()-1 ] );
    boxs_ += cb;
}


void uiCheckList::initGrp( CallBacker* )
{
    if ( pol_ >= uiCheckList::Chain1st )
	handleChain();

    const CallBack cb( mCB(this,uiCheckList,boxChk) );
    for ( int idx=0; idx<boxs_.size(); idx++ )
	boxs_[idx]->activated.notify( cb );
}


bool uiCheckList::isChecked( int idx ) const
{
    return idx < 0 || idx >= boxs_.size() ? false : boxs_[idx]->isChecked();
}


void uiCheckList::setChecked( int idx, bool yn )
{
    if ( idx >= 0 && idx < boxs_.size() )
	boxs_[idx]->setChecked( yn );
}


void uiCheckList::boxChk( CallBacker* c )
{
    mDynamicCastGet(uiCheckBox*,cb,c)
    if ( !cb ) { pErrMsg("Huh"); return; }
    clicked_ = cb;

    switch ( pol_ )
    {
    case Unrel:		break;
    case NotAll:	ensureOne(false); break;
    case AtLeastOne:	ensureOne(true); break;
    default:		handleChain(); break;
    }

    changed.trigger();
}


void uiCheckList::ensureOne( bool ischckd )
{
    if ( clicked_->isChecked() == ischckd )
	return;

    int boxidx = 0;
    for ( int idx=0; idx<boxs_.size(); idx++ )
    {
	if ( boxs_[idx]->isChecked() == ischckd )
	    return;
	else if ( boxs_[idx] == clicked_ )
	    boxidx = idx;
    }
    int box2check = boxidx - 1;
    if ( box2check < 0 ) box2check = boxs_.size() - 1;
    if ( box2check == boxidx ) return; // single button
    setBox( box2check, ischckd );
}


void uiCheckList::handleChain()
{
    bool prevchk = boxs_[0]->isChecked();

    for ( int idx=1; idx<boxs_.size(); idx++ )
    {
	const bool ischkd = boxs_[idx]->isChecked();
	setBox( idx, prevchk ? ischkd : false, prevchk );
	if ( pol_ == ChainAll && prevchk )
	    prevchk = boxs_[idx]->isChecked();
    }
}


void uiCheckList::setBox( int idx, bool chkd, bool shw )
{
    uiCheckBox* cb = boxs_[idx];
    NotifyStopper ns( cb->activated );
    cb->setChecked( chkd );
    cb->display( shw );
}
