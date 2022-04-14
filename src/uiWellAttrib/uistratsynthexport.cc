/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/

#include "uistratsynthexport.h"

#include "ui2dgeomman.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "prestackgather.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "stratlevel.h"
#include "stratsynthexp.h"
#include "syntheticdataimpl.h"
#include "velocitycalc.h"
#include "zdomain.h"
#include "od_helpids.h"


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
    mAttachCB( butPush, uiStratSynthOutSel::selItems );
}

~uiStratSynthOutSel()
{
    detachAllNotifiers();
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(nm_), mNoDlgTitle,
			mODHelpKey(mStartSynthOutSelHelpID) );
    uiDialog dlg( parent(), su );
    auto* lb = new uiListBox( &dlg, nm_.getFullString(), OD::ChooseAtLeastOne );
    lb->addItems( nms_ );

    if ( itmsarelevels_ )
    {
	const Strat::LevelSet& lvls = Strat::LVLS();
	for ( int idx=0; idx<nms_.size(); idx++ )
	{
	    const Strat::Level::ID id = lvls.getIDByName( nms_.get(idx).buf() );
	    lb->setColor( idx, lvls.colorOf(id) );
	}
    }


    for ( const auto& idx : selidxs_ )
	lb->setChosen( idx, true );

    if ( dlg.go() )
    {
	selidxs_.setEmpty();
	selnms_.setEmpty();
	for ( int idx=0; idx<lb->size(); idx++ )
	{
	    if ( lb->isChosen(idx) )
	    {
		selidxs_ += idx;
		selnms_.add( lb->textOfItem(idx) );
	    }
	}
    }
}

virtual BufferString getSummary() const
{
    BufferString ret;
    const int sz = nms_.size();
    const int selsz = selidxs_.size();

    if ( sz < 1 )
	ret = "<None available>";
    else if ( selsz == 0 )
	ret = "<None selected>";
    else
    {
	if ( selsz > 1 )
	{
	    ret.add( "<" ).add( selsz ).add( " selected" );
	    if ( sz == selsz )
		ret.add( " (all)" );
	    ret.add( ">: " );
	}
	ret.add( nms_.get( selidxs_.first() ) );
	if ( selsz > 1 )
	    ret.add( ", ..." );
	else if ( sz == selsz )
	    ret.add( " (all)" );
    }

    return ret;
}

    const uiString		nm_;
    const BufferStringSet	nms_;
    BufferStringSet		selnms_;
    TypeSet<int>		selidxs_;
    bool			itmsarelevels_ = false;

    uiListBox*			listfld_;

};



uiStratSynthExport::uiStratSynthExport( uiParent* p,
					const StratSynth::DataMgr& dm )
    : uiDialog(p,uiDialog::Setup(tr("Save synthetic seismics and horizons"),
				 mNoDlgTitle,
				 mODHelpKey(mStratSynthExportHelpID) ) )
    , datamgr_(dm.getProdMgr())
{
    crnewfld_ = new uiGenInput( this, tr("2D Line"),
			     BoolInpSpec(true,uiStrings::phrCreate(
			     uiStrings::sNew()), tr("Use existing")) );
    mAttachCB( crnewfld_->valuechanged, uiStratSynthExport::crNewChg );

    newlinenmfld_ = new uiGenInput( this, mJoinUiStrs(sNew(),sLineName()),
				    StringInpSpec() );
    newlinenmfld_->attach( alignedBelow, crnewfld_ );
    existlinenmsel_ = new uiSeis2DLineNameSel( this, true );
    existlinenmsel_->fillWithAll();
    existlinenmsel_->attach( alignedBelow, crnewfld_ );

    auto* sep = new uiSeparator( this, "Hsep 1" );
    sep->attach( stretchedBelow, existlinenmsel_ );

    geomgrp_ = new uiGroup( this, "Geometry group" );
    fillGeomGroup();
    geomgrp_->attach( alignedBelow, existlinenmsel_ );
    geomgrp_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hsep 2" );
    sep->attach( stretchedBelow, geomgrp_ );

    auto* selgrp = new uiGroup( this, "Export sel group" );
    selgrp->attach( ensureBelow, sep );

    BufferStringSet postnms, prenms;
    datamgr_->getNames( postnms, StratSynth::DataMgr::NoPS );
    datamgr_->getNames( prenms, StratSynth::DataMgr::OnlyPS );

    if ( !postnms.isEmpty() )
    {
	poststcksel_ = new uiStratSynthOutSel( selgrp,
					tr("Post-stack line data"), postnms );
	repludfsfld_ = new uiCheckBox( selgrp, tr("Fill undefs") );
	repludfsfld_->setChecked( true );
	repludfsfld_->attach( rightOf, poststcksel_ );
    }

    BufferStringSet nms;
    Strat::LVLS().getNames( nms );
    horsel_ = new uiStratSynthOutSel( selgrp, mJoinUiStrs(s2D(),
					    sHorizon(mPlural).toLower()), nms );
    if ( poststcksel_ )
	horsel_->attach( alignedBelow, poststcksel_ );
    horsel_->itmsarelevels_ = true;

    if ( !prenms.isEmpty() )
    {
	prestcksel_ = new uiStratSynthOutSel( selgrp, mJoinUiStrs(sPreStack(),
						  sData().toLower()), prenms );
	prestcksel_->attach( alignedBelow, horsel_ );
    }

    selgrp->setHAlignObj( poststcksel_ ? poststcksel_ : horsel_ );
    selgrp->attach( alignedBelow, geomgrp_ );

    auto* fixgrp = new uiGroup( this, "Pre/Postfix group" );

    auto* lbl1 = new uiLabel( fixgrp,
		tr("Output object names will be generated.") );
    auto* lbl2 = new uiLabel( fixgrp,
	    tr("You can specify an optional prefix and postfix for each:"));
    lbl2->attach( centeredBelow, lbl1 );
    auto* fixfldgrp = new uiGroup( fixgrp, "Pre/Postfix fields group" );
    prefxfld_ = new uiGenInput( fixfldgrp, tr("Prefix") );
    prefxfld_->setElemSzPol( uiObject::Small );
    postfxfld_ = new uiGenInput( fixfldgrp, tr("Postfix") );
    postfxfld_->setElemSzPol( uiObject::Small );
    postfxfld_->attach( rightOf, prefxfld_ );
    fixfldgrp->attach( centeredBelow, lbl2 );
    fixgrp->attach( centeredBelow, selgrp );
    fixgrp->attach( ensureBelow, sep );

    mAttachCB( postFinalize(), uiStratSynthExport::crNewChg );
}


uiStratSynthExport::~uiStratSynthExport()
{
    detachAllNotifiers();
}


void uiStratSynthExport::fillGeomGroup()
{
    StringListInpSpec inpspec;
    inpspec.addString(tr("Straight line"));
    inpspec.addString(uiStrings::sPolygon());
    const bool haverl = SI().has3D();
    if ( haverl )
	inpspec.addString( uiStrings::sRandomLine() );
    geomsel_ = new uiGenInput( geomgrp_, tr("Geometry for line"), inpspec );
    mAttachCB( geomsel_->valuechanged, uiStratSynthExport::geomSel );
    geomgrp_->setHAlignObj( geomsel_ );

    Coord startcoord, stopcoord;
    getCornerPoints( startcoord, stopcoord );
    coord0fld_ = new uiGenInput( geomgrp_, tr("Coordinates: from"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord0fld_->attach( alignedBelow, geomsel_ );
    coord0fld_->setValue( startcoord.x, 0 );
    coord0fld_->setValue( startcoord.y, 1 );
    coord1fld_ = new uiGenInput( geomgrp_, tr("to"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord1fld_->attach( alignedBelow, coord0fld_ );
    coord1fld_->setValue( stopcoord.x, 0 );
    coord1fld_->setValue( stopcoord.y, 1 );

    IOObjContext psctxt( mIOObjContext(PickSet) );
    psctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    picksetsel_ = new uiIOObjSel( geomgrp_, psctxt, uiStrings::sPolygon() );
    picksetsel_->attach( alignedBelow, geomsel_ );
    if ( haverl )
    {
	randlinesel_ = new uiIOObjSel( geomgrp_, mIOObjContext(RandomLineSet) );
	randlinesel_->attach( alignedBelow, geomsel_ );
    }
}


void uiStratSynthExport::getSelections()
{
    selids_.setEmpty();
    if ( poststcksel_ && poststcksel_->isChecked() )
    {
	TypeSet<StratSynth::DataMgr::SynthID> dsids;
	datamgr_->getIDs( dsids, StratSynth::DataMgr::NoPS );
	for ( const auto& idx : poststcksel_->selidxs_ )
	    selids_.add( dsids[idx] );
    }

    if ( prestcksel_ && prestcksel_->isChecked() )
    {
	TypeSet<StratSynth::DataMgr::SynthID> dsids;
	datamgr_->getIDs( dsids, StratSynth::DataMgr::OnlyPS );
	for ( const auto& idx : prestcksel_->selidxs_ )
	    selids_.add( dsids[idx] );
    }

    sellvls_.setEmpty();
    if ( horsel_->isChecked() )
	sellvls_ = horsel_->selnms_;
}


void uiStratSynthExport::getLevels( ObjectSet<StratSynth::Level>& sslvls ) const
{
    TypeSet<StratSynth::DataMgr::SynthID> selids;
    datamgr_->getIDs( selids, StratSynth::DataMgr::NoSubSel, true );
    if ( selids.isEmpty() )
	return;

    ConstRefMan<SyntheticData> sd = datamgr_->getDataSet( selids.first() );
    if ( !sd )
	return;

    for ( const auto* lvlnm : sellvls_ )
    {
	if ( !datamgr_->levels().isPresent(lvlnm->buf()) )
	    continue;

	const StratSynth::Level& depthlvl =
			  datamgr_->levels().getByName( lvlnm->buf() );
	auto* ssl = new StratSynth::Level( depthlvl );
	for ( int trcidx=0; trcidx<ssl->zvals_.size(); trcidx++ )
	{
	    const TimeDepthModel* tdmodel = sd->getTDModel( trcidx );
	    ssl->zvals_[trcidx] = tdmodel
			? tdmodel->getTime( ssl->zvals_[trcidx] )
			: mUdf(float);
	}
	sslvls.add( ssl );
    }
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    newlinenmfld_->display( iscreate );
    existlinenmsel_->display( !iscreate );
    geomgrp_->display( iscreate );
    if ( iscreate )
	geomSel( nullptr );
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


void uiStratSynthExport::getCornerPoints( Coord& start, Coord& stop )
{
    const TrcKey tkstart = TrcKey::getSynth( 1 );
    const TrcKey tkstop = TrcKey::getSynth( datamgr_->nrSequences() );
    start = tkstart.getCoord();
    stop = tkstop.getCoord();
}


bool uiStratSynthExport::createAndWrite2DGeometry( const TypeSet<Coord>& ptlist,
						   Pos::GeomID geomid ) const
{
    Survey::Geometry* geom = Survey::GMAdmin().getGeometry( geomid );
    Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return false;

    RefMan<Survey::Geometry2D> newgeom = geom2d;
    const int nrtrcs = datamgr_->nrTraces();

    int trcnr = 0;
    for ( int idx=0; idx<ptlist.size()-1; idx++ )
    {
	const Coord startpos = ptlist[idx];
	const Coord stoppos = ptlist[idx+1];
	const double dist = startpos.distTo( stoppos );
	const double unitdist = mMAX( SI().inlStep() * SI().inlDistance(),
				      SI().crlStep() * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x - startpos.x ) / nrsegs;
	const double unity = ( stoppos.y - startpos.y ) / nrsegs;
	for ( int nidx=0; nidx<nrsegs; nidx++ )
	{
	    const Coord curpos( startpos.x + nidx * unitx,
				startpos.y + nidx * unity );
	    trcnr++;
	    newgeom->add( curpos, trcnr, -1.f );
	    if ( trcnr >= nrtrcs )
		break;
	}

	trcnr++;
	newgeom->add( stoppos, trcnr, -1.f );
	if ( trcnr >= nrtrcs )
	    break;
    }

    ZSampling zrg = SI().zRange();
    TypeSet<StratSynth::DataMgr::SynthID> selids;
    datamgr_->getIDs( selids, StratSynth::DataMgr::NoSubSel, true );
    if ( !selids.isEmpty() )
    {
	ConstRefMan<SyntheticData> sd = datamgr_->getDataSet( selids.first() );
	const SeisTrc* trc = sd ? sd->getTrace(0) : nullptr;
	if ( trc )
	    zrg = trc->zRange();
    }

    newgeom->dataAdmin().setZRange( zrg );
    newgeom->touch();

    uiString errmsg;
    const bool res = Survey::GMAdmin().write( *newgeom.ptr(), errmsg );
    if ( !res )
	uiMSG().error( errmsg );

    return res;
}


uiStratSynthExport::GeomSel uiStratSynthExport::selType() const
{
    return crnewfld_->getBoolValue()
	? (uiStratSynthExport::GeomSel)geomsel_->getIntValue()
	: uiStratSynthExport::Existing;
}


bool uiStratSynthExport::getGeometry( const char* linenm )
{
    uiStratSynthExport::GeomSel selgeom = selType();
    TypeSet<Coord> ptlist;
    switch ( selgeom )
    {
	case Existing:
	{
	    const Survey::Geometry* geom = Survey::GM().getGeometry( linenm );
	    if ( !geom || !geom->is2D() )
		mErrRet(tr("Could not find the geometry of specified line"),
			false)
	    return true;
	}
	case StraightLine:
	{
	    ptlist += Coord(coord0fld_->getDValue(0), coord0fld_->getDValue(1));
	    ptlist += Coord(coord1fld_->getDValue(0), coord1fld_->getDValue(1));
	    break;
	}
	case Polygon:
	{
	    const IOObj* picksetobj = picksetsel_->ioobj();
	    if ( !picksetobj )
		mErrRet( tr("No pickset selected"), false )
	    RefMan<Pick::Set> pickset;
	    if ( Pick::Mgr().indexOf(picksetobj->key())>0 )
		pickset = Pick::Mgr().get( picksetobj->key() );
	    else
	    {
		BufferString errmsg;
		if ( !PickSetTranslator::retrieve(
			    *pickset,IOM().get(picksetobj->key()),true,errmsg) )
		    mErrRet( mToUiStringTodo(errmsg), false )
	    }

	    for ( int idx=0; idx<pickset->size(); idx++ )
		ptlist += pickset->get(idx).pos();

	    break;
	}
	case RandomLine:
	{
	    const IOObj* randlineobj = randlinesel_->ioobj();
	    if ( !randlineobj )
		mErrRet( tr("No random line selected"), false )
	    Geometry::RandomLineSet lset;
	    BufferString errmsg;
	    if ( !RandomLineSetTranslator::retrieve(lset,randlineobj,errmsg) )
		mErrRet( mToUiStringTodo(errmsg), false )
	    const ObjectSet<Geometry::RandomLine>& lines = lset.lines();
	    BufferStringSet linenames;
	    for ( int idx=0; idx<lines.size(); idx++ )
		linenames.add( lines[idx]->name() );
	    int selitem = 0;
	    if ( linenames.isEmpty() )
		mErrRet( tr("Random line appears to be empty"), false )
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

    const Pos::GeomID newgeomid = Geom2DImpHandler::getGeomID( linenm );
    if ( !Survey::is2DGeom(newgeomid) )
	return false;

    return createAndWrite2DGeometry( ptlist, newgeomid );
}


void uiStratSynthExport::addPrePostFix( BufferString& oldnm ) const
{
    BufferString newnm;

    BufferString pfx( prefxfld_->text() );
    if ( !pfx.isEmpty() )
	newnm.add( pfx ).add( "_" );

    newnm += oldnm.buf();
    pfx = postfxfld_->text();
    if ( !pfx.isEmpty() )
	newnm.add( "_" ).add( pfx );

    oldnm = newnm;
}


bool uiStratSynthExport::createHor2Ds()
{
    EM::EMManager& em = EM::EMM();
    const bool createnew = crnewfld_->getBoolValue();
    const BufferString linenm( createnew ? newlinenmfld_->text()
					 : existlinenmsel_->getInput() );
    const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
    if ( !Survey::is2DGeom(geomid) )
	return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d || geom2d->isEmpty() )
	return false;

    ManagedObjectSet<StratSynth::Level> sslvls;
    getLevels( sslvls );
    if ( sslvls.size() != sellvls_.size() )
	return false;

    const StepInterval<Pos::TraceID> trcnrrg = geom2d->data().trcNrRange();
    for ( const auto* stratlvl : sslvls )
    {
	BufferString hornm( stratlvl->name() );
	addPrePostFix( hornm );
	EM::ObjectID emid = em.createObject( EM::Horizon2D::typeStr(),hornm );
	mDynamicCastGet(EM::Horizon2D*,horizon2d,em.getObject(emid));
	if ( !horizon2d )
	    continue;

	horizon2d->geometry().addLine( geomid );
	horizon2d->setPreferredColor( stratlvl->color() );
	horizon2d->setStratLevelID( stratlvl->id() );
	for ( int trcidx=0; trcidx<stratlvl->zvals_.size(); trcidx++ )
	{
	    const int trcnr = trcnrrg.atIndex( trcidx );
	    horizon2d->setPos( horizon2d->sectionID(0), geomid, trcnr,
			       stratlvl->zvals_[trcidx], false );
	}

	PtrMan<Executor> saver = horizon2d->saver();
	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute(&taskrunner,*saver) )
	    mErrRet( saver->uiMessage(), false );
    }

    return false;
}


bool uiStratSynthExport::acceptOK( CallBacker* )
{
    getSelections();

    if ( selids_.isEmpty() && sellvls_.isEmpty() )
	mErrRet( tr("Nothing selected for export"), false );

    uiTaskRunner trprov( this );
    const bool useexisting = selType()==uiStratSynthExport::Existing;
    if ( selids_.isEmpty() )
    {
	if ( !useexisting || !sellvls_.isEmpty() )
	{ //Create at least one to get valid time-depth models
	    TypeSet<StratSynth::DataMgr::SynthID> ids;
	    datamgr_->getIDs( ids, StratSynth::DataMgr::NoPS );
	    if ( ids.isEmpty() )
		datamgr_->getIDs( ids, StratSynth::DataMgr::OnlyPS );

	    if ( ids.isEmpty() )
		mErrRet( tr("Nothing available for export"), false );

	    for ( const auto& selid : ids )
	    {
		if ( datamgr_->ensureGenerated(selid,&trprov) )
		    break;
	    }
	}
    }
    else
    {
	for ( const auto& selid : selids_ )
	    datamgr_->ensureGenerated( selid, &trprov );
    }

    const BufferString linenm =
		    crnewfld_->getBoolValue() ? newlinenmfld_->text()
					      : existlinenmsel_->getInput();
    if ( linenm.isEmpty() )
	mErrRet( tr("No line name specified"), false );

    if ( !getGeometry(linenm) )
	return false;

    const Pos::GeomID gid = Survey::GM().getGeomID( linenm );
    const Survey::Geometry2D& geom2d = Survey::GM().get2D( gid );
    const int nrgeompos = geom2d.data().positions().size();

    if ( nrgeompos < datamgr_->nrTraces() )
    {
	uiMSG().warning(tr("The geometry of the line could not accommodate \n"
			   "all the traces from the synthetics. Some of the \n"
			   "end traces will be clipped"));
    }

    SeparString prepostfix;
    prepostfix.add( prefxfld_->text() );
    prepostfix.add( postfxfld_->text() );
    RefObjectSet<const SyntheticData> sds;
    for ( const auto& id : selids_ )
    {
	ConstRefMan<SyntheticData> sd = datamgr_->getDataSet( id );
	sds.add( sd.ptr() );
    }

    if ( !sds.isEmpty() )
    {
	StratSynth::Exporter synthexp( sds, gid, prepostfix,
				    repludfsfld_ && repludfsfld_->isChecked() );
	const bool res = TaskRunner::execute( &trprov, synthexp );
	if ( !res )
	    return false;
    }

    if ( !sellvls_.isEmpty() )
	createHor2Ds();

    if ( !SI().has2D() )
	uiMSG().warning(tr("You need to change survey type to 'Both 2D and 3D'"
			   " in survey setup to display the 2D line"));
    return true;
}
