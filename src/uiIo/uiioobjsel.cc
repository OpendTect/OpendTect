/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.5 2001-05-07 16:36:58 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "ioman.h"
#include "ioobj.h"
#include "transl.h"


uiIOObjSelDlg::uiIOObjSelDlg( uiObject* p, const CtxtIOObj& c )
	: uiDialog(p)
	, ctio(c)
	, nmfld(0)
	, ioobj(0)
{
    BufferString nm( "Select " );
    nm += ctio.ctxt.forread ? "input " : "output ";
    nm += ctio.ctxt.trgroup->name();
    setName( nm );

    IOM().to( MultiID(IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id) );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt.trgroup,
				    ctio.ctxt.maychdir );
    if ( ctio.ioobj )
        entrylist->setSelected( ctio.ioobj->key() );

    listfld = new uiListBox( this, entrylist->Ptr() );
    if ( !ctio.ctxt.forread )
    {
	nmfld = new uiGenInput( this, "Name" );
	nmfld->attach( alignedBelow, listfld );
    }

    //TODO
    //listfld->selection.notify( mCB(this,uiIOObjSelDlg,selChg) );
    //listfld->doubleclick.notify( mCB(this,uiDialog,accept) );
    listfld->notify( mCB(this,uiIOObjSelDlg,selChg) );

    setOkText( "Select" );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    delete entrylist;
}


void uiIOObjSelDlg::selChg( CallBacker* )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();
    if ( nmfld )
	nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
}


bool uiIOObjSelDlg::acceptOK( CallBacker* )
{
    if ( !ioobj ) { uiMSG().error( "Please select an object" ); return false; }
    if ( !nmfld ) return true;

    const char* seltxt = nmfld->text();
    if ( ioobj->name() == seltxt ) return true;

    int selidx = entrylist->indexOf( seltxt );
    if ( selidx >= 0 )
    {
	entrylist->setCurrent( selidx );
	ioobj = entrylist->selected();
	return true;
    }

    // create new entry
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( seltxt );
    newctio.fillObj();
    ioobj = newctio.ioobj;
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
	return false;
    }

    return true;
}


uiIOObjSel::uiIOObjSel( uiObject* p, CtxtIOObj& c, const char* txt,
			      bool wclr )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		     txt ? txt : (const char*)c.ctxt.trgroup->name(), wclr )
	, ctio(c)
{
    updateInput();
}


void uiIOObjSel::updateInput()
{
    if ( ctio.ioobj )
	setInput( (const char*)ctio.ioobj->key() );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    static BufferString key;
    key = "";
    if ( ky && *ky )
	key = IOM().nameOf( ky, false );
    return (const char*)key;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    uiIOObjSelDlg dlg( this, ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio.setObj( dlg.ioObj()->clone() );
	updateInput();
	selDone( 0 );
    }
}
