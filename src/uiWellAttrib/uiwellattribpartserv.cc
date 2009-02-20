/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellattribpartserv.cc,v 1.14 2009-02-20 11:34:18 cvsbruno Exp $";


#include "uiwellattribpartserv.h"
#include "wellman.h"
#include "nlamodel.h"
#include "attribdescset.h"
#include "uicreateattriblogdlg.h"
#include "uiwellattribxplot.h"
#include "uiwellimpsegyvsp.h"
#include "uid2tmodelgenwin.h"
#include "uid2tmlogseldlg.h"

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
#include "uimsg.h"

const int uiWellAttribPartServer::evShowPickSet()	{ return 0; }


uiWellAttribPartServer::uiWellAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset(new Attrib::DescSet(false)) //Default, set afterwards
    , nlamodel(0)
    , xplotwin2d_(0)
    , xplotwin3d_(0)
    , selpickset_(0)
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
    attrset = ads.clone();
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
	    mCB(this,uiWellAttribPartServer,showPickSet) );
    xplotwin->show();
}


void uiWellAttribPartServer::showPickSet( CallBacker* )
{
    selpickset_ = attrset->is2D() ? xplotwin2d_->getSelectedPts() :
			 	    xplotwin3d_->getSelectedPts();
    sendEvent( evShowPickSet() );
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


bool uiWellAttribPartServer::createD2TModel( const MultiID& mid )
{
    MultiID wid = mid;
    BufferString attrname, logname1, logname2;
    uiD2TMLogSelDlg* dlg = new uiD2TMLogSelDlg ( parent(), wid, *attrset );
    if ( dlg->go() )
    {
	attrname = dlg -> attrname_;
	logname1 = dlg -> logname1_; logname2 = dlg -> logname2_;
	wid = dlg -> wellid_;
	Wavelet* wvlt = dlg -> wavelet_;

	BufferString wname;
	wname = "Tie ";
	wname += Well::MGR().get(wid)->name();
	wname += " to ";
	wname += attrname;

	if ( !logname1.isEmpty() && !logname2.isEmpty() )
	    uid2tmgenwin_ = new uiD2TModelGenWin( parent(), wid, logname1,
		    logname2, wname, attrname, *attrset, dlg->wavelet_ );
	else
	    uid2tmgenwin_ = new uiD2TModelGenWin( parent(), wid, "Sonic",
		    "Density", wname, attrname, *attrset, dlg->wavelet_ );
	    //TODO : replace Sonic/Density by variable names
	return true;
    }
    else
	return false;
}


