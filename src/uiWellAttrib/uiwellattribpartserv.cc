/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwellattribpartserv.h"
#include "wellman.h"
#include "nlamodel.h"
#include "attribdescset.h"
#include "uicreateattriblogdlg.h"
#include "uicreatelogcubedlg.h"
#include "uiwellattribxplot.h"
#include "uiwellimpsegyvsp.h"
#include "uiwelltiemgrdlg.h"
#include "uisegyread.h"
#include "uichecklist.h"

#include "datapointset.h"
#include "ptrman.h"
#include "ioman.h"
#include "randcolor.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welltiesetup.h"
#include "uimsg.h"


uiWellAttribPartServer::uiWellAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset(new Attrib::DescSet(false)) //Default, set afterwards
    , nlamodel(0)
    , xplotwin2d_(0)
    , xplotwin3d_(0)
    , dpsdispmgr_(0)
    , welltiedlg_(0)
    , cursegyread_(0)
    , welltiedlgopened_(false)
{
    IOM().surveyChanged.notify(
	    mCB(this,uiWellAttribPartServer,surveyChangedCB) );
}


uiWellAttribPartServer::~uiWellAttribPartServer()
{
    delete attrset;
    delete xplotwin2d_;
    delete xplotwin3d_;
}



void uiWellAttribPartServer::surveyChangedCB( CallBacker* )
{
    delete xplotwin2d_; xplotwin2d_=0;
    delete xplotwin3d_; xplotwin3d_=0;
}


void uiWellAttribPartServer::setAttribSet( const Attrib::DescSet& ads )
{
    delete attrset;
    attrset = new Attrib::DescSet( ads );
}


void uiWellAttribPartServer::setNLAModel( const NLAModel* mdl )
{
    nlamodel = mdl;
}


void uiWellAttribPartServer::doXPlot()
{
    const bool is2d = attrset->is2D();

    uiWellAttribCrossPlot*& xplotwin = is2d ? xplotwin2d_ : xplotwin3d_;
    if ( !xplotwin )
	xplotwin = new uiWellAttribCrossPlot( parent(), attrset );
    else
	xplotwin->setDescSet( attrset );

    xplotwin->setDisplayMgr( dpsdispmgr_ );
    xplotwin->show();
}


void uiWellAttribPartServer::doSEGYTool( IOPar* previop, int choice )
{
    if ( choice < 0 )
    {
	uiDialog dlg( parent(), uiDialog::Setup("SEG-Y Tool",
	    previop ? "Import more data?" : "What do you want to do?",
	    mTODOHelpID) );
	BufferStringSet choices;
	choices.add( "&Import SEG-Y file(s) to OpendTect data" );
	choices.add( "&Scan SEG-Y file(s) to use in-place" );
	choices.add( "Import &VSP data from SEG-Y file" );
	choices.add( previop ? "&Quit SEG-Y import"
			     : "&Cancel the operation" );
	uiCheckList* choicefld = new uiCheckList( &dlg, choices,
						  uiCheckList::OneOnly );
	choicefld->setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice = choicefld->firstChecked();
    }

    switch ( choice )
    {
    case 0: case 1:
	if ( cursegyread_ )
	    { cursegyread_->raiseCurrent(); return; }
	cursegyread_ = 0;
	if ( !launchSEGYWiz(previop,choice) )
	    return;
    break;
    case 2:
	doVSPTool( previop );
	return;
    break;
    default:
    return;
    }
}


void uiWellAttribPartServer::doVSPTool( IOPar* previop, int choice )
{
    if ( choice < 0 )
    {
	uiDialog dlg( parent(), uiDialog::Setup("VSP Import",
					mNoDlgTitle,mTODOHelpID) );
	BufferStringSet choices;
	choices.add( "&2-D VSP (will be imported as 2-D seismic line)" );
	choices.add( "&3-D VSP (can only be imported as 3D cube)" );
	choices.add( "&Zero-offset (single trace) VSP" );
	choices.add( previop ? "&Quit import" : "&Cancel the operation" );
	uiCheckList* choicefld = new uiCheckList( &dlg, choices,
						  uiCheckList::OneOnly );
	choicefld->setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice = choicefld->firstChecked();
    }

    switch ( choice )
    {
	case 0: case 1: {
	    const Seis::GeomType gt( choice == 0 ? Seis::Line : Seis::Vol );
	    IOPar iop; if ( previop ) iop = *previop;
	    putInPar( gt, iop );
	    if ( !launchSEGYWiz(&iop,0) )
		return;
	break; }
	case 2: {
	    uiWellImportSEGYVSP dlg( parent() );
	    if ( !dlg.go() )
		return;
	break; }
	default:
	    return;
    }
}


bool uiWellAttribPartServer::launchSEGYWiz( IOPar* previop, int choice )
{
    if ( cursegyread_ )
    {
	uiMSG().error( "Please finish your current SEG-Y import first" );
	cursegyread_->raiseCurrent();
	return false;
    }

    switch ( choice )
    {
	case 0:
	    cursegyread_ = new uiSEGYRead( parent(),
				uiSEGYRead::Setup(uiSEGYRead::Import) );
	break;
	case 1: {
	    uiSEGYRead::Setup su( uiSEGYRead::DirectDef );
	    su.geoms_ -= Seis::Line;
	    cursegyread_ = new uiSEGYRead( parent(), su );
	break; }
	default:
	    return false;
    }

    if ( previop )
	cursegyread_->pars().merge( *previop );
    cursegyread_->processEnded.notify(
			mCB(this,uiWellAttribPartServer,segyToolEnded) );
    return true;
}


void uiWellAttribPartServer::segyToolEnded( CallBacker* )
{
    if ( !cursegyread_ || cursegyread_->state() == uiVarWizard::cCancelled() )
	return;

    IOPar previop( cursegyread_->pars() );
    cursegyread_ = 0;

    doSEGYTool( &previop );
}


bool uiWellAttribPartServer::createAttribLog( const MultiID& wellid )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd )
    {
	uiMSG().error( "Cannot read well data" );
	return false;
    }

    BufferStringSet wellnames;
    wellnames.add( wd->name() );
    return createAttribLog( wellnames );
}


bool uiWellAttribPartServer::createAttribLog( const BufferStringSet& wellnames )
{
    uiCreateAttribLogDlg dlg( appserv().parent(), wellnames, attrset,
			      nlamodel, wellnames.size() == 1 );
    return dlg.go();
}


bool uiWellAttribPartServer::createLogCube( const MultiID& wellid )
{
    uiCreateLogCubeDlg dlg( parent(), &wellid );
    return dlg.go();
}


bool uiWellAttribPartServer::createD2TModel( const MultiID& wid )
{
    WellTie::Setup* wtsetup = new WellTie::Setup();
    wtsetup->wellid_ = wid;
    if ( !welltiedlgopened_ )
    {
	welltiedlg_ = new WellTie::uiTieWinMGRDlg( parent(), *wtsetup );
	welltiedlg_->windowClosed.notify(
		mCB(this,uiWellAttribPartServer,closeWellTieDlg));
	welltiedlgopened_ = welltiedlg_->go();
    }
    return true;
}

void uiWellAttribPartServer::closeWellTieDlg( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWinMGRDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    dlg->delWins();
    welltiedlgopened_ = false;
}

