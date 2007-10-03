/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiseispartserv.cc,v 1.68 2007-10-03 12:53:47 cvskris Exp $
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
#include "seistrcsel.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "segposinfo.h"
#include "survinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"

#include "uiexecutor.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uisegysip.h"
#include "uiseiscbvsimp.h"
#include "uiseisiosimple.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseissegyimpexp.h"
#include "uiseissel.h"
#include "uiseiswvltimp.h"
#include "uiseiswvltman.h"
#include "uiselsimple.h"
#include "uisurvey.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    	: uiApplPartServer(a)
    	, storedgathermenuitem("Display Gather")
	, viewwin_(0)
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
    else if ( opt < 3 )
    {
	Seis::GeomType gt = opt == 0 ? Seis::Vol
	    		 : (opt == 1 ? Seis::Line
				     : Seis::VolPS);
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisSegYImpExp( appserv().parent(), forread, segyid, gt );
    }
    else
    {
	Seis::GeomType gt = opt == 4 ? Seis::Vol
	    		 : (opt == 5 ? Seis::Line
				     : Seis::VolPS);
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisIOSimple( appserv().parent(), gt, forread );
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


void uiSeisPartServer::get2DLineInfo( BufferStringSet& linesets,
				      TypeSet<MultiID>& setids,
				      TypeSet<BufferStringSet>& linenames )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    ObjectSet<IOObj> ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( strcmp(ioobj->translator(),"2D") ) continue;

	uiSeisIOObjInfo oinf(*ioobj,false);
	BufferStringSet lnms;
	oinf.getLineNames( lnms );
	linesets.add( ioobj->name() );
	setids += ioobj->key();
	linenames += lnms;
    }
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
    PtrMan<SeisPSReader> rdr = SPSIOPF().getReader( *ioobj, bid.inl );
    if ( !rdr )
	mErrRet( "This Pre-Stack data store cannot be handled" )
    SeisTrcBuf* tbuf = new SeisTrcBuf;
    if ( !rdr->getGather(bid,*tbuf) )
	mErrRet( rdr->errMsg() )
    const int tbufsz = tbuf->size();
    if ( tbufsz == 0 )
	mErrRet( "Gather is empty" )

    SeisTrcBuf* tbuffreq = new SeisTrcBuf;
    for ( int idx=0; idx<tbufsz; idx++ )
    {
	const SeisTrc& inptrc = *tbuf->get(idx);
	SeisTrc* trc = new SeisTrc( inptrc );
	SeisTrcPropCalc pc( inptrc, 0 );
	for ( int isamp=0; isamp<trc->size(); isamp++ )
	    trc->set( isamp, pc.getFreq(isamp), 0 );
	tbuffreq->add( trc );
    }

    BufferString title( "Gather from [" ); title += ioobj->name();
    title += "] at "; title += bid.inl; title += "/"; title += bid.crl;
    bool isnew = !viewwin_;
    if ( !isnew )
	viewwin_->setWinTitle( title );
    else
    {
	viewwin_ = new uiFlatViewMainWin( appserv().parent(),
				          uiFlatViewMainWin::Setup(title) );
	viewwin_->setDarkBG( false );
	FlatView::Appearance& app = viewwin_->viewer().appearance();
	app.annot_.setAxesAnnot( true );
	app.setGeoDefaults( true );
	app.ddpars_.show( true, false );
	app.ddpars_.wva_.overlap_ = 1;
    }

    uiFlatViewer& vwr = viewwin_->viewer();
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf,
				 Seis::VolPS, SeisTrcInfo::Offset,
				 "Pre-Stack Gather" );
    dp->setName( "Seismics" );
    DPM( DataPackMgr::FlatID ).add( dp );
    vwr.setPack( true, dp );
    dp = new SeisTrcBufDataPack( tbuffreq, Seis::VolPS, SeisTrcInfo::Offset,
				 "Pre-Stack Gather.Freq" );
    dp->setName( "Local frequency" );
    DPM( DataPackMgr::FlatID ).add( dp );
    vwr.setPack( false, dp );

    if ( !isnew )
	vwr.handleChange( FlatView::Viewer::All );
    else
    {
	vwr.appearance().ddpars_.show( true, false );
	int pw = 200 + 10 * tbufsz;
	if ( pw < 400 ) pw = 400; if ( pw > 800 ) pw = 800;
	vwr.setInitialSize( uiSize(pw,500) );
	viewwin_->addControl( new uiFlatViewStdControl( vwr,
			      uiFlatViewStdControl::Setup()
			      .withstates(false) ) );
    }

    viewwin_->start();
    return true;
}
