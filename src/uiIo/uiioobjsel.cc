/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          25/05/2000
 RCS:           $Id: uiioobjsel.cc,v 1.50 2003-05-12 16:15:23 bert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "iodirentry.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilistbox.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"
#include "ioman.h"
#include "iostrm.h"
#include "iolink.h"
#include "iodir.h"
#include "iopar.h"
#include "transl.h"
#include "ptrman.h"
#include "filegen.h"
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
	: uiIOObjRetDlg(p,c.ctxt.forread?"Input selection":"Output selection")
	, ctio(c)
	, nmfld(0)
	, ioobj(0)
	, ismultisel(multisel && ctio.ctxt.forread)
{
    BufferString nm( "Select " );
    nm += ctio.ctxt.forread ? "input " : "output ";
    nm += ctio.ctxt.trgroup->name();
    if ( ismultisel ) nm += "(s)";
    setTitleText( nm );

    IOM().to( MultiID(IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id) );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    if ( ctio.ioobj )
        entrylist->setSelected( ctio.ioobj->key() );

    entrylist->setName( seltxt );
    listfld = new uiLabeledListBox( this, entrylist->Ptr() );
    if ( ismultisel )
	listfld->box()->setMultiSelect( true );
    listfld->box()->setPrefWidthInChar( 
		listfld->box()->optimumFieldWidth(20,60) );

    if ( !ctio.ctxt.forread )
    {
	nmfld = new uiGenInput( this, "Name" );
	nmfld->attach( alignedBelow, listfld );
	nmfld->setStretch( 2, 0 );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiIOObjSelDlg,selChg) );
    listfld->box()->doubleClicked.notify( mCB(this,uiDialog,accept) );
    if ( ctio.ctxt.maydooper )
    {
	uiButtonGroup* bg = new uiButtonGroup( this, "" );
	const ioPixmap locpm( GetDataFileName("filelocation.png") );
	const ioPixmap renpm( GetDataFileName("renameobj.png") );
	const ioPixmap rempm( GetDataFileName("trashcan.png") );
	const ioPixmap ropm( GetDataFileName("readonly.png") );
	locbut = new uiToolButton( bg, "File loc TB", locpm );
	renbut = new uiToolButton( bg, "Obj rename TB", renpm );
	rembut = new uiToolButton( bg, "Remove", rempm );
	robut = new uiToolButton( bg, "Readonly togg", ropm );
	locbut->activated.notify( mCB(this,uiIOObjSelDlg,tbPush) );
	renbut->activated.notify( mCB(this,uiIOObjSelDlg,tbPush) );
	rembut->activated.notify( mCB(this,uiIOObjSelDlg,tbPush) );
	robut->activated.notify( mCB(this,uiIOObjSelDlg,tbPush) );
	bg->attach( rightOf, listfld );
	locbut->setToolTip( "Change location on disk" );
	renbut->setToolTip( "Rename this object" );
	rembut->setToolTip( "Remove this object" );
	robut->setToolTip( "Toggle read-only" );
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


void uiIOObjSelDlg::selChg( CallBacker* c )
{
    if ( ismultisel ) return;
    const int curitm = listfld->box()->currentItem();
    ioobj = 0;
    if ( curitm >= 0 )
    {
	entrylist->setCurrent( listfld->box()->currentItem() );
	ioobj = entrylist->selected();
	if ( c && nmfld )
	    nmfld->setText( ioobj ? (const char*)ioobj->name() : "" );
    }
    mDynamicCastGet(IOStream*,iostrm,ioobj)

    locbut->setSensitive( iostrm );
    robut->setSensitive( iostrm );
    //TODO set state according ro read-only status
    renbut->setSensitive( ioobj );
    rembut->setSensitive( ioobj );
}


void uiIOObjSelDlg::tbPush( CallBacker* c )
{
    if ( !ioobj ) return;
    mDynamicCastGet(uiToolButton*,tb,c)
    if ( !tb ) { pErrMsg("CallBacker is not uiToolButton!"); return; }

    const int curitm = listfld->box()->currentItem();
    MultiID prevkey( ioobj->key() );

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm && (tb == robut || tb == locbut) )
	{ pErrMsg("IOStream button but not IOStream!"); return; }

    PtrMan<Translator> tr = ioobj->getTranslator();

    bool chgd = false;
    if ( tb == locbut )
	chgd = chgEntry( tr );
    else if ( tb == robut )
	chgd = roEntry( tr );
    else if ( tb == renbut )
	chgd = renEntry( tr );
    else if ( tb == rembut )
    {
	chgd = rmEntry( tr ? tr->implRemovable(ioobj) : ioobj->implRemovable());
	if ( chgd ) prevkey = "";
    }
    if ( !chgd ) return;

    entrylist->fill( IOM().dirPtr() );
    if ( prevkey != "" )
	entrylist->setSelected( prevkey );
    else
    {
	const int newcur = curitm >= entrylist->size() ? curitm - 1 : curitm;
	if ( newcur >= 0 )
	    entrylist->setCurrent( newcur );
    }

    listfld->box()->empty();
    listfld->box()->addItems( entrylist->Ptr() );

    selChg(c);
}


bool uiIOObjSelDlg::renImpl( Translator* tr, IOStream& iostrm,
			     IOStream& chiostrm )
{
    const bool oldimplexist = tr ? tr->implExists(&iostrm,true)
				 : iostrm.implExists(true);
    BufferString newfname( chiostrm.fullUserExpr(true) );

    if ( oldimplexist )
    {
	const bool newimplexist = tr ? tr->implExists(&chiostrm,true)
				     : chiostrm.implExists(true);
	if ( newimplexist && !uiRmIOObjImpl( chiostrm, true ) )
	    return false;

	if ( tr )
	    tr->implRename( &iostrm, newfname );
	else
	    iostrm.implRename( newfname );
    }

    iostrm.setFileName( newfname );
    return true;
}


bool uiIOObjSelDlg::renEntry( Translator* tr )
{
    BufferString titl( "Rename '" );
    titl += ioobj->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
	    		new StringInpSpec(ioobj->name()) );
    if ( !dlg.go() ) return false;

    BufferString newnm = dlg.text();
    if ( newnm == ioobj->name() )
	return false;
    ioobj->setName( newnm );

    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm && iostrm->implExists(true) )
    {
	IOStream chiostrm;
	chiostrm.copyFrom( iostrm );
	if ( tr )
	    chiostrm.setExt( tr->defExtension() );
	chiostrm.genDefaultImpl();
	if ( !renImpl(tr,*iostrm,chiostrm) )
	    return false;

	iostrm->copyFrom( &chiostrm );
    }

    IOM().commitChanges( *ioobj );
    return true;
}


bool uiRmIOObjImpl( IOObj& ioob, bool askexist )
{
    BufferString mess = "Remove ";
    if ( askexist ) mess += " existing ";
    mess += "'";
    if ( !ioob.isLink() )
	{ mess += ioob.fullUserExpr(true); mess += "'?"; }
    else
    {
	FileNameString fullexpr( ioob.fullUserExpr(true) );
	mess += File_getFileName(fullexpr);
	mess += "'\n- and everything in it! - ?";
    }
    if ( !uiMSG().askGoOn(mess) )
	return false;

    if ( !fullImplRemove(ioob) )
    {
	BufferString mess = "Could not remove '";
	mess += ioob.fullUserExpr(true);
	mess += "'\nRemove entry from list anyway?";
	if ( !uiMSG().askGoOn(mess) )
	    return false;
    }
    return true;
}


bool uiIOObjSelDlg::rmEntry( bool rmabl )
{
    if ( rmabl && !uiRmIOObjImpl( *ioobj, false ) )
	return false;

    entrylist->curRemoved();
    IOM().permRemove( ioobj->key() );
    return true;
}


bool uiIOObjSelDlg::chgEntry( Translator* tr )
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    BufferString caption( "New file/location for '" );
    caption += ioobj->name(); caption += "'";
    BufferString oldfnm( iostrm->fullUserExpr(true) );
    BufferString defext( ctio.ctxt.trgroup->defExtension() );
    BufferString filefilt( "*" );
    if ( defext != "" )
    {
	filefilt += "."; filefilt += defext;
	filefilt += ";;*";
    }
    uiFileDialog dlg( this, uiFileDialog::AnyFile, oldfnm, filefilt, caption );
    if ( !dlg.go() ) return false;

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    BufferString newfnm( dlg.fileName() );
    chiostrm.setFileName( newfnm );
    if ( !renImpl(tr,*iostrm,chiostrm) )
	return false;

    IOM().commitChanges( *iostrm );
    return true;
}


bool uiIOObjSelDlg::roEntry( Translator* tr )
{
    //TODO implement
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return false;
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
