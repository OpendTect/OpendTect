/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellattribpartserv.cc,v 1.22 2009-07-22 16:01:44 cvsbert Exp $";


#include "uiwellattribpartserv.h"
#include "wellman.h"
#include "nlamodel.h"
#include "attribdescset.h"
#include "uicreateattriblogdlg.h"
#include "uiwellattribxplot.h"
#include "uiwellimpsegyvsp.h"
#include "uiwelltiemgrdlg.h"

#include "datapointset.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "iostrm.h"
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
    , dpsid_(-1)
    , dpsdispmgr_(0)
{
}


uiWellAttribPartServer::~uiWellAttribPartServer()
{
    delete attrset;
    delete xplotwin2d_;
    delete xplotwin3d_;
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
	xplotwin = new uiWellAttribCrossPlot( parent(), *attrset );
    else
	xplotwin->setDescSet( *attrset );

    xplotwin->pointsSelected.notify(
	    mCB(this,uiWellAttribPartServer,showSelPts) );
    xplotwin->pointsToBeRemoved.notify(
	    mCB(this,uiWellAttribPartServer,removeSelPts) );
    xplotwin->show();
}


void uiWellAttribPartServer::removeSelPts( CallBacker* )
{
    if ( dpsdispmgr_ )
	dpsdispmgr_->removeDisplay( dpsid_ );
    dpsid_ = -1;
}


void uiWellAttribPartServer::showSelPts( CallBacker* )
{
    const DataPointSet& dps = attrset->is2D() ? xplotwin2d_->getDPS()
					      : xplotwin3d_->getDPS();
    if ( !dpsdispmgr_ ) return;

    dpsdispmgr_->lock();
    if ( dpsid_ < 0 )
	dpsid_ = dpsdispmgr_->addDisplay( dpsdispmgr_->availableParents(), dps);
    else
	dpsdispmgr_->updateDisplay( dpsid_, dpsdispmgr_->availableParents(),
				    dps );
    dpsdispmgr_->unLock();
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

    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    if ( !iostrm ) mErrRet("Cannot create stream for this well")

    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname( sp.fileName() );
    Well::Writer wtr( fname, *wd );
 
    if ( lognr > wd->logs().size() - 1 )
	lognr =  wd->logs().size() - 1;
    BufferString logfnm = wtr.getFileName( Well::IO::sExtLog(), lognr + 1 );
    StreamProvider splog( logfnm );
    StreamData sdo = splog.makeOStream();
    wtr.putLog( *sdo.ostrm, wd->logs().getLog(lognr) );
    sdo.close();

    return true;
}


bool uiWellAttribPartServer::createD2TModel( const MultiID& wid )
{
    WellTieSetup wtsetup;
    wtsetup.wellid_ = wid;
    
    uiWellTieMGRDlg dlg( parent(), wtsetup, *attrset );
    dlg.go();

    return true;
}

