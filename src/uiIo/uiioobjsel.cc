/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.58 2003-06-27 08:40:47 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "iodirentry.h"
#include "uigeninput.h"
#include "uistatusbar.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "transl.h"
#include "ioman.h"
#include "iostrm.h"
#include "iolink.h"
#include "iodir.h"
#include "iopar.h"
#include "errh.h"


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread?"Input selection":"Output selection",
		    	"","8.1.1")
		.nrstatusflds(multisel?0:1))
	, ctio(c)
	, nmfld(0)
	, ioobj(0)
	, ismultisel(multisel && ctio.ctxt.forread)
	, manipgrp(0)
{
    if ( !ismultisel )
	statusBar()->setTxtAlign( 0, uiStatusBar::Right );
    BufferString nm( "Select " );
    nm += ctio.ctxt.forread ? "input " : "output ";
    nm += ctio.ctxt.trgroup->name();
    if ( ismultisel ) nm += "(s)";
    setTitleText( nm );

    IOM().to( MultiID(IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id) );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    if ( ctio.ioobj )
        entrylist->setSelected( ctio.ioobj->key() );

    uiGroup* topgrp = new uiGroup( this, "Top group" );
    entrylist->setName( seltxt );
    listfld = new uiLabeledListBox( topgrp, entrylist->Ptr() );
    if ( ismultisel )
	listfld->box()->setMultiSelect( true );
    listfld->box()->setPrefWidthInChar( 
		listfld->box()->optimumFieldWidth(25,60) );
    listfld->box()->setPrefHeightInChar( 8 );

    if ( !ctio.ctxt.forread )
    {
	nmfld = new uiGenInput( this, "Name" );
	nmfld->attach( stretchedBelow, topgrp );
	nmfld->setElemSzPol( uiObject::smallmax );
	nmfld->setStretch( 2, 0 );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiIOObjSelDlg,selChg) );
    listfld->box()->doubleClicked.notify( mCB(this,uiDialog,accept) );
    if ( !ismultisel && ctio.ctxt.maydooper )
    {
	manipgrp = new uiIOObjManipGroup( listfld->box(), *entrylist,
					  ctio.ctxt.trgroup->defExtension() );
	manipgrp->preRelocation.notify( mCB(this,uiIOObjSelDlg,preReloc) );
	manipgrp->postRelocation.notify( mCB(this,uiIOObjSelDlg,selChg) );
    }
    setOkText( "Select" );
    selChg( this );
    finaliseDone.notify( mCB(this,uiIOObjSelDlg,selChg) );
}


uiIOObjSelDlg::~uiIOObjSelDlg()
{
    delete entrylist;
}


int uiIOObjSelDlg::nrSel() const
{
    if ( !ismultisel )
	return ioobj ? 1 : 0;

    int nr = 0;
    for ( int idx=0; idx<listfld->box()->size(); idx++ )
	if ( listfld->box()->isSelected(idx) ) nr++;
    return nr;
}


const IOObj* uiIOObjSelDlg::selected( int objnr ) const
{
    const int nrsel = nrSel();
    if ( nrsel < 2 || objnr < 1 ) return ioobj;
    if ( objnr >= nrsel ) return 0;

    for ( int idx=0; idx<listfld->box()->size(); idx++ )
    {
	if ( listfld->box()->isSelected(idx) )
	    objnr--;
	if ( objnr < 0 )
	{
	    entrylist->setCurrent( idx );
	    return entrylist->selected();
	}
    }
    BufferString msg( "Should not reach. objnr=" );
    msg += objnr; msg += " nrsel="; msg += nrsel;
    pErrMsg( msg );
    return 0;
}


void uiIOObjSelDlg::selChg( CallBacker* cb )
{
    if ( ismultisel ) return;
    const int curitm = listfld->box()->currentItem();
    ioobj = 0;
    if ( curitm >= 0 )
    {
	entrylist->setCurrent( listfld->box()->currentItem() );
	ioobj = entrylist->selected();
	if ( cb && nmfld )
	    nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
	BufferString nm( ioobj ? ioobj->fullUserExpr(ctio.ctxt.forread) : "" );
	int len = nm.size();
	if ( len > 44 )
	{
	    BufferString tmp( nm );
	    nm = "....";
	    nm += tmp.buf() + len - 40;
	}
	toStatusBar( nm );
    }

    if ( manipgrp )
	manipgrp->selChg( cb );
}


void uiIOObjSelDlg::preReloc( CallBacker* cb )
{
    if ( manipgrp )
	toStatusBar( manipgrp->curRelocationMsg() );
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
	    listfld->box()->setCurrentItem( entrylist->selected()->name() );
	    const_cast<uiIOObjSelDlg*>( this )->selChg(0);
	}
    }
}


bool uiIOObjSelDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    if ( !nmfld )
    {
	if ( ismultisel )
	{
	    for ( int idx=0; idx<listfld->box()->size(); idx++ )
	    {
		if ( listfld->box()->isSelected(idx) )
		{
		    entrylist->setCurrent( idx );
		    ioobj = entrylist->selected();
		    break;
		}
	    }
	    if ( !ioobj )
	    {
		uiMSG().error( "Please select at least one, or press Cancel" );
		return false;
	    }
	}
	mDynamicCastGet(IOLink*,iol,ioobj)
	bool needmore = !ioobj || iol;
	if ( iol && ctio.ctxt.maychdir && !entrylist->mustChDir() )
	    needmore = false;
	if ( needmore )
	{
	    IOM().to( iol );
	    entrylist->fill( IOM().dirPtr() );
	    listfld->box()->empty();
	    listfld->box()->addItems( entrylist->Ptr() );
	    return false;
	}
	return true;
    }

    const char* seltxt = nmfld->text();
    if ( ioobj && ioobj->name() == seltxt ) return true;

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


void uiIOObjSelDlg::setInitOutputName( const char* nm )
{
    if ( nmfld ) 
	nmfld->setText( nm );
    listfld->box()->setCurrentItem( nm );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			bool wclr, const char* st, const char* buttxt )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		      txt ? txt : (const char*)c.ctxt.trgroup->name(), 
		      wclr, buttxt )
	, ctio(c)
	, forread(c.ctxt.forread)
	, seltxt(st)
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
    if ( specialitems.findKeyFor(inp) )
    {
	ctio.setObj( 0 );
	return;
    }

    int selidx = getCurrentItem();
    if ( selidx >= 0 )
    {
	const char* itemusrnm = userNameFromKey( getItem(selidx) );
	if ( !strcmp(inp,itemusrnm) && ctio.ioobj ) return;
    }

    IOM().to( ctio.ctxt.stdSelKey() );
    const IOObj* ioobj = (*IOM().dirPtr())[inp];
    ctio.setObj( ioobj && ctio.ctxt.validIOObj(*ioobj) ? ioobj->clone() : 0 );
    updateInput();
}


bool uiIOObjSel::existingTyped() const
{
    const char* inp = getInput();
    IOM().to( ctio.ctxt.stdSelKey() );
    return (*IOM().dirPtr())[inp];
}


bool uiIOObjSel::commitInput( bool mknew )
{
    const char* inp = getInput();
    if ( specialitems.findKeyFor(inp) )
    {
	ctio.setObj( 0 );
	return true;
    }

    processInput();
    if ( existingTyped() )
    {
       if ( ctio.ioobj ) return true;
       BufferString msg( getInput() );
       msg += ": Please enter another name.";
       uiMSG().error( msg );
       return false;
    }
    if ( !mknew ) return false;

    ctio.setObj( createEntry( getInput() ) );
    return ctio.ioobj;
}


void uiIOObjSel::doObjSel( CallBacker* )
{
    ctio.ctxt.forread = forread;
    uiIOObjRetDlg* dlg = mkDlg();
    if ( dlg && dlg->go() && dlg->ioObj() )
    {
	ctio.setObj( dlg->ioObj()->clone() );
	updateInput();
	newSelection( dlg );
	selok_ = true;
    }
    delete dlg;
}


void uiIOObjSel::objSel()
{
    const char* key = getKey();
    if ( specialitems.find(key) )
	ctio.setObj( 0 );
    else
	ctio.setObj( IOM().get(getKey()) );
}


uiIOObjRetDlg* uiIOObjSel::mkDlg()
{
    return new uiIOObjSelDlg( this, ctio, seltxt );
}


IOObj* uiIOObjSel::createEntry( const char* nm )
{
    return mkEntry( ctio, nm );
}
