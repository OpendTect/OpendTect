/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
________________________________________________________________________

-*/

#include "uichecklist.h"
#include "uibutton.h"
#include "uilabel.h"
#include "bufstringset.h"


uiCheckList::uiCheckList( uiParent* p, uiCheckList::Pol pl,
			  OD::Orientation ori )
    : uiGroup(p,"CheckList")
    , pol_(pl)
    , orientation_(ori)
    , changed(this)
    , lbl_(0)
    , clicked_(0)
{
    grp_ = new uiGroup( this, "CheckList buttons" );
    setHAlignObj( grp_ );
    postFinalize().notify( mCB(this,uiCheckList,initObj) );
}


uiCheckList& uiCheckList::addItem( const uiString& txt,const char* iconfnm )
{
    uiCheckBox* cb = new uiCheckBox( grp_, txt );

    if ( iconfnm && *iconfnm )
	cb->setIcon( iconfnm );

    if ( !boxs_.isEmpty() )
	cb->attach( isHor() ? rightOf : alignedBelow, boxs_[ boxs_.size()-1 ] );

    cb->setStretch( 0, orientation_ == OD::Vertical ? 2 : 0 );

    boxs_ += cb;
    return *this;
}


uiCheckList& uiCheckList::addItems( const BufferStringSet& itms )
{
    for ( int idx=0; idx<itms.size(); idx++ )
	addItem( mToUiStringTodo(itms.get(idx)), 0 );
    return *this;
}


uiCheckList& uiCheckList::displayIdx( int idx, bool yn )
{
    if ( boxs_.validIdx( idx ) )
	boxs_[idx]->display( yn );

    return *this;
}


void uiCheckList::setLabel( const uiString& txt )
{
    if ( lbl_ )
	lbl_->setText( txt );
    else
    {
	lbl_ = new uiLabel( this, txt );
	lbl_->attach( centeredLeftOf, grp_ );
    }
    setName( txt.getFullString() );

}


uiCheckList& uiCheckList::addItems( const uiStringSet& itms )
{
    for ( int idx=0; idx<itms.size(); idx++ )
	addItem( itms[idx], 0 );
    return *this;
}


void uiCheckList::initObj( CallBacker* )
{
    clicked_ = boxs_.isEmpty() ? 0 : boxs_[0];
    if ( !clicked_ ) return;

    if ( pol_ >= Chain1st )
	handleChain();
    else if ( pol_ >= OneOnly )
	handleRadio( pol_ == MaybeOne );
    else if ( pol_ != Unrel )
	ensureOne( pol_ == OneMinimum );

    const CallBack cb( mCB(this,uiCheckList,boxChk) );
    for ( int idx=0; idx<boxs_.size(); idx++ )
	boxs_[idx]->activated.notify( cb );
}


bool uiCheckList::isChecked( int idx ) const
{
    return boxs_.validIdx(idx) ? boxs_[idx]->isChecked() : false;
}


uiCheckList& uiCheckList::setChecked( int idx, bool yn )
{
    if ( boxs_.validIdx(idx) )
    {
	boxs_[idx]->setChecked( yn );
	boxChk( boxs_[idx] );
    }
    return *this;
}


int uiCheckList::firstChecked() const
{
    for ( int idx=0; idx<boxs_.size(); idx++ )
    {
	if ( boxs_[idx]->isChecked() )
	    return idx;
    }
    return -1;
}


int uiCheckList::lastChecked() const
{
    for ( int idx=boxs_.size()-1; idx>-1; idx-- )
    {
	if ( boxs_[idx]->isChecked() )
	    return idx;
    }
    return -1;
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
    case OneMinimum:	ensureOne(true); break;
    case OneOnly:	handleRadio(false); break;
    case MaybeOne:	handleRadio(true); break;
    default:		handleChain(); break;
    }

    changed.trigger();
}


void uiCheckList::ensureOne( bool ischckd )
{
    if ( clicked_ && clicked_->isChecked() == ischckd )
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


void uiCheckList::handleRadio( bool allownone )
{
    int nrchcked = 0;
    for ( int idx=0; idx<boxs_.size(); idx++ )
    {
	if ( boxs_[idx]->isChecked() )
	    nrchcked++;
    }
    if ( nrchcked == 0 )
    {
	if ( !allownone )
	    setBox( boxs_.indexOf(clicked_), true );
    }
    else if ( nrchcked > 1 )
    {
	const bool clckischcked = clicked_ && clicked_->isChecked();
	bool havechck = false;
	for ( int idx=0; idx<boxs_.size(); idx++ )
	{
	    uiCheckBox* cb = boxs_[idx];
	    if ( !cb->isChecked() )
		continue;

	    if ( clckischcked )
	    {
		if ( cb != clicked_ )
		    setBox( idx, false );
	    }
	    else if ( havechck )
		setBox( idx, false );
	    else
		havechck = true;
	}
    }
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
