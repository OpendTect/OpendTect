/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.12 2001-07-18 21:48:01 bert Exp $
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
#include "iopar.h"
#include "transl.h"


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* trglobexpr )
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
				    ctio.ctxt.maychdir, trglobexpr );
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
    selChg(0);
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


void uiIOObjSelDlg::fillPar( IOPar& iopar ) const
{
    iopar.set( "ID", ioobj ? (const char*)ioobj->key() : "" );
}


void uiIOObjSelDlg::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    if ( res && *res )
    {
	MultiID key( res );
	if ( entrylist->selected() && entrylist->selected()->key() != key )
	{
	    entrylist->setSelected( MultiID(res) );
	    listfld->setCurrentItem( entrylist->selected()->name() );
	    const_cast<uiIOObjSelDlg*>( this )->selChg(0);
	}
    }
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

    return createEntry( seltxt );
}


bool uiIOObjSelDlg::createEntry( const char* seltxt )
{
    ioobj = mkEntry( ctio, seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
	return false;
    }

    return true;
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			bool wclr, const char* trglexp )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		     txt ? txt : (const char*)c.ctxt.trgroup->name(), wclr )
	, ctio(c)
	, forread(c.ctxt.forread)
	, trglobexpr(trglexp)
{
    updateInput();
}


uiIOObjSel::~uiIOObjSel()
{
}


bool uiIOObjSel::fillPar( IOPar& iopar ) const
{
    iopar.set( "ID", ctio.ioobj ? ctio.ioobj->key() : MultiID("") );
    return true;
}


void uiIOObjSel::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    if ( res && *res )
	setInput( res );
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
    if ( selidx < 0 || !strcmp(inp,getItem(selidx)) ) return;

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

    ctio.setObj( createEntry( getInput() ) );
    return ctio.ioobj;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    ctio.ctxt.forread = forread;
    uiIOObjSelDlg* dlg = mkDlg();
    if ( dlg && dlg->go() && dlg->ioObj() )
    {
	ctio.setObj( dlg->ioObj()->clone() );
	updateInput();
	selDone( 0 );
	newSelection( dlg );
    }
    delete dlg;
}


void uiIOObjSel::objSel()
{
    ctio.setObj( IOM().get(getKey()) );
}


uiIOObjSelDlg* uiIOObjSel::mkDlg()
{
    return new uiIOObjSelDlg( this, ctio, trglobexpr );
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    return mkEntry( ctio, nm );
}
