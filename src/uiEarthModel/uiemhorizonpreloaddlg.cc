/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiempreloaddlg.h"

#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
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


uiHorizonPreLoadDlg::uiHorizonPreLoadDlg( uiParent* p )
    : uiEMPreLoadDlg(p)
{
    setCtrlStyle( LeaveOnly );
    listfld_ = new uiListBox( this, "Loaded entries", true );
    listfld_->selectionChanged.notify(mCB(this,uiHorizonPreLoadDlg,selCB) );

    uiToolButton* opentb = new uiToolButton( this, "openpreload",
	    "Retrieve pre-loads", mCB(this,uiHorizonPreLoadDlg,openPushCB) );
    opentb->attach( leftAlignedBelow, listfld_ );

    savebut_ = new uiToolButton( this, "savepreload", "Save pre-loads",
		     mCB(this,uiHorizonPreLoadDlg,savePushCB) );
    savebut_->attach( rightAlignedBelow, listfld_ );

    uiButtonGroup* butgrp = new uiButtonGroup( this, "Manip buttons" );
    butgrp->attach( rightOf, listfld_ );

    uiPushButton* add3dbut mUnusedVar =
	new uiPushButton( butgrp, "Add 3D Horizon",
			  mCB(this,uiHorizonPreLoadDlg,add3DPushCB), false );

    if ( SI().has2D() )
    {
	uiPushButton* add2dbut mUnusedVar =
	    new uiPushButton( butgrp, "Add 2D Horizon",
			      mCB(this,uiHorizonPreLoadDlg,add2DPushCB), false);
    }

    unloadbut_ = new uiPushButton( this, "Unload selected",
	    	mCB(this,uiHorizonPreLoadDlg,unloadPushCB), false );
    unloadbut_->attach( alignedBelow, butgrp );

    infofld_ = new uiTextEdit( this, "Info", true );
    infofld_->attach( alignedBelow, opentb );
    infofld_->setPrefWidthInChar( 60 );
    infofld_->setPrefHeightInChar( 7 );

    EM::HorizonPreLoader& hpl = EM::HPreL();
    const BufferStringSet& preloadnames = hpl.getPreloadedNames();
    if ( !preloadnames.isEmpty() )
	listfld_->addItems( preloadnames );

    selCB( 0 );
}


bool uiHorizonPreLoadDlg::add3DPushCB( CallBacker* )
{
    return loadHorizon( false );
}


bool uiHorizonPreLoadDlg::add2DPushCB( CallBacker* )
{
    return loadHorizon( true );
}


bool uiHorizonPreLoadDlg::loadHorizon( bool is2d )
{
    PtrMan<CtxtIOObj> ctio = is2d ? mMkCtxtIOObj(EMHorizon2D)
				  : mMkCtxtIOObj(EMHorizon3D);

    uiIOObjSelDlg hordlg( this, *ctio, "", true );
    if ( !hordlg.go() || !hordlg.ioObj() || hordlg.nrSel() <= 0 )
	return false;

    EM::HorizonPreLoader& hpl = EM::HPreL();
    TypeSet<MultiID> selmids;
    for ( int idx=0; idx<hordlg.nrSel(); idx++ )
    {
	const MultiID mid = hordlg.selected( idx );
	selmids += mid;
    }

    uiTaskRunner tr( this );
    hpl.load( selmids, &tr );
    uiMSG().message( hpl.errorMsg() );
    listfld_->setEmpty();
    listfld_->addItems( hpl.getPreloadedNames() );
    listfld_->setCurrentItem( 0 );

    return true;
}


void uiHorizonPreLoadDlg::unloadPushCB( CallBacker* )
{
    BufferStringSet selhornms;
    listfld_->getSelectedItems( selhornms );
    if ( selhornms.isEmpty() )
	return;

    BufferString msg( "Unload " );
    msg.add( "selected horizon(s)" )
       .add( "'?\n(This will not delete the object(s) from disk)" );
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
    info.add( "Directory: " ).add( fp.pathOnly() ).add ( "\n" );
    info.add( "File: " ).add( fp.fileName() ).add( "\n" );
    info.add( "File size in KB: " )
	.add( File::getKbSize(ioobj->fullUserExpr(true)) );
    infofld_->setText( info );
}


void uiHorizonPreLoadDlg::openPushCB( CallBacker* )
{
    CtxtIOObj ctio( PreLoadSurfacesTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg hordlg( this, ctio, "Open pre-loaded settings" );
    if ( !hordlg.go() || !hordlg.ioObj() )
	return;

    const BufferString fnm( hordlg.ioObj()->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeIStream() );
    ascistream astrm( *sd.istrm, true );
    IOPar fulliop( astrm );
    if ( fulliop.isEmpty() )
    {
	uiMSG().message( "No valid objects found" );
	return;
    }

    PtrMan<IOPar> par = fulliop.subselect( "Hor" );
    TypeSet<MultiID> selmids;
    for ( int idx=0; idx<par->size(); idx++ )
    {
	PtrMan<IOPar> multiidpar = par->subselect( idx );
	if ( !multiidpar )
	    continue;

	const char* id = multiidpar->find( sKey::ID() );
	if ( !id || !*id )
	    continue;

	const MultiID mid( id );
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

    uiTaskRunner tr( this );
    EM::HorizonPreLoader& hpl = EM::HPreL();
    hpl.load( savedmids, &tr );
    uiMSG().message( hpl.errorMsg() );
    listfld_->setEmpty();
    BufferStringSet hornms = hpl.getPreloadedNames();
    if ( !hornms.isEmpty() )
	listfld_->addItems( hornms );
}


void uiHorizonPreLoadDlg::savePushCB( CallBacker* )
{
    CtxtIOObj ctio( PreLoadSurfacesTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    uiIOObjSelDlg hordlg( this, ctio, "Save pre-loadedsettings" );
    if ( !hordlg.go() || !hordlg.ioObj() )
	return;

    const BufferString fnm( hordlg.ioObj()->fullUserExpr(true) );
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
    {
	uiMSG().message( "Cannot open output file:\n", fnm );
	return;
    }

    IOPar par;
    EM::HorizonPreLoader& hpl = EM::HPreL();
    const BufferStringSet& hornames = hpl.getPreloadedNames();
    const int size = hornames.size();
    for ( int lhidx=0; lhidx<size; lhidx++ )
    {
	const MultiID& mid = hpl.getMultiID( hornames.get( lhidx ) );
	if ( mid < 0 )
	    continue;

	BufferString key( "Hor." , lhidx, ".ID" );
	par.set( key, mid );
    }

    ascostream astrm( *sd.ostrm );
    if ( !astrm.putHeader("Pre-loads") )
    {
	uiMSG().message( "Cannot write to output file:\n", fnm );
	return;
    }

    par.putTo( astrm );
}
