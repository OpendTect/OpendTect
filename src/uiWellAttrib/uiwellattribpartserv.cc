/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.cc,v 1.3 2004-04-21 14:14:34 nanne Exp $
________________________________________________________________________

-*/


#include "uiwellattribpartserv.h"
#include "wellman.h"
#include "nlamodel.h"
#include "attribdescset.h"
#include "uiwellattribsel.h"

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



uiWellAttribPartServer::uiWellAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset(new AttribDescSet)
    , nlamodel(0)
{
}


uiWellAttribPartServer::~uiWellAttribPartServer()
{
    delete attrset;
}


void uiWellAttribPartServer::setAttribSet( const AttribDescSet& ads )
{
    delete attrset;
    attrset = ads.clone();
}


void uiWellAttribPartServer::setNLAModel( const NLAModel* mdl )
{
    nlamodel = mdl;
}

#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiWellAttribPartServer::selectAttribute( const MultiID& wellid )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) mErrRet("Cannot read well data")

    uiWellAttribSel dlg( appserv().parent(), *wd, *attrset, nlamodel );
    if ( !dlg.go() )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj ) mErrRet("Cannot find well in object manager")

    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    if ( !iostrm ) mErrRet("Cannot create stream for this well")

    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname( sp.fileName() );
    Well::Writer wtr( fname, *wd );
   
    int lognr = dlg.newLogIdx() + 1;
    BufferString logfnm = wtr.getFileName( Well::IO::sExtLog, lognr );
    StreamProvider splog( logfnm );
    StreamData sdo = splog.makeOStream();
    wtr.putLog( *sdo.ostrm, wd->logs().getLog(lognr-1) );
    sdo.close();

    return true;
}
