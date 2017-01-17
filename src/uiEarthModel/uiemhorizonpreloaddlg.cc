/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/


#include "uiempreloaddlg.h"

#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uiioobjseldlg.h"
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
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "dbkey.h"
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
    listfld_ = new uiListBox( this, "Loaded entries", OD::ChooseAtLeastOne );
    listfld_->selectionChanged.notify(mCB(this,uiHorizonPreLoadDlg,selCB) );

    uiToolButton* opentb = new uiToolButton( this, "open",
    tr("Retrieve pre-loads"), mCB(this,uiHorizonPreLoadDlg,openPushCB) );
    opentb->attach( leftAlignedBelow, listfld_ );

    savebut_ = new uiToolButton( this, "save", uiStrings::phrSave(
							    tr("pre-load")),
		     mCB(this,uiHorizonPreLoadDlg,savePushCB) );
    savebut_->attach( rightAlignedBelow, listfld_ );

    uiButtonGroup* butgrp = new uiButtonGroup( this, "Manip buttons",
					       OD::Vertical );
    butgrp->attach( rightOf, listfld_ );

    if ( SI().has2D() )
    {
	uiPushButton* add2dbut mUnusedVar =
	    new uiPushButton( butgrp, uiStrings::phrLoad(uiStrings::s2D()) ,
			      mCB(this,uiHorizonPreLoadDlg,add2DPushCB), false);
    }

    uiPushButton* add3dbut mUnusedVar =
	new uiPushButton( butgrp, uiStrings::phrLoad(uiStrings::s3D()),
			  mCB(this,uiHorizonPreLoadDlg,add3DPushCB), false );

    unloadbut_ = new uiPushButton( this, tr("Unload Checked"),
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
    DBKeySet selmids;
    hordlg.getChosen( selmids );

    uiTaskRunner taskrunner( this );
    hpl.load( selmids, &taskrunner );
    uiMSG().message( hpl.errorMsg() );
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
    const DBKey& mid = hpl.getDBKey( listfld_->textOfItem(selidx) );
    PtrMan<IOObj> ioobj = DBM().get( mid );
    if ( !ioobj )
	return;

    EM::EMManager& emmgr = EM::getMgr( mid );
    BufferString type( emmgr.objectType(mid) );
    BufferString info;
    info.add( "Data Type: " ).add( type ).add( "\n" );
    File::Path fp( ioobj->fullUserExpr(true) );
    info.add( "Directory: " ).add( fp.pathOnly() ).add ( "\n" );
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
    DBKeySet selmids;
    for ( int idx=0; idx<par->size(); idx++ )
    {
	PtrMan<IOPar> dbkeypar = par->subselect( idx );
	if ( !dbkeypar )
	    continue;

	const char* idstr = dbkeypar->find( sKey::ID() );
	if ( !DBKey::isValidString(idstr) )
	    continue;

	selmids += DBKey::getFromString( idstr );
    }

    if ( selmids.isEmpty() )
	return;

    loadSavedHorizon( selmids );
}


void uiHorizonPreLoadDlg::loadSavedHorizon( const DBKeySet& savedmids )
{
    if ( savedmids.isEmpty() )
	return;

    uiTaskRunner taskrunner( this );
    EM::HorizonPreLoader& hpl = EM::HPreL();
    hpl.load( savedmids, &taskrunner );
    uiMSG().message( hpl.errorMsg() );
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
	const DBKey mid = hpl.getDBKey( hornames.get( lhidx ) );
	if ( mid.isInvalid() )
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
