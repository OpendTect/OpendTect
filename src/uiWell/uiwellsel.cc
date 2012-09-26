/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwellsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "welltransl.h"

#include "uilistbox.h"

#define mSelTxt seltxt && *seltxt ? seltxt \
				  : ( forread ? "Input Well" : "Output Well" )

uiWellSel::uiWellSel( uiParent* p, bool forread, const char* seltxt )
    : uiIOObjSel(p,mIOObjContext(Well),mSelTxt)
{ setForRead( forread ); }



uiWellParSel::uiWellParSel( uiParent* p )
    : uiCompoundParSel(p,"Well(s)","Select")
    , iopar_(*new IOPar)
{
    butPush.notify( mCB(this,uiWellParSel,doDlg) );
}


uiWellParSel::~uiWellParSel()
{
    delete &iopar_;
}


void uiWellParSel::setSelected( const TypeSet<MultiID>& ids )
{
    selids_ = ids;
    updSummary(0);
}


void uiWellParSel::getSelected( TypeSet<MultiID>& ids ) const
{ ids = selids_; }


void uiWellParSel::doDlg( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    uiIOObjSelDlg dlg( this, *ctio, "Select Wells", true );
    uiIOObjSelGrp* selgrp = dlg.selGrp();
    selgrp->usePar( iopar_ );
    if ( !dlg.go() ) return;

    selgrp->processInput();
    selids_.erase();
    selgrp->getSelected( selids_ );
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
	MultiID mid;
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
	PtrMan<IOObj> ioobj = IOM().get( selids_[idx] );
	if ( !ioobj ) continue;

	names.add( ioobj->name() );
    }

    return names.isEmpty() ? BufferString(" - ") : names.cat( ',' );
}
