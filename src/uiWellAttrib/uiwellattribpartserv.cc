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
#include "ioobj.h"
#include "ioman.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "wellwriter.h"
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


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiWellAttribPartServer::createAttribLog( const MultiID& wellid, int lognr )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) mErrRet("Cannot read well data")
    
    BufferStringSet wellname;
    wellname.add( wd->name() );

    if ( lognr<0 )
    {
	uiCreateAttribLogDlg dlg( appserv().parent(), wellname ,
				  attrset, nlamodel, true );
	dlg.go();
       	lognr = dlg.selectedLogIdx();
    }

    if ( lognr<0 )
	return false;
    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj ) mErrRet("Cannot find well in object manager")

    BufferString fname( ioobj->fullUserExpr(true) );
    Well::Writer wtr( fname, *wd );
 
    if ( lognr > wd->logs().size() - 1 )
	lognr = wd->logs().size() - 1;
    BufferString logfnm = wtr.getFileName( Well::IO::sExtLog(), lognr+1 );
    StreamData sdo = StreamProvider(logfnm).makeOStream();
    if ( !sdo.usable() )
    {
	BufferStringSet errmsg; errmsg.add( "Cannot write log to disk" );
	errmsg.add( logfnm );
	uiMSG().errorWithDetails( errmsg );
	return false;
    }

    wtr.putLog( *sdo.ostrm, wd->logs().getLog(lognr) );
    sdo.close();

    return true;
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

