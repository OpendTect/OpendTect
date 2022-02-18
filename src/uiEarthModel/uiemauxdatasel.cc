/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
________________________________________________________________________

-*/

#include "uiemauxdatasel.h"

#include "uidialog.h"
#include "uiiosurface.h"

#include "bufstringset.h"
#include "keystrs.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ptrman.h"

uiEMAuxDataSel::uiEMAuxDataSel( uiParent* p, const uiString& lbl,
				const MultiID* hormid, const char* auxdatanm )
    : uiCompoundParSel( p, lbl )
    , hormid_(hormid ? *hormid : MultiID::udf())
    , auxdatanm_(auxdatanm)
{
    butPush.notify( mCB(this,uiEMAuxDataSel,butPushCB) );
}


void uiEMAuxDataSel::butPushCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(tr("Horizon/Attributes Selection"),
					uiStrings::sEmptyString(),mNoHelpKey) );
    PtrMan<uiSurfaceRead> surfacefld = new uiSurfaceRead( &dlg,
		  uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::sGroupName())
				 .withsectionfld(false)
				 .multiattribsel(false) );
    surfacefld->setInput( hormid_ );
    BufferStringSet attribname;
    attribname.add( auxdatanm_ );
    surfacefld->setSelAttributes( attribname );

    if ( !dlg.go() )
	return;

    if ( surfacefld->selIOObj() )
	return;

    hormid_ = surfacefld->selIOObj()->key();
    BufferStringSet selattribs;
    surfacefld->getSelAttributes( selattribs );
    if ( selattribs.size() <=0 )
	return;

    auxdatanm_ = selattribs.get( 0 );
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
