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


void uiWellAttribPartServer::importSEGYVSP()
{
    uiWellImportSEGYVSP dlg( parent() );
    dlg.go();
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

