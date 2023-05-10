/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilistboxchoiceio.h"

#include "ioobjselectiontransl.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "ctxtioobj.h"

#include "uilistbox.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"

#define mCtioObjTypeName() ctio_.ctxt_.objectTypeName()

static const char* sDefault()		{ return "Default"; }

uiListBoxChoiceIO::uiListBoxChoiceIO( uiListBox& lb, const char* omftypekey )
    : ctio_(*mMkCtxtIOObj(IOObjSelection))
    , lb_(lb)
    , storeRequested(this)
    , readDone(this)
{
    if ( omftypekey && *omftypekey )
	ctio_.ctxt_.toselect_.require_.add( sKey::Type(), omftypekey );

    lb_.offerReadWriteSelection( mCB(this,uiListBoxChoiceIO,readReqCB),
			         mCB(this,uiListBoxChoiceIO,storeReqCB));
}


uiListBoxChoiceIO::~uiListBoxChoiceIO()
{
    delete ctio_.ioobj_;
    delete &ctio_;
}


void uiListBoxChoiceIO::setChosen( const BufferStringSet& itemnms )
{
    lb_.setChosen( itemnms );
}


void uiListBoxChoiceIO::readReqCB( CallBacker* )
{
    BufferStringSet defkeys;
    chosennames_.setEmpty(); keys_.setEmpty();
    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( lb_.parent(), ctio_ );
    if ( !dlg.go() )
	return;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj )
	return;

    const BufferString fnm( ioobj->fullUserExpr(true) );
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	const uiString msg = tr("Cannot open : %1 for read").arg(fnm);
	uiMSG().error( msg );
	return;
    }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType( mCtioObjTypeName() ) )
    {
	const uiString msg = tr("%1 has the wrong file type: %2")
			   .arg(fnm).arg(astrm.fileType());
	uiMSG().error( msg );
	return;
    }

    while ( !atEndOfSection(astrm.next()) )
    {
	chosennames_.add( astrm.keyWord() );
	keys_.add( MultiID(astrm.value()) );
    }

    readDone.trigger();
}


void uiListBoxChoiceIO::storeReqCB( CallBacker* )
{
    ctio_.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( lb_.parent(), ctio_ );
    if ( !dlg.go() )
	return;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj )
	return;

    const BufferString fnm( ioobj->fullUserExpr(false) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error(tr("Cannot open : %1 for write").arg(fnm));
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
		astrm.put( itmnm, idx<nrkeys ? keys_.get(idx).toString() : "-");
	}
    }

    astrm.newParagraph();
    for ( int idx=0; idx<lb_.size(); idx++ )
    {
	if ( nrkeys<1 )
	    break;

	if ( lb_.isMarked(idx) )
	    astrm.put( sDefault(), keys_.get(idx) );
    }

    astrm.newParagraph();
}
