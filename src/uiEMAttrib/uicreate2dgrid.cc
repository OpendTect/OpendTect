/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uicreate2dgrid.h"
#include "uibasemap.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiselsurvranges.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uiseissel.h"
#include "uisurvmap.h"
#include "uitaskrunner.h"
#include "uiworld2ui.h"


#include "axislayout.h"
#include "cubesampling.h"
#include "emsurfacetr.h"
#include "emhorizon2d.h"
#include "gridcreator.h"
#include "linekey.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seiscbvs.h"
#include "separstr.h"
#include "survinfo.h"
#include "surv2dgeom.h"



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
    const Coord startpos = SI().transform( line->start_ ); \
    const Coord stoppos = SI().transform( line->stop_ ); \
    const uiPoint start = transform_->transform( startpos ); \
    const uiPoint stop = transform_->transform( stoppos ); \
    uiLineItem* item = new uiLineItem( start, stop, true ); \
    item->setPenStyle( ls ); \
    item->setZValue( graphicszval ); \
    itemgrp_->add( item ); \
}

void update()
{
    itemgrp_->removeAll( true );
    lines_.erase();
    
    if ( !grid_ || !transform_ ) return;

    LineStyle ls;
    int graphicszval = 1;
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

    ls.width_ = 2;
    ls.color_.set( 200, 100, 100 );
    graphicszval = 2;
    if ( baseline_ )
	mDrawLine( baseline_ );
}

protected:

    ObjectSet<uiLineItem>	lines_;
    const Grid2D::Line*		baseline_;
    const Grid2D*		grid_;

};



ui2DGridLines::ui2DGridLines( uiParent* p, const HorSampling& hs )
    : uiGroup(p,"2D Grid Parameters")
    , hs_(hs), grid_(new Grid2D)
    , gridChanged(this)
{
    inlprefixfld_ = new uiGenInput( this, "Prefix for parallel lines" );
    crlprefixfld_ = new uiGenInput( this, "Prefix for perpendicular lines" );
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
						  const HorSampling& hs )
    : ui2DGridLines(p,hs)
{
    inlmodefld_ = new uiGenInput( this, "Inline",
				  BoolInpSpec(true,"Range","Loose") );
    inlmodefld_->valuechanged.notify(mCB(this,ui2DGridLinesFromInlCrl,modeChg));
    inlrgfld_ = new uiSelNrRange( this, uiSelNrRange::Inl, true );
    inlrgfld_->rangeChanged.notify( mCB(this,ui2DGridLinesFromInlCrl,
					paramsChgCB) );
    inlrgfld_->attach( alignedBelow, inlmodefld_ );
    inlsfld_ = new uiGenInput( this, "Inlines (comma separated)" );
    inlsfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromInlCrl,
					paramsChgCB) );
    inlsfld_->attach( alignedBelow, inlmodefld_ );

    crlmodefld_ = new uiGenInput( this, "Crossline",
				  BoolInpSpec(true,"Range","Loose") );
    crlmodefld_->valuechanged.notify(mCB(this,ui2DGridLinesFromInlCrl,modeChg));
    crlmodefld_->attach( alignedBelow, inlrgfld_ );
    crlrgfld_ = new uiSelNrRange( this, uiSelNrRange::Crl, true );
    crlrgfld_->rangeChanged.notify( mCB(this,ui2DGridLinesFromInlCrl,
					paramsChgCB) );
    crlrgfld_->attach( alignedBelow, crlmodefld_ );
    crlsfld_ = new uiGenInput( this, "Crosslines (comma separated)" );
    crlsfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromInlCrl,
					paramsChgCB) );
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
    inlrg.step = 100; crlrg.step = 100;
    inlrgfld_->setRange( inlrg );
    inlrgfld_->setLimitRange( inlrg );
    crlrgfld_->setRange( crlrg );
    crlrgfld_->setLimitRange( crlrg );
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
    inltxt = "Nr of Inlines in grid: ";
    inltxt += grid_->size( true );
    crltxt = "Nr of Crosslines in grid: ";
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
	linename += grid_->getLine( inlidx, true )->start_.inl;
	linenames.add( linename );
    }

    for ( int crlidx=0; crlidx < grid_->size(false); crlidx++ )
    {
	BufferString linename( crlprefixfld_->text() );
	linename += grid_->getLine( crlidx, false )->start_.crl;
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
			const HorSampling& hs, const Geometry::RandomLine* rdl )
    : ui2DGridLines(p,hs)
    , rdlfld_(0)
{
    const BinID startnode = rdl ? rdl->nodePosition(0) : BinID::udf();
    const BinID stopnode = rdl ? rdl->nodePosition( rdl->nrNodes() - 1 )
			       : BinID::udf();
    baseline_ = new Grid2D::Line( startnode, stopnode );
    BufferString lbltxt( "Parallel line spacing " );
    lbltxt += SI().getXYUnitString();
    const StepInterval<int> spacinglimits( 500, 1000000, 500 );
    pardistfld_ = new uiGenInput( this, lbltxt.buf(), IntInpSpec()
	    					.setLimits(spacinglimits) );
    pardistfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromRandLine,
					  paramsChgCB) );	
    if ( !rdl || rdl->nrNodes() != 2 )
    {
	rdlfld_ = new uiIOObjSel( this, mIOObjContext(RandomLineSet),
				  "Input RandomLine" );
	rdlfld_->selectionDone.notify( mCB(this,ui2DGridLinesFromRandLine,
		    			   paramsChgCB) );
	pardistfld_->attach( alignedBelow, rdlfld_ );
    }

    lbltxt = "Perpendicular line spacing ";
    lbltxt += SI().getXYUnitString();
    perdistfld_ = new uiGenInput( this, lbltxt.buf(), IntInpSpec()
	    					.setLimits(spacinglimits) );
    perdistfld_->valuechanged.notify( mCB(this,ui2DGridLinesFromRandLine,
					  paramsChgCB) );	
    perdistfld_->attach( alignedBelow, pardistfld_ );

    const Coord maxcoord = SI().maxCoord(false);
    const Coord mincoord = SI().minCoord(false);
    const float maxdim = (float) mMAX( maxcoord.x-mincoord.x, maxcoord.y-mincoord.y );
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
	BufferString msg = "There are too many lines in the grid";
	msg += "You may want to increase the grid spacing to get fewer lines.";
	if ( !uiMSG().askGoOn(msg.buf(),"Continue anyway","Cancel") )
	    return false;
    }

    return grid_->totalSize();
}


void ui2DGridLinesFromRandLine::paramsChgCB( CallBacker* cb )
{
    if ( cb && cb == rdlfld_ )
    {
	if ( !rdlfld_->ioobj(true) )
	    return;

	Geometry::RandomLineSet geom;
	BufferString msg;
	if ( !RandomLineSetTranslator::retrieve(geom,rdlfld_->ioobj(),msg) )
	{
	    uiMSG().error( msg );
	    return;
	}

	const Geometry::RandomLine* rdl = geom.isEmpty() ? 0 : geom.lines()[0];
	baseline_->start_ = rdl->nodePosition( 0 );
	baseline_->stop_ = rdl->nodePosition( rdl->nrNodes() - 1 );
    }

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
    : uiFullBatchDialog(p,uiFullBatchDialog::Setup("Create 2D Seismic grid")
			  .procprognm("od_process_2dgrid"))
    , sourceselfld_(0),inlcrlgridgrp_(0)
    , cs_(*new CubeSampling(true))
{
    setHelpID( "103.2.23" );
    setParFileNmDef( "seis2dgrid" );
    uiGroup* seisgrp = createSeisGroup( rdl );

    uiGroup* previewgrp = createPreviewGroup();
    previewgrp->attach( rightTo, seisgrp );

    uiSeparator* sep = new uiSeparator( uppgrp_, "HSeparator", true );
    sep->attach( stretchedBelow, previewgrp );
    sep->attach( ensureBelow, seisgrp );

    uiGroup* horgrp = createHorizonGroup();
    horgrp->attach( alignedBelow, seisgrp );
    horgrp->attach( ensureBelow, sep );
    addStdFields( false, true );
    uppgrp_->setHAlignObj( horgrp );

    postFinalise().notify( mCB(this,uiCreate2DGrid,finaliseCB) );
}


uiCreate2DGrid::~uiCreate2DGrid()
{
    delete &cs_;
}


uiGroup* uiCreate2DGrid::createSeisGroup( const Geometry::RandomLine* rdl )
{
    uiGroup* grp = new uiGroup( uppgrp_, "Seis group" );

    IOObjContext ctxt = mIOObjContext( SeisTrc );
    ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
    ctxt.forread = true;
    infld_ = new uiSeisSel( grp, ctxt, uiSeisSel::Setup(Seis::Vol) );
    infld_->selectionDone.notify( mCB(this,uiCreate2DGrid,inpSelCB) );

    bboxfld_ = new uiPosSubSel( grp, uiPosSubSel::Setup(false,true)
				     .withstep(false) );
    bboxfld_->selChange.notify( mCB(this,uiCreate2DGrid,subSelCB) );
    bboxfld_->attach( alignedBelow, infld_ );

    randlinegrdgrp_ = new ui2DGridLinesFromRandLine( grp, cs_.hrg, rdl );
    randlinegrdgrp_->gridChanged.notify( mCB(this,uiCreate2DGrid,
					      updatePreview) );
    if ( rdl )
	randlinegrdgrp_->attach( alignedBelow, bboxfld_ );
    else
    {
	sourceselfld_ = new uiGenInput( grp, "Create Grid from",
				  BoolInpSpec(true,"Inl/Crl","Random line") );
	sourceselfld_->valuechanged.notify( mCB(this,uiCreate2DGrid,srcSelCB) );
	sourceselfld_->attach( alignedBelow, bboxfld_ );
	inlcrlgridgrp_ = new ui2DGridLinesFromInlCrl( grp, cs_.hrg );
	inlcrlgridgrp_->gridChanged.notify( mCB(this,uiCreate2DGrid,
		    				 updatePreview) );
	inlcrlgridgrp_->attach( alignedBelow, sourceselfld_ );
	randlinegrdgrp_->attach( alignedBelow, sourceselfld_ );
    }

    ctxt.forread = false;
    ctxt.deftransl = "2D";
    outfld_ = new uiSeisSel( grp, ctxt, uiSeisSel::Setup(Seis::Line) );
    outfld_->setConfirmOverwrite( false );
    outfld_->selectionDone.notify( mCB(this,uiCreate2DGrid,outSelCB) );
    outfld_->attach( alignedBelow, rdl ? randlinegrdgrp_ : inlcrlgridgrp_ );

    grp->setHAlignObj( outfld_ );
    return grp;
}


uiGroup* uiCreate2DGrid::createHorizonGroup()
{
    uiGroup* grp = new uiGroup( uppgrp_, "Horizon group" );

    horcheckfld_ = new uiCheckBox( grp, "Extract horizons for the new grid",
	    			   mCB(this,uiCreate2DGrid,horCheckCB) );
    IOObjContext horctxt = mIOObjContext( EMHorizon3D );
    horselfld_ = new uiIOObjSelGrp( grp, horctxt, "Select horizons", true );
    horselfld_->attach( alignedBelow, horcheckfld_ );

    hornmfld_ = new uiGenInput( grp, "Horizon 2D name prefix" );
    hornmfld_->attach( alignedBelow, horselfld_ );

    grp->setHAlignObj( hornmfld_ );
    return grp;
}


uiGroup* uiCreate2DGrid::createPreviewGroup()
{
    uiGroup* grp = new uiGroup( uppgrp_, "Preview Group" );
    previewmap_ = new uiSurveyMap( grp, false );
    preview_ = new uiGrid2DMapObject;
    preview_->itemGrp()->setZValue( 1 );
    previewmap_->addObject( preview_ );
    previewmap_->setStretch( 0, 0 );
    previewmap_->setPrefWidth( 300 );
    previewmap_->setPrefHeight( 300 );

    nrinlinesfld_ = new uiLabel( grp, "Nr of inlines in grid: XXX" );
    nrinlinesfld_->setPrefWidthInChar( 40 );
    nrinlinesfld_->attach( centeredBelow, previewmap_ );
    nrcrlinesfld_ = new uiLabel( grp, "Nr of crosslines in grid: XXX" );
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
    if ( info.getRanges(cs_) )
    {
	bboxfld_->setInputLimit( cs_ );
	bboxfld_->setInput( cs_ );
    }

    subSelCB( 0 );
}


void uiCreate2DGrid::subSelCB( CallBacker* )
{
    cs_ = bboxfld_->envelope();
    randlinegrdgrp_->updateRange();
    if ( inlcrlgridgrp_ )
	inlcrlgridgrp_->updateRange();
}


void uiCreate2DGrid::srcSelCB( CallBacker* )
{
    if ( !sourceselfld_ )
	return;

    inlcrlgridgrp_->display( sourceselfld_->getBoolValue() );
    randlinegrdgrp_->display( !sourceselfld_->getBoolValue() );
    updatePreview( 0 );
}


void uiCreate2DGrid::outSelCB( CallBacker* )
{
    const IOObj* ioobj = outfld_->ioobj();
    hornmfld_->setText( ioobj ? ioobj->name() : "Grid2D-" );
}


void uiCreate2DGrid::updatePreview( CallBacker* )
{
    const ui2DGridLines* grp = sourceselfld_ && sourceselfld_->getBoolValue() ? 				inlcrlgridgrp_ : randlinegrdgrp_;
    const Grid2D* grid = grp->getGridLines();
    preview_->setGrid( grid );
    preview_->setBaseLine( grp->getBaseLine() );
    previewmap_->drawMap( &SI() );
    BufferString nrinlstxt, nrcrlstxt;
    grp->getNrLinesLabelTexts( nrinlstxt, nrcrlstxt );
    nrinlinesfld_->setText( nrinlstxt.buf() );
    nrcrlinesfld_->setText( nrcrlstxt.buf() );
}


void uiCreate2DGrid::fillSeisPar( IOPar& par )
{
    par.set( Seis2DGridCreator::sKeyInput(), infld_->key() );
    par.set( Seis2DGridCreator::sKeyOutput(), outfld_->key() );
    par.set( sKey::Attribute(), outfld_->attrNm() );

    const bool frominlcrl = sourceselfld_ ? sourceselfld_->getBoolValue()
					  : false;
    par.set( Seis2DGridCreator::sKeySelType(),
	     frominlcrl ? "InlCrl" : "Random" );
    if ( frominlcrl )
	inlcrlgridgrp_->fillPar( par );
    else
	randlinegrdgrp_->fillPar( par );

    const CubeSampling& bbox = bboxfld_->envelope();
    bbox.fillPar( par );
}


void uiCreate2DGrid::fillHorPar( IOPar& par )
{
    BufferStringSet horids;
    for ( int idx=0; idx<horselfld_->nrSel(); idx++ )
	horids.add( horselfld_->selected(idx).buf() );

    par.set( Horizon2DGridCreator::sKeyInputIDs(), horids );
    par.set( Horizon2DGridCreator::sKeySeisID(), outfld_->key() );
    par.set( sKey::Attribute(), outfld_->attrNm() );
    par.set( Horizon2DGridCreator::sKeyPrefix(), hornmfld_->text() );
}


bool uiCreate2DGrid::checkInput() const
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

    S2DPOS().setCurLineSet( outfld_->ioobj()->name() );

    BufferStringSet ovwrlinenms;
    for ( int lidx=0; lidx < linenames.size(); lidx++ )
    {
	if ( S2DPOS().hasLine(linenames.get(lidx)) )
	    ovwrlinenms.add( linenames.get(lidx) );
    }

    if ( ovwrlinenms.isEmpty() )
	return true;

    Seis2DLineSet s2dls( *outfld_->ioobj() );
    BufferStringSet lines;
    s2dls.getLineNamesWithAttrib( lines, outfld_->attrNm() );

    BufferStringSet linestoremove;
    for ( int idx=0; idx<ovwrlinenms.size(); idx++ )
    {
	BufferString line = ovwrlinenms.get( idx );
	if ( !lines.isPresent(line) )
	    linestoremove.add( line );
    }

    while ( linestoremove.size() )
    {
	int lineidx = ovwrlinenms.indexOf( linestoremove.get(0) );
	if ( lineidx >= 0 )
	    ovwrlinenms.removeSingle( lineidx );
	linestoremove.removeSingle( 0 );
    }


    if ( !ovwrlinenms.isEmpty() )
    {
	BufferString msg( "Following lines are already there. Do you want to "
			  "overwrite? Lines : " );
	for ( int idx=0; idx<ovwrlinenms.size(); idx++ )
	{
	    msg += ovwrlinenms.get(idx);
	    msg += idx == ovwrlinenms.size()-1 ? ", " : ".";
	}

	bool res = uiMSG().askGoOn( msg );
	if ( res )
	{
	    for ( int idx=0; idx<ovwrlinenms.size(); idx++ )
		PosInfo::POS2DAdmin().removeLine( ovwrlinenms.get(idx) );
	}

	return res;
    }

    return true;

}


bool uiCreate2DGrid::fillPar( IOPar& batchpar )
{
    if ( !infld_->ioobj() ) return false;
    if ( !outfld_->ioobj() ) return false;

    IOPar par;

    if ( !checkInput() )
	return false;

    fillSeisPar( par );
    batchpar.mergeComp( par, "Seis" );

    if ( horcheckfld_->isChecked() )
    {
	par.setEmpty();
	fillHorPar( par );
	batchpar.mergeComp( par, "Horizon" );
    }

    return true;
}

