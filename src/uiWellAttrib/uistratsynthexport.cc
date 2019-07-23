/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/

#include "uistratsynthexport.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipicksetsel.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "ioobjctxt.h"
#include "od_helpids.h"
#include "picksetmanager.h"
#include "posinfo2dsurv.h"
#include "prestackgather.h"
#include "prestacksynthdataset.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seisbufadapters.h"
#include "stratlevel.h"
#include "stratsynthdatamgr.h"
#include "stratsynthexp.h"
#include "stratsynthlevel.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "synthseisdataset.h"
#include "velocitycalc.h"


#define mErrRet( msg, rettyp ) \
{ \
    uiMSG().error( msg ); \
    return rettyp;\
}


class uiStratSynthOutSel : public uiCheckedCompoundParSel
{ mODTextTranslationClass(uiStratSynthOutSel);
public:

uiStratSynthOutSel( uiParent* p, const uiString& seltxt,
		    const BufferStringSet& nms )
    : uiCheckedCompoundParSel( p, seltxt, false, uiStrings::sSelect() )
    , nms_(nms)
    , nm_(seltxt)
{
    butPush.notify( mCB(this,uiStratSynthOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(nm_), mNoDlgTitle,
			mODHelpKey(mStartSynthOutSelHelpID) );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, toString(nm_) );
    lb->setMultiChoice( true );
    lb->addItems( nms_ );

    if ( itmsarelevels_ )
    {
	const auto& lvls = Strat::LVLS();
	MonitorLock ml( lvls );
	for ( int idx=0; idx<nms_.size(); idx++ )
	{
	    const Strat::Level::ID id = lvls.getIDByIdx( idx );
	    lb->setColorIcon( idx, Strat::LVLS().colorOf(id) );
	}
    }

    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setChosen( selidxs_[idx], true );

    if ( dlg.go() )
    {
	selidxs_.erase();
	selnms_.setEmpty();
	for ( int idx=0; idx<lb->size(); idx++ )
	{
	    if ( lb->isChosen(idx) )
	    {
		selidxs_ += idx;
		selnms_.add( lb->itemText(idx) );
	    }
	}
    }
}


virtual uiString getSummary() const
{
    uiString ret;
    const int sz = nms_.size();
    const int selsz = selidxs_.size();

    if ( sz < 1 )
	ret = tr("None available").embedFinalState();
    else if ( selsz == 0 )
	ret = tr("None selected").embedFinalState();
    else
    {
	if ( selsz > 1 )
	{
	    ret = toUiString("%1 %2").arg(selsz).arg(uiStrings::sSelected());
	    if ( sz == selsz )
		ret.appendPhrase( uiStrings::sAll().toLower().parenthesize(),
				    uiString::Space,uiString::OnSameLine );
	    ret.embedFinalState();
	}
	ret.appendPhrase(toUiString(nms_.get( selidxs_[0] )), uiString::NoSep);
	if ( selsz > 1 )
	    ret.appendPlainText( ", ..." );
	else if ( sz == selsz )
	    ret.appendPhrase( uiStrings::sAll().toLower().parenthesize(),
				    uiString::Space,uiString::OnSameLine );
    }

    return ret;
}

    const uiString	    nm_;
    const BufferStringSet   nms_;
    BufferStringSet	    selnms_;
    TypeSet<int>	    selidxs_;
    bool		    itmsarelevels_	= false;

    uiListBox*		    listfld_;

};



uiStratSynthExport::uiStratSynthExport( uiParent* p,
					const StratSynth::DataMgr& ssdm )
    : uiDialog(p,uiDialog::Setup(tr("Save synthetic seismics and horizons"),
				 mNoDlgTitle,
				 mODHelpKey(mStratSynthExportHelpID) ) )
    , datamgr_(ssdm)
    , randlinesel_(0)
{
    crnewfld_ = new uiGenInput( this, tr("2D Line"),
			     BoolInpSpec(true,uiStrings::phrCreate(
			     uiStrings::sNew()), tr("Use existing")) );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );

    newlinenmfld_ = new uiGenInput( this, tr("New Line Name"),
				    StringInpSpec() );
    newlinenmfld_->attach( alignedBelow, crnewfld_ );
    existlinenmsel_ = new uiSeis2DLineNameSel( this, true );
    existlinenmsel_->fillWithAll();
    existlinenmsel_->attach( alignedBelow, crnewfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hsep 1" );
    sep->attach( stretchedBelow, existlinenmsel_ );

    geomgrp_ = new uiGroup( this, "Geometry group" );
    fillGeomGroup();
    geomgrp_->attach( alignedBelow, existlinenmsel_ );
    geomgrp_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hsep 2" );
    sep->attach( stretchedBelow, geomgrp_ );

    uiGroup* selgrp = new uiGroup( this, "Export sel group" );
    selgrp->attach( ensureBelow, sep );

    BufferStringSet postnms, prenms;
    datamgr_.getNames( postnms, DataMgr::NoPS );
    datamgr_.getNames( prenms, DataMgr::OnlyPS );

    poststcksel_ = new uiStratSynthOutSel( selgrp, tr("Post-stack line data"),
					   postnms );

    repludfsfld_ = new uiCheckBox( selgrp, tr("Fill undefs") );
    repludfsfld_->setChecked( true );
    repludfsfld_->attach( rightOf, poststcksel_ );

    BufferStringSet nms; Strat::LVLS().getNames( nms );
    horsel_ = new uiStratSynthOutSel( selgrp, uiStrings::s2DHorizon(mPlural)
							    .toLower(), nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    horsel_->itmsarelevels_ = true;

    prestcksel_ = new uiStratSynthOutSel( selgrp, tr("PreStack Data"), prenms );
    prestcksel_->attach( alignedBelow, horsel_ );

    selgrp->setHAlignObj( poststcksel_ );
    selgrp->attach( alignedBelow, geomgrp_ );

    sep = new uiSeparator( this, "Hsep 3" );
    sep->attach( stretchedBelow, selgrp );

    auto* fixgrp = new uiGroup( this, "Pre/Postfix group" );
    auto* lbl1 = new uiLabel( fixgrp,
		tr("Output object names will be generated.") );
    auto* lbl2 = new uiLabel( fixgrp,
		tr("You can specify an optional prefix and postfix for each:"));
    lbl2->attach( centeredBelow, lbl1 );
    auto* fixfldgrp = new uiGroup( fixgrp, "Pre/Postfix fields group" );
    prefxfld_ = new uiGenInput( fixfldgrp, uiStrings::sPrefix() );
    prefxfld_->setElemSzPol( uiObject::Small );
    postfxfld_ = new uiGenInput( fixfldgrp, uiStrings::sPostfix() );
    postfxfld_->setElemSzPol( uiObject::Small );
    postfxfld_->attach( rightOf, prefxfld_ );
    fixfldgrp->attach( centeredBelow, lbl2 );
    fixgrp->attach( centeredBelow, selgrp );
    fixgrp->attach( ensureBelow, sep );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
}


void uiStratSynthExport::fillGeomGroup()
{
    StringListInpSpec inpspec;
    inpspec.addString( tr("Straight line") );
    inpspec.addString( uiStrings::sPolygon() );
    const bool haverl = SI().has3D();
    if ( haverl )
	inpspec.addString( uiStrings::sRandomLine() );
    geomsel_ = new uiGenInput( geomgrp_, tr("Geometry for line"), inpspec );
    geomsel_->valuechanged.notify( mCB(this,uiStratSynthExport,geomSel) );
    geomgrp_->setHAlignObj( geomsel_ );

    BinID startbid( SI().inlRange(OD::UsrWork).snappedCenter(),
		    SI().crlRange(OD::UsrWork).start );
    Coord startcoord = SI().transform( startbid );
    BinID stopbid( SI().inlRange(OD::UsrWork).snappedCenter(),
		   SI().crlRange(OD::UsrWork).stop );
    Coord stopcoord = SI().transform( stopbid );
    coord0fld_ = new uiGenInput( geomgrp_, tr("Coordinates: from"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord0fld_->attach( alignedBelow, geomsel_ );
    coord0fld_->setValue( mNINT64(startcoord.x_), 0 );
    coord0fld_->setValue( mNINT64(startcoord.y_), 1 );
    coord1fld_ = new uiGenInput( geomgrp_, tr("to"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord1fld_->attach( alignedBelow, coord0fld_ );
    coord1fld_->setValue( mNINT64(stopcoord.x_), 0 );
    coord1fld_->setValue( mNINT64(stopcoord.y_), 1 );

    picksetsel_ = new uiPickSetIOObjSel( geomgrp_, true,
					 uiPickSetIOObjSel::PolygonOnly );
    picksetsel_->attach( alignedBelow, geomsel_ );
    if ( haverl )
    {
	randlinesel_ = new uiIOObjSel( geomgrp_, mIOObjContext(RandomLineSet) );
	randlinesel_->attach( alignedBelow, geomsel_ );
    }
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    newlinenmfld_->display( iscreate );
    existlinenmsel_->display( !iscreate );
    geomgrp_->display( iscreate );
    if ( iscreate )
	geomSel( 0 );
}


void uiStratSynthExport::geomSel( CallBacker* )
{
    const int selgeom = crnewfld_->getBoolValue() ? geomsel_->getIntValue(): -1;
    coord0fld_->display( selgeom == 0 );
    coord1fld_->display( selgeom == 0 );
    picksetsel_->display( selgeom == 1 );
    if ( randlinesel_ )
	randlinesel_->display( selgeom == 2 );
}


void uiStratSynthExport::create2DGeometry( const TypeSet<Coord>& ptlist,
					   Line2DData& geom )
{
    geom.setEmpty();
    const int nrtrcs = datamgr_.nrTraces();

    int trcnr = 0;
    for ( int idx=0; idx<ptlist.size()-1; idx++ )
    {
	Coord startpos = ptlist[idx];
	Coord stoppos = ptlist[idx+1];
	const float dist = startpos.distTo<float>( stoppos );
	const double unitdist = mMAX( SI().inlStep() * SI().inlDistance(),
				      SI().crlStep() * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x_ - startpos.x_ ) / nrsegs;
	const double unity = ( stoppos.y_ - startpos.y_ ) / nrsegs;
	for ( int nidx=0; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x_ + nidx * unitx;
	    const double cury = startpos.y_ + nidx * unity;
	    Coord curpos( curx, cury );
	    trcnr++;
	    PosInfo::Line2DPos pos( trcnr );
	    pos.coord_ = curpos;
	    geom.add( pos );
	    if ( nrtrcs <= trcnr )
		return;
	}

	trcnr++;
	PosInfo::Line2DPos stop2dpos( trcnr );
	stop2dpos.coord_ = stoppos;
	geom.add( stop2dpos );
	if ( nrtrcs <= trcnr )
	    return;
    }
}


uiStratSynthExport::GeomSel uiStratSynthExport::selType() const
{
    return crnewfld_->getBoolValue()
	? (uiStratSynthExport::GeomSel)geomsel_->getIntValue()
	: uiStratSynthExport::Existing;
}


Pos::GeomID uiStratSynthExport::getGeometry( Line2DData& linegeom )
{
    GeomSel selgeom = selType();
    TypeSet<Coord> ptlist;
    switch ( selgeom )
    {
	case Existing:
	{
	    const auto& geom2d = SurvGeom::get2D( linegeom.lineName() );
	    if ( geom2d.isEmpty() )
		mErrRet(uiStrings::phrCannotFind(
			    tr("the geometry of specified line")), mUdfGeomID )
	    linegeom = geom2d.data();
	    return geom2d.geomID();
	}
	case StraightLine:
	{
	    ptlist += Coord(coord0fld_->getDValue(0), coord0fld_->getDValue(1));
	    ptlist += Coord(coord1fld_->getDValue(0), coord1fld_->getDValue(1));
	    break;
	}
	case Polygon:
	{
	    ConstRefMan<Pick::Set> ps = picksetsel_->getPickSet();
	    if ( !ps )
		return mUdfGeomID;
	    Pick::SetIter psiter( *ps );
	    while ( psiter.next() )
		ptlist += psiter.get().pos().getXY();
	    break;
	}
	case RandomLine:
	{
	    const IOObj* randlineobj = randlinesel_->ioobj();
	    if ( !randlineobj )
		mErrRet( tr("No random line selected"), mUdfGeomID )
	    Geometry::RandomLineSet lset;
	    uiString errmsg;
	    if ( !RandomLineSetTranslator::retrieve(lset,randlineobj,errmsg) )
		mErrRet( errmsg, mUdfGeomID )
	    const ObjectSet<Geometry::RandomLine>& lines = lset.lines();
	    BufferStringSet linenames;
	    for ( int idx=0; idx<lines.size(); idx++ )
		linenames.add( lines[idx]->name() );
	    int selitem = 0;
	    if ( linenames.isEmpty() )
		mErrRet( tr("Random line appears to be empty"), mUdfGeomID )
	    else if ( linenames.size()>1 )
	    {
		uiSelectFromList seldlg( this,
			uiSelectFromList::Setup(tr("Random lines"),linenames) );
		selitem = seldlg.selection();
	    }

	    const Geometry::RandomLine& rdmline = *lset.lines()[ selitem ];
	    for ( int nidx=0; nidx<rdmline.nrNodes(); nidx++ )
		ptlist += SI().transform( rdmline.nodePosition(nidx) );
	    break;
	}
    }

    const auto newgeomid = Geom2DImpHandler::getGeomID( linegeom.lineName() );
    if ( !mIsUdfGeomID(newgeomid) )
	create2DGeometry( ptlist, linegeom );

    return newgeomid;
}


bool uiStratSynthExport::createHor2Ds( const char* prefix, const char* postfix )
{
    EM::ObjectManager& mgr = EM::Hor2DMan();
    const bool createnew = crnewfld_->getBoolValue();
    if ( createnew && selids_.isEmpty() )
	mErrRet(tr("Cannot create horizon without a geometry. Select any "
		   "synthetic data to create a new geometry or use existing "
		   "2D line"), false);
    const char* linenm = createnew ? newlinenmfld_->text()
				   : existlinenmsel_->getInput();
    const Pos::GeomID geomid = SurvGeom::getGeomID( linenm );
    if ( !geomid.isValid() )
	return false;

    const auto& geom2d = SurvGeom::get2D( geomid );
    const auto trcnrrg = geom2d.data().trcNrRange();
    for ( int horidx=0; horidx<sellvls_.size(); horidx++ )
    {
	const Strat::Level stratlvl
		= Strat::LVLS().getByName( sellvls_.get(horidx) );
	if ( stratlvl.id().isInvalid() )
	    continue;

	BufferString hornm( stratlvl.name() );
	if ( prefix && *prefix )
	    hornm.set( prefix ).add( "_" ).add( stratlvl.name() );
	else
	    hornm.set( stratlvl.name() );
	if ( postfix && *postfix )
	    hornm.add( "_" ).add( postfix );

	EM::Object* emobj = mgr.createObject(EM::Horizon2D::typeStr(),hornm);
	mDynamicCastGet(EM::Horizon2D*,horizon2d,emobj);
	if ( !horizon2d )
	    continue;

	horizon2d->geometry().addLine( geomid );
	StratSynth::DataMgr::ZValueSet zvals;
	datamgr_.getLevelDepths( stratlvl.id(), zvals );
	for ( int trcidx=0; trcidx<zvals.size(); trcidx++ )
	{
	    const int trcnr = trcnrrg.atIndex( trcidx );
	    horizon2d->setZPos( geomid, trcnr, zvals[trcidx], false );
	}

	PtrMan<Executor> saver = horizon2d->saver();
	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute(&taskrunner,*saver) )
	    mErrRet( saver->message(), false );
    }

    return false;
}

void uiStratSynthExport::getSelections()
{
    selids_.setEmpty();
    DataMgr::SynthIDSet dsids;
    if ( poststcksel_->isChecked() )
    {
	datamgr_.getIDs( dsids, DataMgr::NoPS );
	for ( auto idx : poststcksel_->selidxs_ )
	    selids_.add( dsids[idx] );
    }
    if ( prestcksel_->isChecked() )
    {
	datamgr_.getIDs( dsids, DataMgr::OnlyPS );
	for ( auto idx : prestcksel_->selidxs_ )
	    selids_.add( dsids[idx] );
    }

    sellvls_.erase();
    if ( horsel_->isChecked() )
	sellvls_ = horsel_->selnms_;
}


bool uiStratSynthExport::acceptOK()
{
    getSelections();

    if ( selids_.isEmpty() && sellvls_.isEmpty() )
	{ mErrRet( tr("Nothing selected for export"), false ); }

    const bool useexisting = selType() == Existing;
    bool havepoststack = false;
    for ( auto id : selids_ )
	if ( !datamgr_.isPS(id) )
	    { havepoststack = true; break; }

    if ( !useexisting && !havepoststack )
    {
	uiString msg = tr("No post stack data selected");
	msg.appendPhrase( tr("Since a new geometry will be created you need "
			     "to select at least one post stack dataset") );
	mErrRet( msg, false );
    }

    BufferString linenm =
	crnewfld_->getBoolValue() ? newlinenmfld_->text()
				  : existlinenmsel_->getInput();
    if ( linenm.isEmpty() )
	{ mErrRet( tr("No line name specified"), false ); }

    PtrMan<Line2DData> linegeom = new Line2DData( linenm );
    Pos::GeomID newgeomid = getGeometry( *linegeom );
    if ( mIsUdfGeomID(newgeomid) )
	return false;

    if ( linegeom->positions().size() < datamgr_.nrTraces() )
	uiMSG().warning(tr("The geometry of the line could not accomodate \n"
			   "all the traces from the synthetics. Some of the \n"
			   "end traces will be clipped"));

    StratSynthExporter::Setup expsu( newgeomid );
    expsu.prefix_ = prefxfld_->text(); expsu.prefix_.trimBlanks();
    expsu.postfix_ = postfxfld_->text(); expsu.postfix_.trimBlanks();
    expsu.replaceudfs_ = repludfsfld_->isChecked();

    ObjectSet<const SynthSeis::DataSet> sds;
    uiTaskRunnerProvider trprov( this );
    for ( auto id : selids_ )
    {
	if ( !datamgr_.ensureGenerated(id,trprov) )
	    return false;
	sds += datamgr_.getDataSet( id );
    }
    if ( !sds.isEmpty() )
    {
	StratSynthExporter synthexp( expsu, sds, *linegeom );
	uiTaskRunner taskrunner( this );
	if ( !taskrunner.execute(synthexp) )
	    return false;
    }

    createHor2Ds( expsu.prefix_, expsu.postfix_ );

    if ( !SI().has2D() )
	uiMSG().warning(tr("You need to change survey type to 'Both 2D and 3D'"
		   " in survey setup to be able to work with the 2D line"));

    return true;
}
