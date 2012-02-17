/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwellsel.cc,v 1.2 2012-02-17 23:09:22 cvsnanne Exp $";

#include "uiwellsel.h"

#include "ioman.h"
#include "ioobj.h"
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
{
    butPush.notify( mCB(this,uiWellParSel,doDlg) );
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
    selgrp->setSelected( selids_ );
    if ( !dlg.go() ) return;

    selids_.erase();
    selgrp->processInput();
    selgrp->getSelected( selids_ );
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
