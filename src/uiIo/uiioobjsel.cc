/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.10 2001-07-13 22:03:14 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "ioman.h"
#include "ioobj.h"
#include "iolink.h"
#include "iodir.h"
#include "transl.h"


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c )
	: uiDialog(p,c.ctxt.forread?"Input":"Output")
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
    //listfld->doubleclick.notify( mCB(this,uiDialog,acceptOK) );
    listfld->notify( mCB(this,uiIOObjSelDlg,selChg) );

    setOkText( "Select" );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    delete entrylist;
}


void uiIOObjSelDlg::selChg( CallBacker* c )
{
    entrylist->setCurrent( listfld->currentItem() );
    ioobj = entrylist->selected();
    if ( nmfld && c )
	nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
}


bool uiIOObjSelDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    if ( !nmfld )
    {
	mDynamicCastGet(IOLink*,iol,ioobj)
	if ( !ioobj || iol )
	{
	    IOM().to( iol );
	    entrylist->fill( IOM().dirPtr() );
	    listfld->empty();
	    listfld->addItems( entrylist->Ptr() );
	    return false;
	}
	return true;
    }

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


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			      bool wclr )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		     txt ? txt : (const char*)c.ctxt.trgroup->name(), wclr )
	, ctio(c)
	, forread(c.ctxt.forread)
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
    static BufferString nm;
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky, false );
    return (const char*)nm;
}


void uiIOObjSel::processInput()
{
    const char* inp = getInput();
    int selidx = getCurrentItem();
    if ( !strcmp(inp,getItem(selidx)) ) return;

    const IOObj* ioobj = (*IOM().dirPtr())[inp];
    if ( !ioobj ) return;

    ctio.setObj( ioobj->clone() );
    updateInput();
}


bool uiIOObjSel::existingTyped() const
{
    const char* inp = getInput();
    return (*IOM().dirPtr())[inp];
}


bool uiIOObjSel::commitInput( bool mknew )
{
    processInput();
    if ( existingTyped() ) return true;
    if ( !mknew ) return false;

    ctio.setName( getInput() );
    IOM().getEntry( ctio );
    return ctio.ioobj;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    ctio.ctxt.forread = forread;
    uiIOObjSelDlg dlg( this, ctio );
    if ( dlg.go() && dlg.ioObj() )
    {
	ctio.setObj( dlg.ioObj()->clone() );
	updateInput();
	selDone( 0 );
    }
}


void uiIOObjSel::objSel()
{
    ctio.setObj( IOM().get(getKey()) );
}
