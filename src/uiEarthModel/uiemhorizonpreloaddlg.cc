/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiempreloaddlg.h"

#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uiioobjseldlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "uitextedit.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "emhorizonpreload.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "filepath.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "multiid.h"
#include "preloads.h"
#include "ptrman.h"
#include "survinfo.h"
#include "strmprov.h"
#include "transl.h"
#include "od_helpids.h"


// uiEMPreLoadDlg
uiEMPreLoadDlg::uiEMPreLoadDlg( uiParent* p, const Setup& s )
    : uiDialog(p,s)
{}


// uiHorizonPreLoadDlg
uiHorizonPreLoadDlg::uiHorizonPreLoadDlg( uiParent* p )
    : uiEMPreLoadDlg( p,uiDialog::Setup( tr("Horizon Pre-load Manager"),
		     mNoDlgTitle, mODHelpKey(mHorizonPreLoadDlgHelpID)) )
{
    setCtrlStyle( CloseOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries", OD::ChooseAtLeastOne );
    listfld_->selectionChanged.notify(mCB(this,uiHorizonPreLoadDlg,selCB) );

    uiButtonGroup* butgrp = new uiButtonGroup( listfld_, "Manip buttons",
					       OD::Vertical );
    butgrp->attach( rightOf, listfld_->box() );

    if ( SI().has2D() )
    {
	uiPushButton* add2dbut =
	    new uiPushButton( butgrp, uiStrings::phrLoad(uiStrings::s2D()),
			      mCB(this,uiHorizonPreLoadDlg,add2DPushCB), false);
	add2dbut->setIcon( "tree-horizon2d" );
    }

    uiPushButton* add3dbut =
	new uiPushButton( butgrp, uiStrings::phrLoad(uiStrings::s3D()),
			  mCB(this,uiHorizonPreLoadDlg,add3DPushCB), false );
    add3dbut->setIcon( "tree-horizon3d" );

    unloadbut_ = new uiPushButton( butgrp, tr("Unload Checked"),
		mCB(this,uiHorizonPreLoadDlg,unloadPushCB), false );
    unloadbut_->setIcon( "unload" );

    savebut_ = new uiToolButton( listfld_, "save",
	tr("Save pre-loads"), mCB(this,uiHorizonPreLoadDlg,savePushCB) );
    savebut_->attach( rightAlignedAbove, listfld_->box() );
    uiToolButton* opentb = new uiToolButton( listfld_, "open",
	tr("Retrieve pre-loads"), mCB(this,uiHorizonPreLoadDlg,openPushCB) );
    opentb->attach( leftOf, savebut_ );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info", true );
    infofld_->setPrefHeightInChar( 7 );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( topgrp );
    spl->addGroup( infogrp );

    EM::HorizonPreLoader& hpl = EM::HPreL();
    const BufferStringSet& preloadnames = hpl.getPreloadedNames();
    if ( !preloadnames.isEmpty() )
	listfld_->addItems( preloadnames );

    selCB( 0 );
}


void uiHorizonPreLoadDlg::add3DPushCB( CallBacker* )
{
    loadHorizon( false );
}


void uiHorizonPreLoadDlg::add2DPushCB( CallBacker* )
{
    loadHorizon( true );
}


bool uiHorizonPreLoadDlg::loadHorizon( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = is2d ? mMkCtxtIOObj(EMHorizon2D)
				  : mMkCtxtIOObj(EMHorizon3D);

    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg hordlg( this, sdsu, *ctio );
    if ( !hordlg.go() || !hordlg.ioObj() || hordlg.nrChosen() < 1 )
	return false;

    EM::HorizonPreLoader& hpl = EM::HPreL();
    TypeSet<MultiID> selmids;
    hordlg.getChosen( selmids );

    uiTaskRunner taskrunner( this );
    hpl.load( selmids, &taskrunner );
    uiMSG().message( mToUiStringTodo(hpl.errorMsg()) );
    listfld_->setEmpty();
    listfld_->addItems( hpl.getPreloadedNames() );
    listfld_->setCurrentItem( 0 );

    return true;
}


void uiHorizonPreLoadDlg::unloadPushCB( CallBacker* )
{
    BufferStringSet selhornms;
    listfld_->getChosen( selhornms );
    if ( selhornms.isEmpty() )
	return;

    uiString msg = tr( "Unload checked horizon(s)?\n"
		  "(This will not delete the file(s) from disk)", 0,
		  selhornms.size() );

    if ( !uiMSG().askGoOn( msg ) )
	return;

    EM::HorizonPreLoader& hpl = EM::HPreL();
    hpl.unload( selhornms );
    listfld_->setEmpty();
    const BufferStringSet& names = hpl.getPreloadedNames();
    if ( !names.isEmpty() )
	listfld_->addItems( names );
}


void uiHorizonPreLoadDlg::selCB( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 )
    {
	unloadbut_->setSensitive( false );
	savebut_->setSensitive( false );
	infofld_->setText( "No pre-loaded horizons" );
	return;
    }

    unloadbut_->setSensitive( true );
    savebut_->setSensitive( true );
    EM::HorizonPreLoader& hpl = EM::HPreL();
    const MultiID& mid = hpl.getMultiID( listfld_->textOfItem(selidx) );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return;

    BufferString type( EM::EMM().objectType(mid) );
    BufferString info;
    info.add( "Data Type: " ).add( type ).add( "\n" );
    FilePath fp( ioobj->fullUserExpr(true) );
    info.add( "Location: " ).add( fp.pathOnly() ).add ( "\n" );
    info.add( "File: " ).add( fp.fileName() ).add( "\n" );
    info.add( "File size in KB: " )
	.add( File::getKbSize(ioobj->fullUserExpr(true)) );
    infofld_->setText( info );
}


void uiHorizonPreLoadDlg::openPushCB( CallBacker* )
{
    CtxtIOObj ctio( PreLoadSurfacesTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    uiIOObjSelDlg hordlg( this, ctio, uiStrings::phrJoinStrings(
				uiStrings::sOpen(),tr("pre-loaded settings")) );
    if ( !hordlg.go() || !hordlg.ioObj() )
	return;

    const BufferString fnm( hordlg.ioObj()->fullUserExpr(true) );
    od_istream strm( fnm );
    ascistream astrm( strm, true );
    IOPar fulliop( astrm );
    if ( fulliop.isEmpty() )
	{ uiMSG().message( tr("No valid objects found") ); return; }

    PtrMan<IOPar> par = fulliop.subselect( "Hor" );
    TypeSet<MultiID> selmids;
    for ( int idx=0; idx<par->size(); idx++ )
    {
	PtrMan<IOPar> multiidpar = par->subselect( idx );
	if ( !multiidpar )
	    continue;

	MultiID mid;
	multiidpar->get( sKey::ID(), mid );
	if ( mid.isUdf() )
	    continue;

	selmids += mid;
    }

    if ( selmids.isEmpty() )
	return;

    loadSavedHorizon( selmids );
}


void uiHorizonPreLoadDlg::loadSavedHorizon( const TypeSet<MultiID>& savedmids )
{
    if ( savedmids.isEmpty() )
	return;

    uiTaskRunner taskrunner( this );
    EM::HorizonPreLoader& hpl = EM::HPreL();
    hpl.load( savedmids, &taskrunner );
    uiMSG().message( mToUiStringTodo(hpl.errorMsg()) );
    listfld_->setEmpty();
    BufferStringSet hornms = hpl.getPreloadedNames();
    if ( !hornms.isEmpty() )
	listfld_->addItems( hornms );
}


void uiHorizonPreLoadDlg::savePushCB( CallBacker* )
{
    CtxtIOObj ctio( PreLoadSurfacesTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = false;
    uiIOObjSelDlg hordlg( this, ctio, uiStrings::phrJoinStrings(
			  uiStrings::sSave(), tr("pre-loaded settings")) );
    if ( !hordlg.go() || !hordlg.ioObj() )
	return;

    const BufferString fnm( hordlg.ioObj()->fullUserExpr(true) );
    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().message(tr("Cannot open output file:\n%1").arg(fnm));
	return;
    }

    IOPar par;
    EM::HorizonPreLoader& hpl = EM::HPreL();
    const BufferStringSet& hornames = hpl.getPreloadedNames();
    const int size = hornames.size();
    for ( int lhidx=0; lhidx<size; lhidx++ )
    {
	const MultiID& mid = hpl.getMultiID( hornames.get( lhidx ) );
	if ( mid.isUdf() )
	    continue;

	BufferString key( "Hor." , lhidx, ".ID" );
	par.set( key, mid );
    }

    ascostream astrm( strm );
    if ( !astrm.putHeader("Pre-loads") )
    {
	uiMSG().message(tr("Cannot write to output file:\n%1").arg(fnm));
	return;
    }

    par.putTo( astrm );
}
