/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.cc,v 1.2 2004-03-18 10:13:33 nanne Exp $
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


bool uiWellAttribPartServer::selectAttribute( const MultiID& wellid )
{
    // TODO: error messages
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) return false;

    uiWellAttribSel dlg( appserv().parent(), *wd, *attrset, nlamodel );
    bool ret = dlg.go();
    if ( !ret ) return false;

    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj ) return false;
    Translator* tr = ioobj->getTranslator();

    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    if ( !iostrm ) return false;
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
