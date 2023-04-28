/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "ioman.h"
#include "linear.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"



static IOObjContext mkCtxt()
{
    IOObjContext ret( mIOObjContext(SurvGeom2D) );
    return ret;
}

ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage( tr("2D Geometry")),
			       mNoDlgTitle, mODHelpKey(m2DGeomManageDlgHelpID))
			       .nrstatusflds(1).modal(false),mkCtxt())
{
    ctxt_.toselect_.allownonuserselectable_ = false;
    createDefaultUI( false, false );
    addManipButton( "delete", tr("Delete this Line"),
			mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    addManipButton( "browse2dgeom", tr("Manage Line Geometry"),
			mCB(this,ui2DGeomManageDlg,manLineGeom) );
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

    TypeSet<MultiID> selids;
    getChosen( selids );
    TypeSet<Pos::GeomID> geomidset;

    for ( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids[idx] );
	if ( !ioobj || ioobj->implReadOnly() )
	    continue;

	const BufferString linenm( ioobj->name() );
	const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
	if ( !geomid.isValid() )
	    continue;

	geomidset += geomid;
    }

    uiManageLineGeomDlg dlg( this, geomidset,
			     !transl->isUserSelectable(false) );
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
	Interval<float> sprg; sprg.setUdf();
	if ( !geom2d->spnrs().isEmpty() )
	    sprg.set( geom2d->spnrs().first(), geom2d->spnrs().last() );
	const BufferString diststr = toString(geom2d->averageTrcDist(),2);
	const BufferString lengthstr = toString(geom2d->lineLength(),0);
	const BufferString unitstr = SI().getXYUnitString();
	txt.add( "Number of traces: " ).add( trcrg.nrSteps()+1 )
	   .add( "\nTrace range: " ).add( trcrg.start ).add( " - " )
	   .add( trcrg.stop );
	if ( !sprg.isUdf() )
	    txt.add( "\nShotpoint range: ").add( sprg.start ).add( " - " )
		.add( sprg.stop );

	txt.add( "\nAverage distance: " ).add( diststr ).addSpace().add(unitstr)
	   .add( "\nLine length: " ).add( lengthstr ).addSpace().add(unitstr)
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



// uiTrc2SPDlg
class uiTrc2SPDlg : public uiDialog
{ mODTextTranslationClass(uiTrc2SPDlg)
public:
uiTrc2SPDlg( uiParent* p )
	: uiDialog(p,Setup(tr("Set Trace Number vs SP Number Relationship" ),
			   mNoDlgTitle,mODHelpKey(mTrc2SPHelpID)))
{
    dirfld_ = new uiGenInput( this, tr("Calculate"),
	BoolInpSpec(true,uiStrings::sSPNumber(),uiStrings::sTraceNumber()) );
    dirfld_->valueChanged.notify( mCB(this,uiTrc2SPDlg,dirChg) );

    uiString splbl = toUiString( "%1 =" ).arg( uiStrings::sSPNumber() );
    spincrfld_ = new uiGenInput( this, splbl, FloatInpSpec(1) );
    spincrfld_->attach( alignedBelow, dirfld_ );
    spstartfld_ = new uiGenInput( this, toUiString("x TrcNr +"),
				  FloatInpSpec(0) );
    spstartfld_->attach( rightTo, spincrfld_ );

    uiString trclbl = toUiString( "%1 =" ).arg( uiStrings::sTraceNumber() );
    trcincrfld_ = new uiGenInput( this, trclbl, FloatInpSpec(1) );
    trcincrfld_->attach( alignedBelow, dirfld_ );
    trcstartfld_ = new uiGenInput( this, toUiString("x SP +"),
				   FloatInpSpec(0) );
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

    uiGenInput*		dirfld_;
    uiGenInput*		trcstartfld_;
    uiGenInput*		trcincrfld_;
    uiGenInput*		spstartfld_;
    uiGenInput*		spincrfld_;
};



// uiManageLineGeomDlg
uiManageLineGeomDlg::uiManageLineGeomDlg( uiParent* p,
			const TypeSet<Pos::GeomID>& geomidset, bool readonly )
    : uiDialog(p,uiDialog::Setup(tr("Edit Line Geometry"),mNoDlgTitle,
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
    for ( int idx=0; idx<geomidset.size(); idx++ )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomidset[idx]));
	if ( geom2d )
	    linenms.add( geom2d->getName() );
    }

    linefld_= new uiGenInput( this, uiStrings::sLineName(),
			StringListInpSpec(linenms));
    mAttachCB( linefld_->valueChanged, uiManageLineGeomDlg::lineSel );
    linefld_->attach( hCentered );

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomidset[0]);
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom);

    const TypeSet<PosInfo::Line2DPos>& positions = geom2d->data().positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, linefld_ );
    table_->setPrefWidth( 400 );
    uiStringSet collbls;
    collbls.add( uiStrings::sTraceNumber() ).add( uiStrings::sSPNumber() )
           .add( uiStrings::sX() ).add( uiStrings::sY() );
    table_->setColumnLabels( collbls );
    if ( readonly )
	table_->setTableReadOnly( true );

    FloatInpIntervalSpec spec( true );
    uiString zlbl = toUiString( "%1 %2" ).arg( uiStrings::sZRange() )
					 .arg( SI().getUiZUnitString());
    rgfld_ = new uiGenInput( this, zlbl, spec );
    rgfld_->attach( centeredBelow, table_ );
    StepInterval<float> zrg = geom2d->data().zRange();
    zrg.scale( sCast(float,SI().zDomain().userFactor()) );
    rgfld_->setValue( zrg );
    rgfld_->setReadOnly( readonly );

    if ( !readonly )
    {
	uiButtonGroup* grp =
		new uiButtonGroup( this, "buttons", OD::Horizontal );
	new uiPushButton( grp, tr("Set new Geometry"),
			  mCB(this,uiManageLineGeomDlg,impGeomCB), false );
	new uiPushButton( grp, tr("Set Trace/SP Number"),
			  mCB(this,uiManageLineGeomDlg,setTrcSPNrCB), false );
	new uiPushButton( grp, tr("Export Geometry"),
			  mCB(this,uiManageLineGeomDlg,expGeomCB), false );
	grp->attach( centeredBelow, table_ );
	grp->attach( ensureBelow, rgfld_ );
    }

    fillTable( *geom2d );
}


uiManageLineGeomDlg::~uiManageLineGeomDlg()
{
    detachAllNotifiers();
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
    if ( readonly_ )
	return;

    uiExp2DGeom dlg( this, &geomidset_, true );
    dlg.go();
}


void uiManageLineGeomDlg::lineSel( CallBacker* )
{
    const int lineidx = linefld_->getIntValue();
    const Survey::Geometry* geom = geomidset_.validIdx(lineidx) ?
	Survey::GM().getGeometry(geomidset_[lineidx]) : nullptr;
    if ( geom && geom->as2D() )
	fillTable( *geom->as2D() );
}


void uiManageLineGeomDlg::setTrcSPNrCB( CallBacker* )
{
    if ( readonly_ )
	return;

    uiTrc2SPDlg dlg( this );
    if ( !dlg.go() )
	return;

    const bool calcsp = dlg.calcSP();
    const LinePars lp = dlg.getRelationship();
    const int fromidx = calcsp ? 0 : 1;
    const int toidx = calcsp ? 1 : 0;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const float var = table_->getFValue( RowCol(idx,fromidx) );
	const float val = lp.getValue( var );
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
	table_->setValue( RowCol(idx,0), positions[idx].nr_ );
	table_->setValue( RowCol(idx,1), spnrs.validIdx(idx) ? spnrs[idx] : -1,
			  nrspdec );
	table_->setValue( RowCol(idx,2), positions[idx].coord_.x, nrdec );
	table_->setValue( RowCol(idx,3), positions[idx].coord_.y, nrdec );
    }
}


bool uiManageLineGeomDlg::acceptOK( CallBacker* )
{
    if (!uiMSG().askGoOn(tr("Do you really want to change the geometry?\n"
			    "This will affect all associated data.")))
	return false;

    const int lineidx = linefld_->getIntValue();
    Survey::Geometry* geom = geomidset_.validIdx(lineidx) ?
	Survey::GMAdmin().getGeometry(geomidset_[lineidx]) : nullptr;
    Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
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
    if (  geomid == mUdfGeomID )
	return createNewGeom( nm );

    if ( ovwok || confirmOverwrite(nm) )
	setGeomEmpty( geomid );

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
	if ( geomid != mUdfGeomID )
	    existingidxs += idx;
	else
	{
	    geomid = createNewGeom( nms.get(idx) );
	    if ( geomid == mUdfGeomID )
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
