/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.82 2006-03-01 13:45:46 cvsbert Exp $
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

static void getIOObjNames( IODirEntryList& entrylist, BufferStringSet& nms )
{
    for ( int idx=0; idx<entrylist.size(); idx++ )
	nms.add( entrylist[idx]->name() );
}


uiIOObjSelGrp::uiIOObjSelGrp( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
    : uiGroup(p)
    , ctio(*new CtxtIOObj(c))
    , nmfld(0)
    , ioobj(0)
    , ismultisel(multisel && ctio.ctxt.forread)
    , manipgrp(0)
    , newstatusmessage( this )
    , statusmessage( 0 )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id) );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );

    topgrp = new uiGroup( this, "Top group" );
    filtfld = new uiGenInput( topgrp, "Filter", "*" );
    filtfld->valuechanged.notify( mCB(this,uiIOObjSelGrp,rebuildList) );
    listfld = new uiLabeledListBox( topgrp, seltxt );
    if ( ismultisel )
	listfld->box()->setMultiSelect( true );
    listfld->box()->setPrefWidthInChar( 
		listfld->box()->optimumFieldWidth(25,60) );
    listfld->box()->setPrefHeightInChar( 8 );
    fillList();
    listfld->attach( alignedBelow, filtfld );
    topgrp->setHAlignObj( listfld->box() );

    if ( ctio.ioobj )
        listfld->box()->setCurrentItem( ctio.ioobj->name() );

    if ( !ctio.ctxt.forread )
    {
	nmfld = new uiGenInput( this, "Name" );
	nmfld->attach( alignedBelow, topgrp );
	nmfld->setElemSzPol( uiObject::smallmax );
	nmfld->setStretch( 2, 0 );

	const char* nm = ctio.name();
	if ( nm && *nm )
	{
	    nmfld->setText( nm );
	    if ( listfld->box()->isPresent( nm ) )
		listfld->box()->setCurrentItem( nm );
	    else
		listfld->box()->clear();
	}
    }

    listfld->box()->selectionChanged.notify(
	    mCB(this,uiIOObjSelGrp,selectionChange) );
    if ( !ismultisel && ctio.ctxt.maydooper )
    {
	manipgrp = new uiIOObjManipGroup( listfld->box(), *entrylist,
					  ctio.ctxt.trgroup->defExtension() );
	manipgrp->preRelocation.notify( mCB(this,uiIOObjSelGrp,preReloc) );
	manipgrp->postRelocation.notify(
		mCB(this,uiIOObjSelGrp,selectionChange) );
    }

    selectionChange( this );
}


uiIOObjSelGrp::~uiIOObjSelGrp()
{
    delete &ctio;
    delete entrylist;
}


int uiIOObjSelGrp::nrSel() const
{
    if ( !ismultisel )
	return ioobj ? 1 : 0;

    int nr = 0;
    for ( int idx=0; idx<listfld->box()->size(); idx++ )
	if ( listfld->box()->isSelected(idx) ) nr++;
    return nr;
}


const IOObj* uiIOObjSelGrp::selected( int objnr ) const
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


void uiIOObjSelGrp::rebuildList( CallBacker* cb )
{
    IOM().to( ctio.ctxt.stdSelKey() );
    //entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    BufferString nmflt = filtfld->text();
    entrylist->ctxt = ctio.ctxt;

    if ( nmflt != "" && nmflt != "*" )
	entrylist->fill( IOM().dirPtr(), nmflt );
    else 
	entrylist->fill( IOM().dirPtr() );
    fillList();
    selectionChange(cb);
}


void uiIOObjSelGrp::fillList()
{
    listfld->box()->empty();
    BufferStringSet nms; getIOObjNames( *entrylist, nms );
    listfld->box()->addItems( nms );
    if ( nms.size() )
	listfld->box()->setCurrentItem( 0 );
}


void uiIOObjSelGrp::toStatusBar( const char* txt )
{
    statusmessage = txt;
    newstatusmessage.trigger();
}


void uiIOObjSelGrp::selectionChange( CallBacker* cb )
{
    if ( ismultisel ) return;

    const int curitm = listfld->box()->currentItem();
    bool issel = listfld->box()->isSelected( curitm );
    ioobj = 0;
    if ( curitm >= 0 && issel )
    {
	entrylist->setCurrent( curitm );
	ioobj = entrylist->selected();
	if ( cb && nmfld )
	    nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
	BufferString nm( ioobj ? ioobj->fullUserExpr(ctio.ctxt.forread) : "" );
	int len = nm.size();
	if ( len>44 )
	{
	    BufferString tmp( nm );
	    nm = "....";
	    nm += tmp.buf() + len - 40;
	}
	toStatusBar( nm );
    }
    else if ( !issel )
    {
	if ( cb && nmfld ) nmfld->setText("");
	toStatusBar( "" );
    }

    if ( manipgrp )
	manipgrp->selChg( cb );
}


void uiIOObjSelGrp::setContext( const CtxtIOObj& c )
{
    ctio = c;
    rebuildList();
}


void uiIOObjSelGrp::preReloc( CallBacker* cb )
{
    if ( manipgrp )
	toStatusBar( manipgrp->curRelocationMsg() );
}


bool uiIOObjSelGrp::fillPar( IOPar& iopar ) const
{
    iopar.set( "ID", ioobj ? (const char*)ioobj->key() : "" );
    return true;
}


void uiIOObjSelGrp::usePar( const IOPar& iopar )
{
    const char* res = iopar.find( "ID" );
    if ( res && *res )
    {
	MultiID key( res );
	if ( entrylist->selected() && entrylist->selected()->key() != key )
	{
	    entrylist->setSelected( MultiID(res) );
	    listfld->box()->setCurrentItem( entrylist->selected()->name() );
	    const_cast<uiIOObjSelGrp*>( this )->selectionChange(0);
	}
    }
}


bool uiIOObjSelGrp::processInput()
{
    selectionChange( 0 );
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
	    BufferStringSet nms; getIOObjNames( *entrylist, nms );
	    listfld->box()->addItems( nms );
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


bool uiIOObjSelGrp::createEntry( const char* seltxt )
{
    ioobj = mkEntry( ctio, seltxt );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot create object with this name" );
	return false;
    }

    return true;
}


uiIOObjSelDlg::uiIOObjSelDlg( uiParent* p, const CtxtIOObj& c,
			      const char* seltxt, bool multisel )
	: uiIOObjRetDlg(p,
		Setup(c.ctxt.forread?"Input selection":"Output selection",
		    	"","8.1.1")
		.nrstatusflds(multisel?0:1))
	, selgrp( 0 )
{
    const bool ismultisel = multisel && c.ctxt.forread;
    selgrp = new uiIOObjSelGrp( this, c, seltxt, multisel );
    if ( !ismultisel )
    {
	statusBar()->setTxtAlign( 0, uiStatusBar::Right );
	selgrp->newstatusmessage.notify( mCB(this, uiIOObjSelDlg, statusMsgCB));
    }

    BufferString nm( "Select " );
    nm += c.ctxt.forread ? "input " : "output ";
    nm += c.ctxt.trgroup->userName();
    if ( ismultisel ) nm += "(s)";
    setTitleText( nm );
    setOkText( "&Ok (Select)" );
    finaliseDone.notify( mCB(this,uiIOObjSelDlg,setInitial) );
    selgrp->getListField()->box()->doubleClicked.notify(
	    mCB(this,uiDialog,accept) );
}


void uiIOObjSelDlg::setInitial( CallBacker* )
{
    const char* presetnm = selgrp->getNameField()
			 ? selgrp->getNameField()->text() : "";
    if ( *presetnm )
    {
	if ( !selgrp->getListField()->box()->isPresent( presetnm ) )
	    return;
	else
	    selgrp->getListField()->box()->setCurrentItem( presetnm );
    }
    selgrp->selectionChange( 0 );
}


void uiIOObjSelDlg::statusMsgCB( CallBacker* )
{
    toStatusBar( selgrp->statusmessage );
}


uiIOObjSel::uiIOObjSel( uiParent* p, CtxtIOObj& c, const char* txt,
			bool wclr, const char* st, const char* buttxt, 
			bool keepmytxt )
	: uiIOSelect( p, mCB(this,uiIOObjSel,doObjSel),
		      txt ? txt : (const char*)c.ctxt.trgroup->userName(), 
		      wclr, buttxt, keepmytxt )
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
    {
	setInput( (const char*)ctio.ioobj->key() );
	return;
    }

    setInput( "" );
}


const char* uiIOObjSel::userNameFromKey( const char* ky ) const
{
    static BufferString nm;
    nm = "";
    if ( ky && *ky )
	nm = IOM().nameOf( ky, false );
    return (const char*)nm;
}


void uiIOObjSel::obtainIOObj()
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
}


void uiIOObjSel::processInput()
{
    obtainIOObj();
    if ( ctio.ioobj || ctio.ctxt.forread )
	updateInput();
}


bool uiIOObjSel::existingUsrName( const char* nm ) const
{
    IOM().to( ctio.ctxt.stdSelKey() );
    return (*IOM().dirPtr())[nm];
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
    const char* curinp = getInput();
    if ( curinp && *curinp && dlg->selGrp() && dlg->selGrp()->getNameField() )
	dlg->selGrp()->getNameField()->setText( curinp );

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
