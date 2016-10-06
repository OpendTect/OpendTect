/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/


#include "uiwellsel.h"

#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "welltransl.h"

#include "uilistbox.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"

#define mSelTxt seltxt && *seltxt ? seltxt \
				  : ( forread ? "Input Well" : "Output Well" )

uiIOObjSel::Setup uiWellSel::getSetup( bool forread, const uiString& seltxt,
					bool withinserters ) const
{
    uiString st = seltxt;
    if ( st.isEmpty() )
	st = forread ? tr("Input Well") : tr("Output Well");
    uiIOObjSel::Setup su( st );
    su.withinserters( withinserters );
    return su;
}


IOObjContext uiWellSel::getContext( bool forread, bool withinserters ) const
{
    IOObjContext ret( mRWIOObjContext(Well,forread) );
    if ( !withinserters )
	ret.fixTranslator( "dGB" );
    return ret;
}


uiWellSel::uiWellSel( uiParent* p, bool forread, const uiString& seltxt,
			bool withinserters )
    : uiIOObjSel(p,getContext(forread,withinserters),
		 getSetup(forread,seltxt,withinserters))
{
}


uiWellSel::uiWellSel( uiParent* p, bool forread, const uiIOObjSel::Setup& su )
    : uiIOObjSel(p,getContext(forread,su.withinserters_),su)
{
}



uiWellParSel::uiWellParSel( uiParent* p )
    : uiCompoundParSel(p,uiStrings::sWell(mPlural))
    , iopar_(*new IOPar)
{
    butPush.notify( mCB(this,uiWellParSel,doDlg) );
}


uiWellParSel::~uiWellParSel()
{
    delete &iopar_;
}


void uiWellParSel::setSelected( const DBKeySet& ids )
{
    selids_ = ids;
    updSummary(0);
}


void uiWellParSel::getSelected( DBKeySet& ids ) const
{ ids = selids_; }


void uiWellParSel::doDlg( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    uiIOObjSelDlg::Setup sdsu( tr("Select Wells") ); sdsu.multisel( true );
    uiIOObjSelDlg dlg( this, sdsu, *ctio );
    uiIOObjSelGrp* selgrp = dlg.selGrp();
    selgrp->usePar( iopar_ );
    if ( !dlg.go() ) return;

    selids_.erase();
    selgrp->getChosen( selids_ );
    iopar_.setEmpty();
    selgrp->fillPar( iopar_ );
}


void uiWellParSel::fillPar( IOPar& iop ) const
{ iop.mergeComp( iopar_, sKey::Well() ); }

bool uiWellParSel::usePar( const IOPar& iop )
{
    selids_.erase();
    iopar_.setEmpty();
    PtrMan<IOPar> subsel = iop.subselect( sKey::Well() );
    if ( !subsel ) return false;

    iopar_ = *subsel;

    int nrids;
    iopar_.get( sKey::Size(), nrids );
    for ( int idx=0; idx<nrids; idx++ )
    {
	DBKey mid;
	if ( iopar_.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    selids_ += mid;
    }

    updSummary(0);
    return true;
}


BufferString uiWellParSel::getSummary() const
{
    BufferStringSet names;
    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( selids_[idx] );
	if ( !ioobj ) continue;

	names.add( ioobj->name() );
    }

    return names.getDispString( -1, false );
}
