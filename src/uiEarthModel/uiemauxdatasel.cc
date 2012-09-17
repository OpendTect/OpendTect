/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id: uiemauxdatasel.cc,v 1.1 2010/04/07 12:11:40 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiemauxdatasel.h"

#include "uidialog.h"
#include "uiiosurface.h"

#include "bufstringset.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ptrman.h"

uiEMAuxDataSel::uiEMAuxDataSel( uiParent* p, const char* lbl,
       				const MultiID* hormid, const char* auxdatanm )
    : uiCompoundParSel( p, lbl )
    , hormid_(hormid ? *hormid : -1)
    , auxdatanm_(auxdatanm)
{
    butPush.notify( mCB(this,uiEMAuxDataSel,butPushCB) );
}


bool uiEMAuxDataSel::butPushCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Horizon/Attributes Selection","","") );
    PtrMan<uiSurfaceRead> surfacefld = new uiSurfaceRead( &dlg,
	    	  uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::keyword())
		  		 .withsectionfld(false)
	   			 .multiattribsel(false) );
    surfacefld->setInput( hormid_ );
    BufferStringSet attribname;
    attribname.add( auxdatanm_ );
    surfacefld->setSelAttributes( attribname );

    if ( !dlg.go() )
	return false;

    if ( surfacefld->selIOObj() )
	return false;

    hormid_ = surfacefld->selIOObj()->key();
    BufferStringSet selattribs;
    surfacefld->getSelAttributes( selattribs );
    if ( selattribs.size() <=0 )
	return false;

    auxdatanm_ = selattribs.get( 0 );
    return true;
}


const MultiID& uiEMAuxDataSel::getSurfaceID() const
{ return hormid_; }


const char* uiEMAuxDataSel::getAuxDataSel() const
{ return auxdatanm_; }


BufferString uiEMAuxDataSel::getSummary() const
{
    PtrMan<IOObj> ioobj = IOM().get( hormid_ );
    if ( !ioobj )
	return "";

    BufferString summary( ioobj->name() );
    summary.add( "[" ).add( auxdatanm_ ).add( "]" );
    return summary;
}
