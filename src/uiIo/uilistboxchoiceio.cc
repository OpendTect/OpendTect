/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilistboxchoiceio.h"

#include "ioobjselectiontransl.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "ctxtioobj.h"

#include "uilistbox.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#define mCtioObjTypeName() ctio_.ctxt.objectTypeName()


uiListBoxChoiceIO::uiListBoxChoiceIO( uiListBox& lb, const char* omftypekey )
    : ctio_(*mMkCtxtIOObj(IOObjSelection))
    , lb_(lb)
    , storeRequested(this)
    , readDone(this)
{
    if ( omftypekey && *omftypekey )
	ctio_.ctxt.toselect.require_.add( sKey::Type(), omftypekey );

    lb_.offerReadWriteSelection( mCB(this,uiListBoxChoiceIO,readReqCB),
			         mCB(this,uiListBoxChoiceIO,storeReqCB));
}


uiListBoxChoiceIO::~uiListBoxChoiceIO()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiListBoxChoiceIO::setChosen( const BufferStringSet& itemnms )
{
    lb_.setChosen( itemnms );
}


void uiListBoxChoiceIO::readReqCB( CallBacker* )
{
    chosennames_.setEmpty(); keys_.setEmpty();
    ctio_.ctxt.forread = true;
    uiIOObjSelDlg dlg( lb_.parent(), ctio_ );
    if ( !dlg.go() ) return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    const BufferString fnm( ioobj->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	const BufferString msg( "Cannot open : ", fnm, " for read" );
	uiMSG().error( msg );
	return;
    }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType( mCtioObjTypeName() ) )
    {
	const BufferString msg( fnm, " has the wrong file type: ",
				astrm.fileType() );
	uiMSG().error( msg );
	return;
    }

    while ( !atEndOfSection(astrm.next()) )
    {
	chosennames_.add( astrm.keyWord() );
	keys_.add( astrm.value() );
    }

    readDone.trigger();
}


void uiListBoxChoiceIO::storeReqCB( CallBacker* )
{
    ctio_.ctxt.forread = false;
    uiIOObjSelDlg dlg( lb_.parent(), ctio_ );
    if ( !dlg.go() ) return;
    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return;

    const BufferString fnm( ioobj->fullUserExpr(false) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( BufferString("Cannot open : ", fnm, " for write") );
	return;
    }

    storeRequested.trigger();
    const int nrkeys = keys_.size();

    ascostream astrm( strm );
    astrm.putHeader( mCtioObjTypeName() );
    for ( int idx=0; idx<lb_.size(); idx++ )
    {
	if ( lb_.isChosen(idx) )
	{
	    const BufferString itmnm( lb_.textOfItem(idx) );
	    if ( nrkeys < 1 )
		astrm.put( itmnm );
	    else
		astrm.put( itmnm, idx < nrkeys ? keys_.get(idx).buf() : "-" );
	}
    }
    astrm.newParagraph();
}
