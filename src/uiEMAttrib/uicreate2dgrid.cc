/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2009
________________________________________________________________________

-*/

#include "uicreate2dgrid.h"

#include "uibasemap.h"
#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiseis2dfrom3d.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiselsurvranges.h"
#include "uiseparator.h"
#include "uisurvmap.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"
#include "ui2dgeomman.h"

#include "axislayout.h"
#include "trckeyzsampling.h"
#include "emsurfacetr.h"
#include "emhorizon2d.h"
#include "gridcreator.h"
#include "linekey.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "seiscbvs.h"
#include "separstr.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"
#include "od_helpids.h"



class uiGrid2DMapObject : public uiBaseMapObject
{
public:
uiGrid2DMapObject()
    : uiBaseMapObject(0)
    , grid_(0),baseline_(0)
{}

const char* getType()
{ return "Grid2D"; }

void setGrid( const Grid2D* grid )
{
    grid_ = grid;
}

void setBaseLine( const Grid2D::Line* baseline )
{
    baseline_ = baseline;
}

#define mDrawLine(line) { \
    const Coord start = SI().transform( line->start_ ); \
    const Coord stop = SI().transform( line->stop_ ); \
    TypeSet<uiWorldPoint> pts; pts.add( start ); pts.add( stop ); \
    auto* item = new uiPolyLineItem( pts ); \
    item->setPenStyle( ls ); \
    item->setZValue( graphicszval ); \
    graphitem_.addChild( item ); \
}

void update()
{
    graphitem_.removeAll( true );
    lines_.erase();

    if ( !grid_ ) return;

    OD::LineStyle ls;
    ls.color_ = OD::Color( 40, 140, 180 );
    ls.width_ = 1;
    int graphicszval = 2;
    for ( int idx=0; idx<grid_->size(true); idx++ )
    {
	const Grid2D::Line* line2d = grid_->getLine( idx, true );
	if ( line2d )
	    mDrawLine( line2d );
    }

    for ( int idx=0; idx<grid_->size(false); idx++ )
    {
	const Grid2D::Line* line2d = grid_->getLine( idx, false );
	if ( line2d )
	    mDrawLine( line2d );
    }

    ls.width_ += 2;
    ls.color_ = OD::Color::Green();
    graphicszval = 4;
    if ( baseline_ )
	mDrawLine( baseline_ );
}

protected:

    ObjectSet<uiLineItem>	lines_;
    const Grid2D::Line*		baseline_;
    const Grid2D*		grid_;
};



ui2DGridLines::ui2DGridLines( uiParent* p, const TrcKeySampling& hs )
    : uiGroup(p,"2D Grid Parameters")
    , hs_(hs), grid_(new Grid2D)
    , gridChanged(this)
{
    inlprefixfld_ = new uiGenInput( this, tr("Prefix for parallel lines") );
    crlprefixfld_ = new uiGenInput( this, tr("Prefix for perpendicular lines"));
    crlprefixfld_->attach( alignedBelow, inlprefixfld_ );
}


ui2DGridLines::~ui2DGridLines()
{
    delete grid_;
}


void ui2DGridLines::updateRange()
{
    computeGrid();
    gridChanged.trigger();
}


bool ui2DGridLines::fillPar( IOPar& par ) const
{
    par.set( Seis2DGridCreator::sKeyInlPrefix(), inlprefixfld_->text() );
    par.set( Seis2DGridCreator::sKeyCrlPrefix(), crlprefixfld_->text() );
    return true;
}


ui2DGridLinesFromInlCrl::ui2DGridLinesFromInlCrl( uiParent* p,
						  const TrcKeySampling& hs )
    : ui2DGridLines(p,hs)
{
    CallBack modecb = mCB(this,ui2DGridLinesFromInlCrl,modeChg);
    CallBack parscb = mCB(this,ui2DGridLinesFromInlCrl,paramsChgCB);
    inlmodefld_ = new uiGenInput( this, uiStrings::sInline(),
				  BoolInpSpec(true,uiStrings::sRange(),
				  tr("Individual line(s)")) );
    inlmodefld_->valuechanged.notify( modecb );

    inlrgfld_ = new uiSelNrRange( this, uiSelNrRange::Inl, true );
    inlrgfld_->rangeChanged.notify( parscb );
    inlrgfld_->attach( alignedBelow, inlmodefld_ );

    inlsfld_ = new uiGenInput( this, tr("In-lines (comma separated)") );
    inlsfld_->valuechanged.notify( parscb );
    inlsfld_->attach( alignedBelow, inlmodefld_ );

    crlmodefld_ = new uiGenInput( this, uiStrings::sCrossline(),
				  BoolInpSpec(true,uiStrings::sRange(),
				  tr("Individual line(s)")) );
    crlmodefld_->valuechanged.notify( modecb );
    crlmodefld_->attach( alignedBelow, inlrgfld_ );

    crlrgfld_ = new uiSelNrRange( this, uiSelNrRange::Crl, true );
    crlrgfld_->rangeChanged.notify( parscb );
    crlrgfld_->attach( alignedBelow, crlmodefld_ );

    crlsfld_ = new uiGenInput( this, tr("Cross-lines (comma separated)") );
    crlsfld_->valuechanged.notify( parscb );
    crlsfld_->attach( alignedBelow, crlmodefld_ );

    inlprefixfld_->attach( alignedBelow, crlsfld_ );
    inlprefixfld_->setText( "INL" );
    crlprefixfld_->setText( "XL" );

    setHAlignObj( inlmodefld_ );
    modeChg( 0 );
}


void ui2DGridLinesFromInlCrl::modeChg( CallBacker* cb )
{
    if ( !cb || cb == inlmodefld_ )
    {
	inlrgfld_->display( inlmodefld_->getBoolValue() );
	inlsfld_->display( !inlmodefld_->getBoolValue() );
    }
    if ( !cb || cb == crlmodefld_ )
    {
	crlrgfld_->display( crlmodefld_->getBoolValue() );
	crlsfld_->display( !crlmodefld_->getBoolValue() );
    }

    if ( cb )
	paramsChgCB( 0 );
}


void ui2DGridLinesFromInlCrl::updateRange()
{
    inlrgfld_->rangeChanged.disable();
    crlrgfld_->rangeChanged.disable();
    StepInterval<int> inlrg = hs_.inlRange();
    StepInterval<int> crlrg = hs_.crlRange();
    inlrgfld_->setLimitRange( inlrg );
    crlrgfld_->setLimitRange( crlrg );

    inlrg.step = 100; crlrg.step = 100;
    inlrgfld_->setRange( inlrg );
    crlrgfld_->setRange( crlrg );

    inlrgfld_->rangeChanged.enable();
    crlrgfld_->rangeChanged.enable();
    ui2DGridLines::updateRange();
}


void ui2DGridLinesFromInlCrl::paramsChgCB( CallBacker* )
{
    ui2DGridLines::updateRange();
}


void ui2DGridLinesFromInlCrl::getNrLinesLabelTexts( BufferString& inltxt,
						    BufferString& crltxt ) const
{
    inltxt = "Nr of In-lines in grid: ";
    inltxt += grid_->size( true );
    crltxt = "Nr of Cross-lines in grid: ";
    crltxt += grid_->size( false );
}


bool ui2DGridLinesFromInlCrl::computeGrid()
{
    TypeSet<int> inlines, crlines;
    if ( inlmodefld_->getBoolValue() )
    {
	StepInterval<int> range = inlrgfld_->getRange();
	for ( int idx=0; idx<=range.nrSteps(); idx++ )
	    inlines += range.atIndex( idx );
    }
    else
    {
	SeparString str( inlsfld_->text() );
	for ( int idx=0; idx<str.size(); idx++ )
	    inlines += str.getIValue(idx);
    }

    if ( crlmodefld_->getBoolValue() )
    {
	StepInterval<int> range = crlrgfld_->getRange();
	for ( int idx=0; idx<=range.nrSteps(); idx++ )
	    crlines += range.atIndex( idx );
    }
    else
    {
	SeparString str( crlsfld_->text() );
	for ( int idx=0; idx<str.size(); idx++ )
	    crlines += str.getIValue(idx);
    }

    grid_->set( inlines, crlines, hs_ );
    return grid_->totalSize();
}


void ui2DGridLinesFromInlCrl::getLineNames( BufferStringSet& linenames ) const
{
    for ( int inlidx=0; inlidx < grid_->size(true); inlidx++ )
    {
	BufferString linename( inlprefixfld_->text() );
	linename += grid_->getLine( inlidx, true )->start_.inl();
	linenames.add( linename );
    }

    for ( int crlidx=0; crlidx < grid_->size(false); crlidx++ )
    {
	BufferString linename( crlprefixfld_->text() );
	linename += grid_->getLine( crlidx, false )->start_.crl();
	linenames.add( linename );
    }
}


bool ui2DGridLinesFromInlCrl::fillPar( IOPar& par ) const
{
    if ( inlmodefld_->getBoolValue() )
    {
	par.set( Seis2DGridCreator::sKeyInlSelType(), sKey::Range() );
	par.set( sKey::InlRange(), inlrgfld_->getRange() );
    }
    else
    {
	par.set( Seis2DGridCreator::sKeyInlSelType(), sKey::Selection() );
	par.set( sKey::InlRange(), inlsfld_->text() );
    }

    if ( crlmodefld_->getBoolValue() )
    {
	par.set( Seis2DGridCreator::sKeyCrlSelType(), sKey::Range() );
	par.set( sKey::CrlRange(), crlrgfld_->getRange() );
    }
    else
    {
	par.set( Seis2DGridCreator::sKeyCrlSelType(), sKey::Selection() );
	par.set( sKey::CrlRange(), crlsfld_->text() );
    }

    return ui2DGridLines::fillPar( par );
}


ui2DGridLinesFromRandLine::ui2DGridLinesFromRandLine( uiParent* p,
			const TrcKeySampling& hs,
			const Geometry::RandomLine* rdl )
    : ui2DGridLines(p,hs)
    , rdlfld_(0)
{
    const BinID startnode = rdl ? rdl->nodePosition(0) : BinID::udf();
    const BinID stopnode = rdl ? rdl->nodePosition( rdl->nrNodes() - 1 )
			       : BinID::udf();
    baseline_ = new Grid2D::Line( startnode, stopnode );
    uiString parlbl(
	    tr( "Parallel line spacing %1" ).arg( SI().getXYUnitString() ) );
    const StepInterval<int> spacinglimits( 500, 1000000, 500 );
    pardistfld_ = new uiGenInput( this, parlbl,
				  IntInpSpec().setLimits(spacinglimits) );
    pardistfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromRandLine,
					  paramsChgCB) );
    if ( !rdl || rdl->nrNodes() != 2 )
    {
	rdlfld_ = new uiIOObjSel( this, mIOObjContext(RandomLineSet),
				uiStrings::phrInput(uiStrings::sRandomLine()) );
	rdlfld_->selectionDone.notify(
		mCB(this,ui2DGridLinesFromRandLine,paramsChgCB) );
	pardistfld_->attach( alignedBelow, rdlfld_ );
    }

    uiString perlbl(
	    tr("Perpendicular line spacing %1").arg(SI().getUiXYUnitString()) );
    perdistfld_ = new uiGenInput( this, perlbl,
				  IntInpSpec().setLimits(spacinglimits) );
    perdistfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromRandLine,
					  paramsChgCB) );
    perdistfld_->attach( alignedBelow, pardistfld_ );

    const Coord maxcoord = SI().maxCoord(false);
    const Coord mincoord = SI().minCoord(false);
    const float maxdim = (float) mMAX( maxcoord.x-mincoord.x,
				       maxcoord.y-mincoord.y );
    AxisLayout<float> axl( Interval<float>(0,maxdim) );
    pardistfld_->setValue( mNINT32(axl.sd_.step/2) );
    perdistfld_->setValue( mNINT32(axl.sd_.step/2) );

    inlprefixfld_->attach( alignedBelow, perdistfld_ );
    inlprefixfld_->setText( "PAR" );
    crlprefixfld_->setText( "PERP" );
    setHAlignObj( pardistfld_ );
}


ui2DGridLinesFromRandLine::~ui2DGridLinesFromRandLine()
{
    delete baseline_;
}


bool ui2DGridLinesFromRandLine::computeGrid()
{
    const double pardist = (double) pardistfld_->getIntValue();
    const double perdist = (double) perdistfld_->getIntValue();
    if ( !baseline_ )
	return false;

    grid_->set( *baseline_, pardist, perdist, hs_ );
    if ( grid_->size(true) > 100 || grid_->size(true) > 100 )
    {
	uiString msg = tr( "There are too many lines in the grid" );
	msg.append( "You may want to increase the grid spacing "
		    "to get fewer lines." );
	if ( !uiMSG().askGoOn(msg,tr("Continue anyway"),uiStrings::sCancel()) )
	    return false;
    }

    return grid_->totalSize();
}


void ui2DGridLinesFromRandLine::paramsChgCB( CallBacker* cb )
{
    if ( cb && cb == rdlfld_ )
	processInput();

    ui2DGridLines::updateRange();
}


void ui2DGridLinesFromRandLine::getNrLinesLabelTexts( BufferString& inltxt,
						BufferString& crltxt ) const
{
    inltxt = "Nr of parallel lines in grid: ";
    inltxt += grid_->size( true );
    crltxt = "Nr of perpendicular lines in grid: ";
    crltxt += grid_->size( false );
}


void ui2DGridLinesFromRandLine::getLineNames( BufferStringSet& linenames ) const
{
    for ( int strikelidx=0; strikelidx < grid_->size(true); strikelidx++ )
    {
	BufferString linename( inlprefixfld_->text() );
	linename += strikelidx;
	linenames.add( linename );
    }

    for ( int diplidx=0; diplidx < grid_->size(false); diplidx++ )
    {
	BufferString linename( crlprefixfld_->text() );
	linename += diplidx;
	linenames.add( linename );
    }
}


bool ui2DGridLinesFromRandLine::hasLine() const
{
    return !baseline_->start_.isUdf() && !baseline_->stop_.isUdf();
}


void ui2DGridLinesFromRandLine::processInput()
{
    if ( !rdlfld_->ioobj(true) )
	return;

    Geometry::RandomLineSet geom;
    BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(geom,rdlfld_->ioobj(),msg) )
    {
	uiMSG().error( mToUiStringTodo(msg) );
	return;
    }

    const Geometry::RandomLine* rdl = geom.isEmpty() ? 0 : geom.lines()[0];
    baseline_->start_ = rdl->nodePosition( 0 );
    baseline_->stop_ = rdl->nodePosition( rdl->nrNodes() - 1 );
}


bool ui2DGridLinesFromRandLine::fillPar( IOPar& par ) const
{
    IOPar lnpar;
    lnpar.set( Seis2DGridCreator::sKeyStartBinID(), baseline_->start_ );
    lnpar.set( Seis2DGridCreator::sKeyStopBinID(), baseline_->stop_ );
    par.mergeComp( lnpar, Seis2DGridCreator::sKeyBaseLine() );

    par.set( Seis2DGridCreator::sKeyInlSpacing(), pardistfld_->getIntValue() );
    par.set( Seis2DGridCreator::sKeyCrlSpacing(), perdistfld_->getIntValue() );
    return ui2DGridLines::fillPar( par );
}


uiCreate2DGrid::uiCreate2DGrid( uiParent* p, const Geometry::RandomLine* rdl )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCreate(tr("%2 %3 Grid")
		 .arg(uiStrings::s2D()).arg(uiStrings::sSeismic())),mNoDlgTitle,
		 mODHelpKey(mCreate2DGridHelpID) ) )
    , sourceselfld_(0),inlcrlgridgrp_(0)
    , tkzs_(*new TrcKeyZSampling(true))
{
    setCtrlStyle( RunAndClose );

    uiGroup* seisgrp = createSeisGroup( rdl );

    uiGroup* previewgrp = createPreviewGroup();
    previewgrp->attach( rightTo, seisgrp );

    uiSeparator* sep = new uiSeparator( this, "HSeparator" );
    sep->attach( stretchedBelow, previewgrp );
    sep->attach( ensureBelow, seisgrp );

    uiGroup* horgrp = createHorizonGroup();
    horgrp->attach( alignedBelow, seisgrp );
    horgrp->attach( ensureBelow, sep );
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::Grid2D );
    batchfld_->attach( alignedBelow, horgrp );

    postFinalise().notify( mCB(this,uiCreate2DGrid,finaliseCB) );
}


uiCreate2DGrid::~uiCreate2DGrid()
{
    delete &tkzs_;
}


uiGroup* uiCreate2DGrid::createSeisGroup( const Geometry::RandomLine* rdl )
{
    uiGroup* grp = new uiGroup( this, "Seis group" );

    IOObjContext ctxt = uiSeisSel::ioContext( Seis::Vol, true );
    infld_ = new uiSeisSel( grp, ctxt, uiSeisSel::Setup(Seis::Vol) );
    infld_->selectionDone.notify( mCB(this,uiCreate2DGrid,inpSelCB) );

    bboxfld_ = new uiPosSubSel( grp, uiPosSubSel::Setup(false,true)
				     .withstep(false) );
    bboxfld_->selChange.notify( mCB(this,uiCreate2DGrid,subSelCB) );
    bboxfld_->attach( alignedBelow, infld_ );

    randlinegrdgrp_ = new ui2DGridLinesFromRandLine( grp, tkzs_.hsamp_, rdl );
    randlinegrdgrp_->gridChanged.notify(
			mCB(this,uiCreate2DGrid,updatePreview) );
    if ( rdl )
	randlinegrdgrp_->attach( alignedBelow, bboxfld_ );
    else
    {
	sourceselfld_ = new uiGenInput( grp, uiStrings::phrCreate(
				  tr("Grid from")), BoolInpSpec(true,
				  tr("Inl/Crl"),uiStrings::sRandomLine()) );
	sourceselfld_->valuechanged.notify( mCB(this,uiCreate2DGrid,srcSelCB) );
	sourceselfld_->attach( alignedBelow, bboxfld_ );
	inlcrlgridgrp_ = new ui2DGridLinesFromInlCrl( grp, tkzs_.hsamp_ );
	inlcrlgridgrp_->gridChanged.notify(
			mCB(this,uiCreate2DGrid,updatePreview) );
	inlcrlgridgrp_->attach( alignedBelow, sourceselfld_ );
	randlinegrdgrp_->attach( alignedBelow, sourceselfld_ );
    }

    ctxt = uiSeisSel::ioContext( Seis::Line, false );
    outfld_ = new uiSeisSel( grp, ctxt, uiSeisSel::Setup(Seis::Line) );
    outfld_->setConfirmOverwrite( false );
    outfld_->selectionDone.notify( mCB(this,uiCreate2DGrid,outSelCB) );
    outfld_->attach( alignedBelow, rdl ? randlinegrdgrp_ : inlcrlgridgrp_ );

    grp->setHAlignObj( outfld_ );
    return grp;
}


uiGroup* uiCreate2DGrid::createHorizonGroup()
{
    uiGroup* grp = new uiGroup( this, "Horizon group" );

    horcheckfld_ = new uiCheckBox( grp, uiStrings::phrExtract(
				   tr("horizons for the new grid")),
				   mCB(this,uiCreate2DGrid,horCheckCB) );
    horselfld_ = new uiHorizonParSel( grp, false, true );
    horselfld_->attach( alignedBelow, horcheckfld_ );

    hornmfld_ = new uiGenInput( grp, tr("%1 name prefix")
				     .arg(mJoinUiStrs(sHorizon(),s2D())) );
    hornmfld_->attach( alignedBelow, horselfld_ );

    grp->setHAlignObj( hornmfld_ );
    return grp;
}


uiGroup* uiCreate2DGrid::createPreviewGroup()
{
    uiGroup* grp = new uiGroup( this, "Preview Group" );
    previewmap_ = new uiSurveyMap( grp, false );
    preview_ = new uiGrid2DMapObject;
    preview_->graphItem().setZValue( 1 );
    previewmap_->addObject( preview_ );
    previewmap_->setStretch( 0, 0 );
    previewmap_->setPrefWidth( 300 );
    previewmap_->setPrefHeight( 300 );

    nrinlinesfld_ = new uiLabel( grp, uiStrings::sEmptyString() );
    nrinlinesfld_->setPrefWidthInChar( 40 );
    nrinlinesfld_->attach( centeredBelow, previewmap_ );
    nrcrlinesfld_ = new uiLabel( grp, uiStrings::sEmptyString() );
    nrcrlinesfld_->setPrefWidthInChar( 40 );
    nrcrlinesfld_->attach( centeredBelow, nrinlinesfld_ );
    return grp;
}


void uiCreate2DGrid::horCheckCB( CallBacker* )
{
    horselfld_->display( horcheckfld_->isChecked() );
    hornmfld_->display( horcheckfld_->isChecked() );
}


void uiCreate2DGrid::finaliseCB( CallBacker* )
{
    horCheckCB( 0 );
    srcSelCB( 0 );
    inpSelCB( 0 );
}


void uiCreate2DGrid::inpSelCB( CallBacker* )
{
    uiSeisIOObjInfo info( infld_->key(true) );
    if ( info.getRanges(tkzs_) )
    {
	bboxfld_->setInputLimit( tkzs_ );
	bboxfld_->setInput( tkzs_ );
    }

    subSelCB( 0 );
}


void uiCreate2DGrid::subSelCB( CallBacker* )
{
    tkzs_ = bboxfld_->envelope();
    randlinegrdgrp_->updateRange();
    if ( inlcrlgridgrp_ )
	inlcrlgridgrp_->updateRange();
}


void uiCreate2DGrid::srcSelCB( CallBacker* )
{
    if ( !sourceselfld_ )
	return;

    const bool isinlcrlbased = sourceselfld_->getBoolValue();
    inlcrlgridgrp_->display( isinlcrlbased );
    randlinegrdgrp_->display( !isinlcrlbased );

    if ( !isinlcrlbased )
    {
	mDynamicCastGet(ui2DGridLinesFromRandLine*,rlgrp,randlinegrdgrp_)
	if ( !rlgrp->hasLine() )
	    rlgrp->processInput();
    }

    ui2DGridLines* grp = isinlcrlbased ? inlcrlgridgrp_ : randlinegrdgrp_;
    grp->updateRange();
}


void uiCreate2DGrid::outSelCB( CallBacker* )
{
    const IOObj* ioobj = outfld_->ioobj();
    hornmfld_->setText( ioobj ? ioobj->name().buf() : "Grid2D-" );
}


void uiCreate2DGrid::updatePreview( CallBacker* )
{
    const bool isinlcrlbased = sourceselfld_ && sourceselfld_->getBoolValue();
    const ui2DGridLines* grp = isinlcrlbased ? inlcrlgridgrp_ : randlinegrdgrp_;
    const Grid2D* grid = grp->getGridLines();
    preview_->setGrid( grid );
    preview_->setBaseLine( grp->getBaseLine() );
    previewmap_->setSurveyInfo( &SI() );
    BufferString nrinlstxt, nrcrlstxt;
    grp->getNrLinesLabelTexts( nrinlstxt, nrcrlstxt );
    nrinlinesfld_->setText( toUiString(nrinlstxt.buf()) );
    nrcrlinesfld_->setText( toUiString(nrcrlstxt.buf()) );
}


void uiCreate2DGrid::fillSeisPar( IOPar& par )
{
    par.set( Seis2DGridCreator::sKeyInput(), infld_->key() );
    par.set( Seis2DGridCreator::sKeyOutput(), outfld_->key() );

    const bool frominlcrl = sourceselfld_ ? sourceselfld_->getBoolValue()
					  : false;
    par.set( Seis2DGridCreator::sKeySelType(),
	     frominlcrl ? "InlCrl" : "Random" );
    if ( frominlcrl )
	inlcrlgridgrp_->fillPar( par );
    else
	randlinegrdgrp_->fillPar( par );

    const TrcKeyZSampling& bbox = bboxfld_->envelope();
    IOPar subselpar;
    bbox.fillPar( subselpar );
    par.mergeComp( subselpar, sKey::Subsel() );
}


void uiCreate2DGrid::fillHorPar( IOPar& par )
{
    const TypeSet<MultiID>& horids = horselfld_->getSelected();
    par.set( Horizon2DGridCreator::sKeyInputIDs(), horids );
    par.set( Horizon2DGridCreator::sKeySeisID(), outfld_->key() );
    par.set( Horizon2DGridCreator::sKeyPrefix(), hornmfld_->text() );
}


bool uiCreate2DGrid::checkInput( IOPar& ) const
{
    return checkLineNames() == 1;
}


int uiCreate2DGrid::checkLineNames() const
{
    BufferStringSet linenames;
    const bool frominlcrl = sourceselfld_ ? sourceselfld_->getBoolValue()
					  : false;
    if ( frominlcrl )
    {
	mDynamicCastGet(ui2DGridLinesFromInlCrl*,grp,inlcrlgridgrp_);
	if ( grp ) grp->getLineNames( linenames );
    }
    else
    {
	mDynamicCastGet(ui2DGridLinesFromRandLine*,grp,randlinegrdgrp_);
	if ( grp ) grp->getLineNames( linenames );
    }

    BufferStringSet ovwrlinenms;
    for ( int lidx=0; lidx<linenames.size(); lidx++ )
    {
	const char* lnm = linenames.get(lidx).buf();
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	if ( geomid != Survey::GeometryManager::cUndefGeomID() )
	    ovwrlinenms.add( lnm );
	else
	    Geom2DImpHandler::getGeomID( lnm ); // adds to database
    }

    if ( ovwrlinenms.isEmpty() )
	return 1;

    uiString msg =
	tr( "The following lines are already there in the database:\n%1\n\n"
	    "Do you want to overwrite them?\n"
	    "If you want to extract data along existing 2D lines without "
	    "overwriting the geometries, choose 'Extract 2D from 3D'." )
	    .arg( ovwrlinenms.getDispString(20) );

    return uiMSG().askGoOnAfter( msg, uiStrings::sCancel(),
		uiStrings::sOverwrite(), m3Dots(tr("Extract 2D from 3D")) );
}


bool uiCreate2DGrid::fillPar()
{
    IOPar par;
    fillSeisPar( par );
    IOPar& batchpar = batchfld_->jobSpec().pars_;
    batchpar.mergeComp( par, "Seis" );

    if ( horcheckfld_->isChecked() )
    {
	par.setEmpty();
	fillHorPar( par );
	batchpar.mergeComp( par, "Horizon" );
    }

    if ( !SI().has2D() )
	uiMSG().warning( tr("You need to change survey type to 'Both 2D and 3D'"
			    " in survey setup to display the 2D lines") );

    return true;
}


bool uiCreate2DGrid::acceptOK( CallBacker* )
{
    if ( !infld_->ioobj() || !outfld_->ioobj() )
	return false;

    const int res = checkLineNames();
    if ( res < 0 )
	return false;

    if ( res == 0 )
    {
	uiSeis2DFrom3D dlg( this );
	dlg.go();
	return true;
    }

    if ( !fillPar() )
	return false;

    batchfld_->setJobName( outfld_->ioobj()->name() );
    batchfld_->start();
    return false;
}
