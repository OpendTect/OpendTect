/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwellsel.cc,v 1.1 2012-02-14 23:23:12 cvsnanne Exp $";

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
    selids_.erase();
    selnms_.erase();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ids[idx] );
	if ( !ioobj ) continue;

	selnms_.add( ioobj->name() );
	selids_ += ids[idx];
    }

    updSummary(0);
}


void uiWellParSel::doDlg( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    uiIOObjSelDlg dlg( this, *ctio, "Select Wells", true );
    uiIOObjSelGrp* selgrp = dlg.selGrp();
    selgrp->getListField()->setSelectedItems( selnms_ );
    if ( !dlg.go() ) return;

    selnms_.erase();
    selids_.erase();
    selgrp->processInput();
    selgrp->getListField()->getSelectedItems( selnms_ );
    selgrp->getSelected( selids_ );
}


BufferString uiWellParSel::getSummary() const
{
    BufferString summ;
    for ( int idx=0; idx<selnms_.size(); idx++ )
    {
	summ += selnms_.get(idx);
	summ += idx == selnms_.size()-1 ? "." : ", ";
    }

    return summ.isEmpty() ? BufferString(" - ") : summ;
}

