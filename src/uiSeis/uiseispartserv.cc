/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.48 2007-01-15 10:58:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2dline.h"
#include "segposinfo.h"

#include "uiexecutor.h"
#include "uiflatvertview.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uisegysip.h"
#include "uiseiscbvsimp.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseissegyimpexp.h"
#include "uiseissel.h"
#include "uiseiswvltimp.h"
#include "uiseiswvltman.h"
#include "uiselsimple.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    	: uiApplPartServer(a)
    	, storedgathermenuitem("Display Gather")
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider( segyid );
    uiSurveyInfoEditor::addInfoProvider( sip );
    SeisIOObjInfo::initDefault( sKey::Steering );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    PtrMan<uiDialog> dlg = 0;
    if ( opt == 3 )
	dlg = new uiSeisImpCBVS( appserv().parent() );
    else
    {
	Seis::GeomType gt = opt == 0 ? Seis::Vol
	    		 : (opt == 1 ? Seis::Line
				     : Seis::VolPS);
	dlg = new uiSeisSegYImpExp( appserv().parent(), forread, segyid, gt );
    }
    return dlg->go();
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


void uiSeisPartServer::manageSeismics()
{
    uiSeisFileMan dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    uiSeisWvltImp dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::manageWavelets()
{
    uiSeisWvltMan dlg( appserv().parent() );
    dlg.go();
}


bool uiSeisPartServer::select2DSeis( MultiID& mid, bool with_attr )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    SeisSelSetup setup;
    setup.is2d( true ).selattr( with_attr );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DLineSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiSeisPartServer::get2DLineSetName( const MultiID& mid, 
					 BufferString& setname ) const
{
    mGet2DLineSet()
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getLineNames( linenames );

    uiSelectFromList::Setup setup( "Lines", linenames );
    uiSelectFromList dlg( appserv().parent(), setup );
    dlg.selFld()->setMultiSelect();
    dlg.selFld()->selectAll( true );
    if ( !dlg.go() )
	return false;

    dlg.selFld()->getSelectedItems( res );
    return res.size();
}


bool uiSeisPartServer::get2DLineGeometry( const MultiID& mid,
					  const char* linenm,
					  PosInfo::Line2DData& geom ) const
{
    mGet2DLineSet(false)
    int lineidx = lineset.indexOf( linenm );
    if ( lineidx < 0 )
    {
	BufferStringSet attribs;
	get2DStoredAttribs( mid, linenm, attribs );
	if ( attribs.isEmpty() ) return false;
	lineidx = lineset.indexOf( LineKey(linenm,attribs.get(0)) );
	if ( lineidx < 0 ) return false;
    }

    return lineset.getGeometry( lineidx, geom );
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs ) const
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)

    int lidx = lineset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    lineset.getCubeSampling( cs, lidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lidx, buf );
    uiExecutor dlg( appserv().parent(), *exec );
    return dlg.go();
}


BufferStringSet uiSeisPartServer::getStoredGathersList() const
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    BufferStringSet ioobjnms;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( strcmp(ioobj.group(),sKey::PSSeis) ) continue;
	ioobjnms.add( (const char*)ioobj.name() );
	if ( ioobjnms.size() > 1 )
	{
	    for ( int icmp=ioobjnms.size()-2; icmp>=0; icmp-- )
	    {
		if ( ioobjnms.get(icmp) > ioobjnms.get(icmp+1) )
		    ioobjnms.swap(icmp,icmp+1);
	    }
	}
    }
    
    return ioobjnms;
}


MenuItem* uiSeisPartServer::storedGathersSubMenu( bool createnew )
{
    if ( createnew )
    {
	storedgathermenuitem.removeItems();
	storedgathermenuitem.createItems( getStoredGathersList() );
    }

    return &storedgathermenuitem;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiSeisPartServer::handleGatherSubMenu( int mnuid, const BinID& bid )
{
    const int mnuindex = storedgathermenuitem.itemIndex(mnuid);
    if ( mnuindex==-1 ) return true;

    PtrMan<IOObj> ioobj =
	IOM().getLocal( storedgathermenuitem.getItem(mnuindex)->text );

    if ( !ioobj )
	mErrRet( "No valid gather selected" )

    SeisPSReader* rdr = SPSIOPF().getReader( *ioobj, bid.inl );
    if ( !rdr )
	mErrRet( "This Pre-Stack data store cannot be handeled" )

    SeisTrcBuf tbuf;
    if ( !rdr->getGather(bid,tbuf) )
	mErrRet( rdr->errMsg() )

    if ( tbuf.size() == 0 )
	mErrRet( "Gather is empty" )

    const int nrsamples = tbuf.get(0)->size();
    PtrMan< Array2DImpl<float> > a2d =
		new Array2DImpl<float>( tbuf.size(), nrsamples );
    for ( int trcidx=0; trcidx<tbuf.size(); trcidx++ )
    {
	const SeisTrc& trc = *tbuf.get( trcidx );
	for ( int sidx=0; sidx<nrsamples; sidx++ )
	    a2d->set( trcidx, sidx, trc.get(sidx,0) );
    }

    FlatDisp::Context fdctxt;
    fdctxt.annot_.x1name_ = "";
    fdctxt.annot_.x2name_ = "";
    fdctxt.ddpars_.dispvd_ = false;
    fdctxt.ddpars_.dispwva_ = true;
    fdctxt.ddpars_.wva_.overlap_ = 1;
    fdctxt.ddpars_.wva_.clipperc_ = 1;
    const SeisTrc& firsttrc = *tbuf.get(0);
    const SeisTrc& lasttrc = *tbuf.get(tbuf.size()-1);
    fdctxt.posdata_.x1rg_.start = firsttrc.info().nr;
    fdctxt.posdata_.x1rg_.stop = lasttrc.info().nr;
    fdctxt.posdata_.x1rg_.step = 1;
    fdctxt.posdata_.x2rg_.start = firsttrc.info().sampling.start;
    fdctxt.posdata_.x2rg_.stop = firsttrc.info().sampling.start +
				firsttrc.size()*firsttrc.info().sampling.step;
    fdctxt.posdata_.x2rg_.step = firsttrc.info().sampling.step;

    BufferString title( "Gather from [" ); title += ioobj->name();
    title += "] at "; title += bid.inl; title += "/"; title += bid.crl;
    uiDialog psdlg( appserv().parent(),
	    	    uiDialog::Setup("PS Display",title,"") );
    psdlg.setCtrlStyle( uiDialog::LeaveOnly );
    uiFlatDisp::VertViewer* vwr = new uiFlatDisp::VertViewer( &psdlg );
    vwr->setContext( fdctxt );
    vwr->setData( true, a2d, "" );
    psdlg.go();

    return true;
}
