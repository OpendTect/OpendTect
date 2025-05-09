/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "interpol1d.h"
#include "ioman.h"
#include "latlong.h"
#include "linear.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uigisexp.h"
#include "uigisexpdlgs.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbutton.h"

mDefineInstanceCreatedNotifierAccess(ui2DGeomManageDlg)

static IOObjContext mkCtxt()
{
    IOObjContext ret( mIOObjContext(SurvGeom2D) );
    return ret;
}

ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiObjFileMan(p,Setup(uiStrings::phrManage( tr("2D Geometry")),
			   mODHelpKey(m2DGeomManageDlgHelpID))
			.nrstatusflds(1).modal(false),mkCtxt())
{
    ctxt_.toselect_.allownonuserselectable_ = false;
    createDefaultUI( false, false );
    addManipButton( "delete", tr("Delete this Line"),
			mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    addManipButton( "browse2dgeom", tr("Manage Line Geometry"),
			mCB(this,ui2DGeomManageDlg,manLineGeom) );
    mTriggerInstanceCreatedNotifier();
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


void ui2DGeomManageDlg::manLineGeom( CallBacker* )
{
    if ( !curioobj_ )
	return;

    PtrMan<Translator> transl = curioobj_->createTranslator();
    if ( !transl )
	return;

    BoolTypeSet geomisro;
    TypeSet<Pos::GeomID> geomidset;
    TypeSet<MultiID> selids;
    getChosen( selids );
    for ( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids[idx] );
	if ( !ioobj )
	    continue;

	const BufferString linenm( ioobj->name() );
	const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
	if ( !geomid.isValid() )
	    continue;

	geomidset += geomid;
	geomisro += ioobj->implReadOnly();
    }

    if ( geomidset.isEmpty() )
    {
	uiMSG().error( tr("No valid geometry objects selected") );
	return;
    }

    const bool readonly = geomidset.size()>1 || geomisro.first();
    uiManageLineGeomDlg dlg( this, geomidset, readonly );
    dlg.go();
}


void ui2DGeomManageDlg::ownSelChg()
{
}


void ui2DGeomManageDlg::mkFileInfo()
{
    if ( !curioobj_ )
    {
	setInfo( getFileInfo() );
	return;
    }

    BufferString txt;

    const BufferString linenm = curioobj_->name();
    const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;

    if ( geom2d )
    {
	const StepInterval<int> trcrg = geom2d->data().trcNrRange();
	Interval<float> sprg;
	sprg.setUdf();
	if ( !geom2d->spnrs().isEmpty() )
	    sprg.set( geom2d->spnrs().first(), geom2d->spnrs().last() );
	const ZDomain::Info& zinfo = SI().zDomainInfo();
	ZSampling zrg = geom2d->zRange();
	const int nrzdec = zinfo.nrDecimals( zrg.step_, false );
	if ( !zrg.isUdf() )
	    zrg.scale( zinfo.userFactor() );

	const float linelength = geom2d->lineLength();
	const BufferString diststr = toStringDec(geom2d->averageTrcDist(),2);
	const BufferString lengthstr = toString(linelength,0,'f',0);
	const BufferString unitstr = SI().getXYUnitString();
	txt.add( "Number of traces: " ).add( trcrg.nrSteps()+1 )
		.add( "\nTrace range: " ).add( trcrg.start_ ).add( " - " )
		.add( trcrg.stop_ );
	if ( !sprg.isUdf() )
	    txt.add( "\nShotpoint range: ").add( sprg.start_ ).add( " - " )
		    .add( sprg.stop_ );
	if ( !zrg.isUdf() )
	{
	    txt.addNewLine().add( zinfo.getRange() )
	       .add( ": " ).add( toString(zrg.start_,0,'f',nrzdec) )
	       .add( " - " ).add( toString(zrg.stop_,0,'f',nrzdec) )
	       .add( " [" ).add( toString(zrg.step_,0,'f',nrzdec) ).add( "]" );
	}

	const UnitOfMeasure* uomfrom = UoMR().get(SI().getXYUnitString(false));
	const UnitOfMeasure* uomto = nullptr;
	if ( SI().xyInFeet() )
	    uomto = UoMR().get( "mile" );
	else
	    uomto = UoMR().get( "kilometer" );

	const float length2 = getConvertedValue( linelength, uomfrom, uomto );
	const BufferString length2str = toString(length2,0,'f',2);
	const BufferString unit2str = uomto->symbol();

	txt.add( "\nAverage distance: " ).add( diststr ).addSpace().add(unitstr)
	   .add( "\nLine length: " ).add( lengthstr ).addSpace().add( unitstr )
	   .add( " / " ).add( length2str ).addSpace()
			.add("(").add(unit2str ).add(")")
	   .addNewLine();
    }

    txt.add( getFileInfo() );
    setInfo( txt );
}


void ui2DGeomManageDlg::lineRemoveCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool docont = uiMSG().askContinue(
       tr("All selected 2D line geometries will be deleted.\n"
	  "This will invalidate all data and interpretations associated with "
	  "these lines"));
    if ( !docont )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
    uiStringSet msgs;
    TypeSet<MultiID> selids;
    getChosen( selids );
    for ( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids[idx] );
	if ( !ioobj || ioobj->implReadOnly() )
	    continue;

	const BufferString lnm( ioobj->name() );
	if ( !IOM().implRemove(*ioobj) )
	{
	    msgs += tr("Cannot remove %1").arg(lnm);
	    continue;
	}

	const Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	IOM().permRemove( ioobj->key() );
	Survey::GMAdmin().removeGeometry( geomid );
    }

    chgr.restore();
    fullUpdate( MultiID::udf() );

    if ( !msgs.isEmpty() )
	uiMSG().errorWithDetails(msgs);
}


// uiInterpolateGeomDlg
class uiInterpolateGeomDlg : public uiDialog
{
mODTextTranslationClass(uiInterpolateGeomDlg)
public:
uiInterpolateGeomDlg( uiParent* p, const Survey::Geometry2D& geom2d )
    : uiDialog(p,Setup(tr("Interpolate Geometry"),mODHelpKey(mTrc2SPHelpID)))
    , geom2d_(geom2d)
{
    const Interval<int> trcrg = geom2d.data().trcNrRange();
    trcnrfld_ = new uiGenInput( this, tr("Trace range"),
		IntInpIntervalSpec(trcrg).setLimits(trcrg) );
    auto* lbl = new uiLabel( this, tr("Traces outside this range will "
				      "be removed.") );
    lbl->attach( alignedBelow, trcnrfld_ );
}


~uiInterpolateGeomDlg()
{}


bool acceptOK( CallBacker* ) override
{
    const Interval<int> trcnrrg = trcnrfld_->getIInterval();

    newgeom_ = new Survey::Geometry2D( geom2d_.getName() );
    const TypeSet<float>& spnrs = geom2d_.spnrs();
    const TypeSet<PosInfo::Line2DPos>& linepos = geom2d_.data().positions();
    const bool usesp = linepos.size() == spnrs.size();
    const int nrtrcs = geom2d_.size();
    for ( int idx=0; idx<nrtrcs-1; idx++ )
    {
	const int t0 = linepos[idx].nr_;
	if ( !trcnrrg.includes(t0,false) )
	    continue;

	const float sp0 = usesp ? spnrs[idx] : t0;
	const Coord& crd0 = linepos[idx].coord_;
	newgeom_->add( crd0, t0, sp0 );

	const int t1 = linepos[idx+1].nr_;
	const float sp1 = usesp ? spnrs[idx+1] : t1;
	const Coord& crd1 = linepos[idx+1].coord_;
	if ( t1-t0 == 1 )
	{
	    newgeom_->add( crd1, t1, sp1 );
	    continue;
	}

	for ( int trcnr=t0+1; trcnr<t1; trcnr++ )
	{
	    const double newx =
                    Interpolate::linear1D( t0, crd0.x_, t1, crd1.x_, trcnr );
	    const double newy =
                    Interpolate::linear1D( t0, crd0.y_, t1, crd1.y_, trcnr );
	    const float newsp = !usesp ? float(trcnr) :
		Interpolate::linear1D( t0, sp0, t1, sp1, trcnr );
	    newgeom_->add( newx, newy, trcnr, newsp );
	}

	if ( idx == nrtrcs-2 )
	    newgeom_->add( crd1, t1, sp1 );
    }

    return true;
}

    uiGenInput*				trcnrfld_;
    const Survey::Geometry2D&		geom2d_;
    RefMan<Survey::Geometry2D>		newgeom_;


}; // class uiInterpolateGeomDlg


static LinePars getRelation( int t0, float sp0, int t1, float sp1, bool forsp )
{
    const int dtrc = t1 - t0;
    const float dsp = sp1 - sp0;
    if ( dtrc==0 || mIsZero(dsp,mDefEpsF) )
	return LinePars(0,0);

    LinePars lp;
    if ( forsp )
    {
	lp.ax = dsp / float(dtrc);
	lp.a0 = sp0 - lp.ax * t0;
    }
    else
    {
	lp.ax = float(dtrc) / dsp;
	lp.a0 = t0 - lp.ax * sp0;
    }

    return lp;
}


// uiTrc2SPDlg
class uiTrc2SPDlg : public uiDialog
{ mODTextTranslationClass(uiTrc2SPDlg)
public:
uiTrc2SPDlg( uiParent* p, const Survey::Geometry2D& geom2d )
    : uiDialog(p,Setup(tr("Set Trace Number vs SP Number Relationship"),
		       mODHelpKey(mTrc2SPHelpID)))
    , geom2d_(geom2d)
{
    dirfld_ = new uiGenInput( this, tr("Calculate"),
	BoolInpSpec(true,uiStrings::sSPNumber(),uiStrings::sTraceNumber()) );
    dirfld_->valueChanged.notify( mCB(this,uiTrc2SPDlg,dirChg) );

    auto* tb = new uiToolButton( this, "math", tr("Calculate from 2 points") );
    tb->attach( rightOf, dirfld_ );
    mAttachCB( tb->activated, uiTrc2SPDlg::mathCB );

    LinePars lp = getRelation( geom2d.data().positions().first().nr_,
			       geom2d.spnrs().first(),
			       geom2d.data().positions().last().nr_,
			       geom2d.spnrs().last(), true );

    uiString splbl = toUiString( "%1 =" ).arg( uiStrings::sSPNumber() );
    spincrfld_ = new uiGenInput( this, splbl, FloatInpSpec(lp.ax) );
    spincrfld_->attach( alignedBelow, dirfld_ );
    spstartfld_ = new uiGenInput( this, toUiString("x TrcNr +"),
				  FloatInpSpec(lp.a0) );
    spstartfld_->attach( rightTo, spincrfld_ );

   lp = getRelation( geom2d.data().positions().first().nr_,
		     geom2d.spnrs().first(),
		     geom2d.data().positions().last().nr_,
		     geom2d.spnrs().last(), false );

    uiString trclbl = toUiString( "%1 =" ).arg( uiStrings::sTraceNumber() );
    trcincrfld_ = new uiGenInput( this, trclbl, FloatInpSpec(lp.ax) );
    trcincrfld_->attach( alignedBelow, dirfld_ );
    trcstartfld_ = new uiGenInput( this, toUiString("x SP +"),
				   FloatInpSpec(lp.a0) );
    trcstartfld_->attach( rightTo, trcincrfld_ );

    dirChg( 0 );
}



bool calcSP() const
{
    return dirfld_->getBoolValue();
}


LinePars getRelationship() const
{
    return calcSP()
	? LinePars( spstartfld_->getFValue(), spincrfld_->getFValue() )
	: LinePars( trcstartfld_->getFValue(), trcincrfld_->getFValue() );
}


protected:
void dirChg( CallBacker* )
{
    const bool calcsp = calcSP();
    spincrfld_->display( calcsp );
    spstartfld_->display( calcsp );
    trcincrfld_->display( !calcsp );
    trcstartfld_->display( !calcsp );
}


class uiRelDlg : public uiDialog
{
mODTextTranslationClass(uiRelDlg)
public:
uiRelDlg( uiParent* p, const Survey::Geometry2D& geom2d )
    : uiDialog(p,Setup(tr("Linear Relation Calculator"),mTODOHelpKey))
{
    pt1fld_ = new uiGenInput( this, tr("First trace (TrcNr/SP)"),
			      IntInpSpec(), FloatInpSpec() );
    pt2fld_ = new uiGenInput( this, tr("Second trace (TrcNr/SP)"),
			      IntInpSpec(), FloatInpSpec() );
    pt2fld_->attach( alignedBelow, pt1fld_ );

    pt1fld_->setValue( geom2d.data().positions().first().nr_, 0 );
    pt2fld_->setValue( geom2d.data().positions().last().nr_, 0 );

    if ( !geom2d.spnrs().isEmpty() )
    {
	pt1fld_->setValue( geom2d.spnrs().first(), 1 );
	pt2fld_->setValue( geom2d.spnrs().last(), 1 );
    }
}


LinePars getRelationship( bool forsp )
{
    return getRelation( pt1fld_->getIntValue(0), pt1fld_->getFValue(1),
			pt2fld_->getIntValue(0), pt2fld_->getFValue(1), forsp );
}


    uiGenInput*		pt1fld_;
    uiGenInput*		pt2fld_;

};

void mathCB( CallBacker* )
{
    uiRelDlg dlg( this, geom2d_ );
    if ( !dlg.go() )
	return;

    LinePars lp = dlg.getRelationship( true );
    spstartfld_->setValue( lp.a0 );
    spincrfld_->setValue( lp.ax );

    lp = dlg.getRelationship( false );
    trcstartfld_->setValue( lp.a0 );
    trcincrfld_->setValue( lp.ax );
}


bool acceptOK( CallBacker* ) override
{
    bool isudf = false;
    if ( calcSP() )
	isudf = spstartfld_->isUndef() || spincrfld_->isUndef();
    else
	isudf = trcstartfld_->isUndef() || trcincrfld_->isUndef();

    if ( isudf )
    {
	uiMSG().error( tr("Please enter valid relationship.") );
	return false;
    }

    return true;
}

    const Survey::Geometry2D&	geom2d_;

    uiGenInput*		dirfld_;
    uiGenInput*		trcstartfld_;
    uiGenInput*		trcincrfld_;
    uiGenInput*		spstartfld_;
    uiGenInput*		spincrfld_;

}; // class uiTrc2SPDlg



// uiManageLineGeomDlg
uiManageLineGeomDlg::uiManageLineGeomDlg( uiParent* p,
			const TypeSet<Pos::GeomID>& geomidset, bool readonly )
    : uiDialog(p,Setup(tr("Edit Line Geometry"),
		       mODHelpKey(mManageLineGeomDlgHelpID)))
    , geomidset_(geomidset)
    , readonly_(readonly)
{
    if ( readonly )
    {
	setCtrlStyle( CloseOnly );
	setCaption( tr("Browse Line Geometry") );
    }

    BufferStringSet linenms;
    for ( const auto& geomid : geomidset )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomid))
	if ( geom2d )
	    linenms.add( geom2d->getName() );
    }

    linefld_= new uiGenInput( this, uiStrings::sLineName(),
			StringListInpSpec(linenms));
    mAttachCB( linefld_->valueChanged, uiManageLineGeomDlg::lineSel );
    linefld_->attach( hCentered );

    table_ = new uiTable( this, uiTable::Setup(20,4), "2DGeom" );
    table_->setPrefWidth( 600 );
    table_->setStretch( 2, 2 );
    table_->attach( ensureBelow, linefld_ );
    uiStringSet collbls;
    collbls.add( uiStrings::sTraceNumber() ).add( tr("SP") )
	   .add( uiStrings::sX() ).add( uiStrings::sY() );

    if ( SI().hasProjection() )
	collbls.add( uiStrings::sLat() ).add( uiStrings::sLongitude() );

    table_->setColumnLabels( collbls );
    table_->setTableReadOnly( readonly );

    FloatInpIntervalSpec spec( true );
    uiString zlbl = toUiString( "Default %1 %2" ).arg( uiStrings::sZRange() )
					 .arg( SI().getUiZUnitString());
    rgfld_ = new uiGenInput( this, zlbl, spec );
    rgfld_->attach( centeredBelow, table_ );
    rgfld_->setReadOnly( readonly );

    auto* grp = new uiButtonGroup( this, "buttons", OD::Horizontal );
    if ( !readonly )
    {
	new uiPushButton( grp, tr("Set new Geometry"),
			  mCB(this,uiManageLineGeomDlg,impGeomCB), false );
	new uiPushButton( grp, tr("Set Trace/SP Number"),
			  mCB(this,uiManageLineGeomDlg,setTrcSPNrCB), false );
	new uiPushButton( grp, tr("Interpolate"),
			  mCB(this,uiManageLineGeomDlg,interpolGeomCB), false );
    }

    new uiPushButton( grp, tr("Export Geometry"),
		      mCB(this,uiManageLineGeomDlg,expGeomCB), false );
    grp->attach( centeredBelow, table_ );
    grp->attach( ensureBelow, rgfld_ );

    lineSel( nullptr );
}


uiManageLineGeomDlg::~uiManageLineGeomDlg()
{
    detachAllNotifiers();
}


const Survey::Geometry2D* uiManageLineGeomDlg::selectedGeom() const
{
    const int lineidx = linefld_->getIntValue();
    const Survey::Geometry* geom = geomidset_.validIdx(lineidx) ?
	Survey::GM().getGeometry(geomidset_[lineidx]) : nullptr;
    return geom ? geom->as2D() : nullptr;
}


void uiManageLineGeomDlg::impGeomCB( CallBacker* )
{
    if ( readonly_ )
	return;

    const int lineidx = linefld_->getIntValue();
    if ( !geomidset_.validIdx(lineidx) )
	return;

    const BufferString linenm = Survey::GM().getName( geomidset_[lineidx] );
    uiImp2DGeom dlg( this, linenm );
    if ( !dlg.go() )
	return;

    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( linenm );
    if ( !dlg.fillGeom(*geom) )
	return;

    table_->clearTable();
    fillTable( *geom );
}


void uiManageLineGeomDlg::expGeomCB( CallBacker* )
{
    uiExp2DGeom dlg( this, &geomidset_, true );
    dlg.go();
}


void uiManageLineGeomDlg::interpolGeomCB( CallBacker* )
{
    if ( readonly_ )
	return;

    const auto* geom2d = selectedGeom();
    if ( !geom2d )
	return;

    if ( geom2d->data().trcNrRange().step_ == 1 )
    {
	const bool res = uiMSG().askGoOn(
		tr("Trace numbers already have an increment of 1.\n"
		   "Do you want to continue interpolating the geometry?") );
	if ( !res )
	    return;
    }

    uiInterpolateGeomDlg dlg( this, *geom2d );
    if ( !dlg.go() )
	return;

    if ( dlg.newgeom_ )
	fillTable( *dlg.newgeom_ );
}


void uiManageLineGeomDlg::lineSel( CallBacker* )
{
    const Survey::Geometry2D* geom2d = selectedGeom();
    if ( !geom2d )
	return;

    fillTable( *geom2d );

    StepInterval<float> zrg = geom2d->data().zRange();
    zrg.scale( sCast(float,SI().zDomain().userFactor()) );
    rgfld_->setValue( zrg );
}


void uiManageLineGeomDlg::setTrcSPNrCB( CallBacker* )
{
    const Survey::Geometry2D* geom2d = selectedGeom();
    if ( readonly_ || !geom2d )
	return;

    uiTrc2SPDlg dlg( this, *geom2d );
    if ( !dlg.go() )
	return;

    const bool calcsp = dlg.calcSP();
    const LinePars lp = dlg.getRelationship();
    const int fromidx = calcsp ? 0 : 1;
    const int toidx = calcsp ? 1 : 0;
    const int nrspdec = 3;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const float var = table_->getFValue( RowCol(idx,fromidx) );
	const float val = lp.getValue( var );
	if ( calcsp )
	    table_->setValue( RowCol(idx,toidx), val, 0, 'f', nrspdec );
	else
	    table_->setValue( RowCol(idx,toidx), val );

    }
}


void uiManageLineGeomDlg::fillTable( const Survey::Geometry2D& geom2d )
{
    const TypeSet<PosInfo::Line2DPos>& positions = geom2d.data().positions();
    const TypeSet<float>& spnrs = geom2d.spnrs();
    table_->setNrRows( positions.size() );
    const int nrdec = SI().nrXYDecimals();
    const int nrspdec = 3;
    for ( int idx=0; idx<positions.size(); idx++ )
    {
	const Coord& crd = positions[idx].coord_;
	table_->setValue( RowCol(idx,0), positions[idx].nr_ );
	table_->setValue( RowCol(idx,1), spnrs.validIdx(idx) ? spnrs[idx] : -1,
			  0, 'f', nrspdec );
        table_->setValue( RowCol(idx,2), crd.x_, 0, 'f', nrdec );
        table_->setValue( RowCol(idx,3), crd.y_, 0, 'f', nrdec );

	if ( SI().hasProjection() )
	{
	    const LatLong ll = LatLong::transform( crd );
	    table_->setValue( RowCol(idx,4), ll.lat_, 0, 'f', 5 );
	    table_->setValue( RowCol(idx,5), ll.lng_, 0, 'f', 5 );
	}
    }
}


bool uiManageLineGeomDlg::acceptOK( CallBacker* )
{
    if ( !uiMSG().askGoOn(tr("Do you really want to change the geometry?\n"
			     "This will affect all associated data.")) )
	return false;

    Survey::Geometry2D* geom2d = getNonConst( selectedGeom() );
    if ( !geom2d )
	return true;

    geom2d->setEmpty();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	geom2d->add( table_->getDValue(RowCol(idx,2)),
		     table_->getDValue(RowCol(idx,3)),
		     table_->getIntValue(RowCol(idx,0)),
		     table_->getFValue(RowCol(idx,1)) );
    }

    StepInterval<float> newzrg = rgfld_->getFStepInterval();
    if ( newzrg.isUdf() )
    {
	uiMSG().error( tr("Please set valid Z range") );
	return false;
    }

    newzrg.scale( 1.f/sCast(float,SI().zDomain().userFactor()) );
    geom2d->dataAdmin().setZRange( newzrg );
    geom2d->touch();

    uiString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}



// Geom2DImpHandler
Geom2DImpHandler::Geom2DImpHandler()
{}


Geom2DImpHandler::~Geom2DImpHandler()
{}


Pos::GeomID Geom2DImpHandler::getGeomID( const char* nm, bool ovwok )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( nm );
    if (  geomid.isUdf() )
	return createNewGeom( nm );

    if ( ovwok || confirmOverwrite(nm) )
	setGeomEmpty( geomid );
    else
	return Pos::GeomID::udf();

    return geomid;
}


bool Geom2DImpHandler::getGeomIDs( const BufferStringSet& nms,
				     TypeSet<Pos::GeomID>& geomids, bool ovwok )
{
    geomids.erase();
    TypeSet<int> existingidxs;
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	Pos::GeomID geomid = Survey::GM().getGeomID( nms.get(idx) );
	if ( geomid.isValid() )
	    existingidxs += idx;
	else
	{
	    geomid = createNewGeom( nms.get(idx) );
	    if ( geomid.isUdf() )
		return false;
	}

	geomids += geomid;
    }

    if ( !existingidxs.isEmpty() )
    {
	BufferStringSet existinglnms;
	for ( int idx=0; idx<existingidxs.size(); idx++ )
	    existinglnms.add( nms.get(existingidxs[idx]) );

	if ( ovwok || confirmOverwrite(existinglnms) )
	{
	    for ( int idx=0; idx<existingidxs.size(); idx++ )
		setGeomEmpty( geomids[existingidxs[idx]] );
	}
    }

    return true;
}


void Geom2DImpHandler::setGeomEmpty( Pos::GeomID geomid )
{
    mDynamicCastGet( Survey::Geometry2D*, geom2d,
		     Survey::GMAdmin().getGeometry(geomid) );
    if ( !geom2d )
	return;

    geom2d->dataAdmin().setEmpty();
    geom2d->touch();
}


Pos::GeomID Geom2DImpHandler::createNewGeom( const char* nm )
{
    auto* l2d = new PosInfo::Line2DData( nm );
    auto* newgeom = new Survey::Geometry2D( l2d );
    uiString msg;
    Pos::GeomID geomid = Survey::GMAdmin().addNewEntry( newgeom, msg );
    if ( !Survey::is2DGeom(geomid) )
	uiMSG().error( msg );

    return geomid;
}


bool Geom2DImpHandler::confirmOverwrite( const BufferStringSet& lnms )
{
    if ( lnms.size() == 1 )
	return confirmOverwrite( lnms.get(0) );

    uiString msg =
	tr("The 2D Lines %1 already exist. If you overwrite "
	   "their geometry, all the associated data will be "
	   "affected. Do you still want to overwrite?")
	.arg(lnms.getDispString(5));

    return uiMSG().askOverwrite( msg );
}


bool Geom2DImpHandler::confirmOverwrite( const char* lnm )
{
    uiString msg = tr("The 2D Line '%1' already exists. If you overwrite "
		      "its geometry, all the associated data will be "
		      "affected. Do you still want to overwrite?")
		      .arg(lnm);
    return uiMSG().askOverwrite( msg );
}
