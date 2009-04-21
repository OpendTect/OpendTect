/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisrandto2dline.cc,v 1.11 2009-04-21 06:16:35 cvsnanne Exp $";

#include "uiseisrandto2dline.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "draw.h"
#include "linear.h"
#include "linekey.h"
#include "randomlinegeom.h"
#include "seisrandlineto2d.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "uibutton.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"


uiSeisRandTo2DBase::uiSeisRandTo2DBase( uiParent* p,
				        const Geometry::RandomLine& rln )
    : uiGroup(p,"Base group")
    , inctio_(*mMkCtxtIOObj(SeisTrc))
    , outctio_(*mMkCtxtIOObj(SeisTrc))
    , randln_(rln)
    , inpfld_(0),outpfld_(0)
{
    inctio_.ctxt.forread = true;
    inpfld_ = new uiSeisSel( this, inctio_, uiSeisSel::Setup(Seis::Vol) );

    outctio_.ctxt.forread = false;
    outpfld_ = new uiSeisSel( this, outctio_, uiSeisSel::Setup(Seis::Line) );
    outpfld_->setConfirmOverwrite( false );
    outpfld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( outpfld_ );
}


uiSeisRandTo2DBase::~uiSeisRandTo2DBase()
{ delete &inctio_; delete &outctio_; }


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeisRandTo2DBase::checkInputs()
{
    if ( !inpfld_->commitInput() )
	mErrRet("Missing Input\nPlease select the input seismics")
    if ( !outpfld_->commitInput() )
	mErrRet("Missing Output\nPlease select a lineset for output")

    BufferString attrnm = outpfld_->attrNm();
    if ( attrnm.isEmpty() )
	mErrRet("Missing Attribute name")

    return true;
}


uiSeisRandTo2DLineDlg::uiSeisRandTo2DLineDlg( uiParent* p,
					      const Geometry::RandomLine& rln )
    : uiDialog(p,uiDialog::Setup("Save as 2D line","",""))
{
    basegrp_ = new uiSeisRandTo2DBase( this, rln );

    linenmfld_ = new uiGenInput( this, "Line Name", StringInpSpec(rln.name()) );
    linenmfld_->attach( alignedBelow, basegrp_ );

    trcnrfld_ = new uiGenInput( this, "First Trace Nr", IntInpSpec(1) );
    trcnrfld_->attach( alignedBelow, linenmfld_ );
}


bool uiSeisRandTo2DLineDlg::acceptOK( CallBacker* )
{
    if ( !basegrp_->checkInputs() )
	return false;

    BufferString attrnm = basegrp_->outpfld_->attrNm();
    BufferString linenm = linenmfld_->text();
    if ( linenm.isEmpty() )
	mErrRet("Missing Line Name\nPlease enter a Line Name")

    const int trcnrstart = trcnrfld_->getIntValue();
    if ( mIsUdf(trcnrstart) || trcnrstart <= 0 )
	mErrRet("Please specify a valid starting trace number")

    LineKey lk( linenm, attrnm );
    SeisRandLineTo2D exec( basegrp_->inctio_.ioobj, basegrp_->outctio_.ioobj,
	    		   lk, trcnrstart, basegrp_->randln_ );
    uiTaskRunner dlg( this );
    if ( !dlg.execute(exec) )
	return false;
    
    if ( !SI().has2D() )
	uiMSG().warning( "You need to change survey type to 'Both 2D and 3D'"
	       		 " to display the 2D line" );

    return true;
}


static const int cMargin = 40;
static const int cCanvssz = 300;
uiSeisRandTo2DGridDlg::uiSeisRandTo2DGridDlg( uiParent* p,
					      const Geometry::RandomLine& rln )
    : uiFullBatchDialog(p,uiFullBatchDialog::Setup("Create 2D Grid")
	    					  .procprognm("odseis2dgrid"))
    , preview_(0)
    , parallelset_(0),perpset_(0)
{
    setTitleText("Specify parameters");
    inpgrp_ = new uiGroup( this, "Input group" );
    basegrp_ = new uiSeisRandTo2DBase( inpgrp_, rln );

    parlineprefixfld_ = new uiGenInput( inpgrp_, "Prefix for parallel lines",
	    				StringInpSpec("Dip") );
    parlineprefixfld_->attach( alignedBelow, basegrp_ );

    perplineprefixfld_ = new uiGenInput( inpgrp_, "Prefix for perp lines",
	    				 StringInpSpec("Strike") );
    perplineprefixfld_->attach( alignedBelow, parlineprefixfld_ );

    BufferString lbltxt( "Grid spacing " );
    lbltxt += SI().getXYUnitString();
    distfld_ = new uiGenInput( inpgrp_, lbltxt.buf(), IntInpSpec() );
    distfld_->valuechanged.notify( mCB(this,uiSeisRandTo2DGridDlg,distChgCB) );
    distfld_->attach( alignedBelow, perplineprefixfld_ );

    nrparlinesfld_ = new uiLabel( inpgrp_, "0" );
    nrparlinesfld_->setPrefWidthInChar( 3 );
    nrparlinesfld_->attach( alignedBelow, distfld_ );
    uiLabel* lbl = new uiLabel( inpgrp_, "Nr of Parallel lines:" );
    lbl->attach( leftOf, nrparlinesfld_ );

    nrperplinesfld_ = new uiLabel( inpgrp_, "0" );
    nrperplinesfld_->setPrefWidthInChar( 3 );
    nrperplinesfld_->attach( alignedBelow, nrparlinesfld_ );
    lbl = new uiLabel( inpgrp_, "Nr of Perpendicular lines:" );
    lbl->attach( leftOf, nrperplinesfld_ );

    createPreview();
    const Coord maxcoord = SI().maxCoord(false);
    const Coord mincoord = SI().minCoord(false);
    const float maxdim = mMAX( maxcoord.x-mincoord.x, maxcoord.y-mincoord.y );
    AxisLayout axl( Interval<float>(0,maxdim) );
    distfld_->setValue( mNINT(axl.sd.step/2) );
    finaliseDone.notify( mCB(this,uiSeisRandTo2DGridDlg,distChgCB) );
}


void uiSeisRandTo2DGridDlg::createPreview()
{
    preview_ = new uiGraphicsView( this, "Preview" );
    preview_->attach( rightOf, inpgrp_ );
    preview_->setStretch( 0, 0 );
    preview_->setPrefWidth( 300 );
    preview_->setPrefHeight( 300 );
}

void uiSeisRandTo2DGridDlg::updatePreview()
{
    uiGraphicsScene& scene = preview_->scene();
    scene.removeAllItems();

    const CubeSampling& cs = SI().sampling( false );
    Coord mapcnr[4];
    mapcnr[0] = SI().transform( cs.hrg.start );
    mapcnr[1] = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    mapcnr[2] = SI().transform( cs.hrg.stop );
    mapcnr[3] = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );

    Coord mincoord = SI().minCoord(false);
    Coord maxcoord = SI().maxCoord(false);

    const double xwidth = maxcoord.x - mincoord.x;
    const double ywidth = maxcoord.y - mincoord.y;
    const double ratio = xwidth / ywidth;
    int width = cCanvssz - 2*cMargin;
    int height = cCanvssz - 2*cMargin;
    if ( ratio > 1 )
	height = mNINT( height / ratio );
    else
	width = mNINT( width * ratio );

    uiWorldRect wr( mincoord.x, maxcoord.y, maxcoord.x, mincoord.y );
    uiRect rect( (cCanvssz-width)/2, (cCanvssz-height)/2,
	    	 (cCanvssz+width)/2, (cCanvssz+height)/2 );
    uiWorld2Ui w2ui;
    w2ui.set( rect, wr );

    Color red( 255, 0, 0, 0);
    LineStyle ls( LineStyle::Solid, 3, red );
    for ( int idx=0; idx<4; idx++ )
    {
	const int idx2 = idx < 3 ? idx + 1 : 0;
	uiPoint pt1 = w2ui.transform( uiWorldPoint(mapcnr[idx].x,
		    				   mapcnr[idx].y) );
	uiPoint pt2 = w2ui.transform( uiWorldPoint(mapcnr[idx2].x,
		    				   mapcnr[idx2].y) );
	uiLineItem* lineitm = scene.addItem( new uiLineItem(pt1,pt2,true) );
	lineitm->setPenStyle( ls );
    }

    for ( int idx=0; idx<4; idx++ )
    {
	uiPoint pt = w2ui.transform( uiWorldPoint(mapcnr[idx].x,
		  				  mapcnr[idx].y) );
	bool bot = pt.y > cCanvssz/2;
        BinID bid = SI().transform( mapcnr[idx] );
        int spacing =  bot ? 10 : -10;
	BufferString annot;
        annot += bid.inl; annot += "/"; annot += bid.crl;
	uiPoint txtpos( pt.x, pt.y+spacing );
        uiTextItem* textitm1 = scene.addItem(
	    new uiTextItem(txtpos,annot.buf(),mAlignment(HCenter,VCenter)) );
	textitm1->setPenColor( Color::Black() );
	textitm1->setFont(
		FontList().get(FontData::key(FontData::GraphicsSmall)) );
	textitm1->setZValue( 1 );
    }

    if ( parallelset_ )
	drawLines( w2ui, true );
    if ( perpset_ )
	drawLines( w2ui, false );
    
    Color brown( 150, 10, 10, 0);
    LineStyle lsbase( LineStyle::Solid, 2, brown );
    const Coord c1 = SI().transform( basegrp_->randln_.nodePosition(0) );
    const Coord c2 = SI().transform( basegrp_->randln_.nodePosition(1) );
    const uiPoint start = w2ui.transform( uiWorldPoint(c1.x,c1.y) );
    const uiPoint stop = w2ui.transform( uiWorldPoint(c2.x,c2.y) );
    uiLineItem* lineitm = scene.addItem( new uiLineItem(start,stop,true) );
    lineitm->setPenStyle( lsbase );
    lineitm->setZValue( 22 );
}


void uiSeisRandTo2DGridDlg::drawLines( const uiWorld2Ui& w2ui, bool parll )
{
    const ObjectSet<Geometry::RandomLine>& rlset = parll ? parallelset_->lines()
							 : perpset_->lines();
    const int nrlines = rlset.size();
    if ( nrlines <= 0 )
	return;

    uiGraphicsScene& scene = preview_->scene();
    Color green( 0, 255, 0, 0);
    LineStyle ls( LineStyle::Solid, 1, green );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	const Coord c1 = SI().transform( rlset[idx]->nodePosition(0) );
	const Coord c2 = SI().transform( rlset[idx]->nodePosition(1) );
	const uiPoint start = w2ui.transform( uiWorldPoint(c1.x,c1.y) );
	const uiPoint stop = w2ui.transform( uiWorldPoint(c2.x,c2.y) );
	uiLineItem* lineitm = scene.addItem( new uiLineItem(start,stop,true) );
	lineitm->setPenStyle( ls );
    }
}


void uiSeisRandTo2DGridDlg::distChgCB( CallBacker* )
{
    const int dist = distfld_->getIntValue();
    if ( !mIsUdf(dist) )
    {
	createLines();
	nrparlinesfld_->setText( Conv::to<const char*>(parallelset_->size()) );
	nrperplinesfld_->setText( Conv::to<const char*>(perpset_->size()) );
    }

    updatePreview();
}


bool uiSeisRandTo2DGridDlg::checkInputs()
{
    if ( !basegrp_->checkInputs() )
	return false;

    BufferString parprefix = parlineprefixfld_->text();
    if ( parprefix.isEmpty() )
	mErrRet("Please enter a valid prefix for parallel lines");

    BufferString perpprefix = perplineprefixfld_->text();
    if ( perpprefix.isEmpty() )
	mErrRet("Please enter a valid prefix for perpendicular lines");

    return true;
}


bool uiSeisRandTo2DGridDlg::fillPar( IOPar& par )
{
    if ( !checkInputs() || !createLines() )
	return false;

    par.set( SeisRandLineTo2DGrid::sKeyInputID(),
	     basegrp_->inctio_.ioobj->key() );
    par.set( SeisRandLineTo2DGrid::sKeyOutputID(),
	     basegrp_->outctio_.ioobj->key() );
    par.set( SeisRandLineTo2DGrid::sKeyOutpAttrib(),
	     basegrp_->outpfld_->attrNm() );
    par.set( SeisRandLineTo2DGrid::sKeyGridSpacing(),
	     distfld_->getIntValue() );
    par.set( SeisRandLineTo2DGrid::sKeyParPrefix(),
	     parlineprefixfld_->text() );
    par.set( SeisRandLineTo2DGrid::sKeyPerpPrefix(),
	     perplineprefixfld_->text() );

    IOPar rlnpar;
    rlnpar.set( SeisRandLineTo2DGrid::sKeyStartBinID(),
	        basegrp_->randln_.nodePosition(0) );
    rlnpar.set( SeisRandLineTo2DGrid::sKeyStopBinID(),
	        basegrp_->randln_.nodePosition(1) );
    par.mergeComp( rlnpar, SeisRandLineTo2DGrid::sKeyRandomLine() );
    return true;
}


bool uiSeisRandTo2DGridDlg::createLines()
{
    const Coord maxcoord = SI().maxCoord( false );
    const Coord mincoord = SI().minCoord( false );
    const double maxdist = (maxcoord.x - mincoord.x) / 2;
    const double mindist = (maxcoord.x - mincoord.x) / 200;
    const double dist = (double) distfld_->getIntValue();
    if ( dist < mindist || dist > maxdist )
	mErrRet("Grid spacing is either too small or too large");

    delete parallelset_;
    delete perpset_;
    parallelset_ = new Geometry::RandomLineSet( basegrp_->randln_, dist, true );
    perpset_ = new Geometry::RandomLineSet( basegrp_->randln_, dist, false );

    return parallelset_->size() && perpset_->size();
}

#undef mErrRet

