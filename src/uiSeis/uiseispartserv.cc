/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisioobjinfo.h"

#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uiseiscbvsimp.h"
#include "uiseisiosimple.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseisrandto2dline.h"
#include "uiseiscbvsimpfromothersurv.h"
#include "uisegyread.h"
#include "uisegyexp.h"
#include "uisegysip.h"
#include "uisurvinfoed.h"
#include "uiseissel.h"
#include "uiseiswvltimpexp.h"
#include "uiseiswvltman.h"
#include "uiseispreloadmgr.h"
#include "uisegyresortdlg.h"
#include "uiselsimple.h"
#include "uisurvey.h"
#include "uitaskrunner.h"
#include "uibatchtime2depthsetup.h"
#include "uivelocityvolumeconversion.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider();
    uiSurveyInfoEditor::addInfoProvider( sip );
    SeisIOObjInfo::initDefault( sKey::Steering() );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    PtrMan<uiDialog> dlg = 0;
    if ( opt == 0 )
	dlg = new uiSeisImpCBVS( appserv().parent() );
    else if ( opt == 9 )
	dlg = new uiSeisImpCBVSFromOtherSurveyDlg( appserv().parent() ); 
    else if ( opt < 5 )
    {
	if ( !forread )
	    dlg = new uiSEGYExp( appserv().parent(),
		    		 Seis::geomTypeOf( !(opt%2), opt > 2 ) );
	else
	{
	    const bool isdirect = !(opt % 2);
	    uiSEGYRead::Setup su( isdirect ? uiSEGYRead::DirectDef
		    			   : uiSEGYRead::Import );
	    if ( isdirect )
		su.geoms_ -= Seis::Line;
	    new uiSEGYRead( appserv().parent(), su );
	}
    }
    else
    {
	const Seis::GeomType gt( Seis::geomTypeOf( !(opt%2), opt > 6 ) );
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisIOSimple( appserv().parent(), gt, forread );
    }


    return dlg ? dlg->go() : true;
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


void uiSeisPartServer::manageSeismics( bool is2d )
{
    uiSeisFileMan dlg( appserv().parent(), is2d );
    dlg.go();
}


void uiSeisPartServer::managePreLoad()
{
    uiSeisPreLoadMgr dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    uiSeisWvltImp dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::exportWavelets()
{
    uiSeisWvltExp dlg( appserv().parent() );
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
    uiSeisSel::Setup setup(Seis::Line); setup.selattr( with_attr );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    PtrMan<IOObj> lsobj = IOM().get( mid );
    if ( !lsobj || !S2DPOS().hasLineSet(lsobj->name()) )
    {
	uiMSG().error( "Lineset has no or corrupted geometry file" );
	return false;
    }

    return true;
}


#define mGet2DLineSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiSeisPartServer::get2DLineSetName( const MultiID& mid, 
					 BufferString& setname )
{
    mGet2DLineSet(;)
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( mid );
    objinfo.ioObjInfo().getLineNames( linenames );

    uiSelectFromList::Setup setup( "Select 2D Lines", linenames );
    uiSelectFromList dlg( appserv().parent(), setup );
    dlg.setHelpID("50.0.17");
    if ( dlg.selFld() )
    {
	dlg.selFld()->setMultiSelect();
	dlg.selFld()->selectAll( true );
    }
    if ( !dlg.go() )
	return false;

    if ( dlg.selFld() )
	dlg.selFld()->getSelectedItems( res );
    return res.size();
}


void uiSeisPartServer::get2DLineInfo( BufferStringSet& linesets,
				      TypeSet<MultiID>& setids,
				      TypeSet<BufferStringSet>& linenames )
{
    SeisIOObjInfo::get2DLineInfo( linesets, &setids, &linenames );
}


bool uiSeisPartServer::get2DLineGeometry( const MultiID& mid,
					  const char* linenm,
					  PosInfo::Line2DData& geom )
{
    mGet2DLineSet(false);

    bool setzrange = false;
    StepInterval<float> maxzrg;

    int lineidx = lineset.indexOf( linenm );
    if ( lineidx < 0 )
    {
	BufferStringSet attribs;
	get2DStoredAttribs( mid, linenm, attribs );
	if ( attribs.isEmpty() ) return false;

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	int maxnrtrcs = 0;
	bool first = true;
	for ( int idx=0; idx<attribs.size(); idx++ )
	{
	    int indx = lineset.indexOf( LineKey(linenm,attribs.get(idx)) );
	    if ( indx<0 || !lineset.getRanges(indx,trcrg,zrg) || !trcrg.step )
		continue;

	    const int nrtrcs = trcrg.nrSteps() + 1;
	    if ( nrtrcs > maxnrtrcs )
	    {
		maxnrtrcs = nrtrcs;
		lineidx = indx;
	    }

	    if ( first )
	    {
		first = false;
		maxzrg = zrg;
	    }
	    else
	    {
		maxzrg.start = mMIN(maxzrg.start, zrg.start );
		maxzrg.stop = mMAX(maxzrg.stop, zrg.stop );
		maxzrg.step = mMIN(maxzrg.step, zrg.step );
	    }
	}

	if ( lineidx < 0 ) return false;
	setzrange = true;
    }

    S2DPOS().setCurLineSet( lineset.name() );
    geom.setLineName( BufferString(linenm) );
    if ( !S2DPOS().getGeometry( geom ) )
	return false;

    if ( setzrange )
	geom.setZRange( maxzrg );

    return true;
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid,
					   const char* linenm,
					   BufferStringSet& attribs,
					   int steerpol )
{
    uiSeisIOObjInfo objinfo( mid );
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = steerpol;
    objinfo.ioObjInfo().getAttribNamesForLine( linenm, attribs, o2d );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)

    const int lidx = lineset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    lineset.getCubeSampling( cs, lidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lidx, buf );
    uiTaskRunner dlg( appserv().parent() );
    return dlg.execute( *exec );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj)
	  && SeisTrcTranslator::is2D(ioobj) != for3d )
	    nms.add( (const char*)ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( appserv().parent(), &rln );
    dlg.go();
}


void uiSeisPartServer::resortSEGY() const
{
    uiResortSEGYDlg dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::processTime2Depth() const
{
    uiBatchTime2DepthSetup dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::processVelConv() const
{
    Vel::uiBatchVolumeConversion dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::get2DZdomainAttribs( const MultiID& mid,
	const char* linenm, const char* zdomainstr, BufferStringSet& attribs )
{
    mGet2DLineSet(;)
    lineset.getZDomainAttrib( attribs, linenm, zdomainstr );
}
