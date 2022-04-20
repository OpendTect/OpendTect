/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/

#include "uistratsynthexport.h"

#include "ui2dgeomman.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uiselsimple.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "ioman.h"
#include "prestackgather.h"
#include "picksettr.h"
#include "pickset.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "stratsynth.h"
#include "stratsynthexp.h"
#include "stratsynthlevel.h"
#include "stratlevel.h"
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
    butPush.notify( mCB(this,uiStratSynthOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(nm_), mNoDlgTitle,
			mODHelpKey(mStartSynthOutSelHelpID) );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, nm_.getFullString(),
				   OD::ChooseAtLeastOne );
    lb->addItems( nms_ );
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setChosen( selidxs_[idx], true );

    if ( dlg.go() )
    {
	selidxs_.erase();
	for ( int idx=0; idx<lb->size(); idx++ )
	    if ( lb->isChosen(idx) ) selidxs_ += idx;
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
	ret.add( nms_.get( selidxs_[0] ) );
	if ( selsz > 1 )
	    ret.add( ", ..." );
	else if ( sz == selsz )
	    ret.add( " (all)" );
    }

    return ret;
}

    const uiString	    nm_;
    const BufferStringSet   nms_;
    TypeSet<int>	    selidxs_;

    uiListBox*		    listfld_;

};



uiStratSynthExport::uiStratSynthExport( uiParent* p, const StratSynth& ss )
    : uiDialog(p,uiDialog::Setup(tr("Save synthetic seismics and horizons"),
				 mNoDlgTitle,
				 mODHelpKey(mStratSynthExportHelpID) ) )
    , ss_(ss)
    , randlinesel_(0)
{
    crnewfld_ = new uiGenInput( this, tr("2D Line"),
			     BoolInpSpec(true,uiStrings::phrCreate(
			     uiStrings::sNew()), tr("Use existing")) );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );


    newlinenmfld_ = new uiGenInput( this, mJoinUiStrs(sNew(),sLineName()),
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
    getExpObjs();

    BufferStringSet nms; addNames( postsds_, nms );
    poststcksel_ = new uiStratSynthOutSel( selgrp, tr("Post-stack line data")
									,nms );
    nms.erase(); addNames( sslvls_, nms );
    horsel_ = new uiStratSynthOutSel( selgrp, mJoinUiStrs(s2D(),
					    sHorizon(mPlural).toLower()), nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    nms.erase(); addNames( presds_, nms );
    prestcksel_ = new uiStratSynthOutSel( selgrp, mJoinUiStrs(sPreStack(),
						      sData().toLower()), nms );
    prestcksel_->attach( alignedBelow, horsel_ );
    selgrp->setHAlignObj( poststcksel_ );
    selgrp->attach( alignedBelow, geomgrp_ );

    uiLabel* lbl = new uiLabel( this, tr("Output object names will "
					 "be generated.\nYou can specify "
					 "an optional prefix and postfix "
					 "for each:") );
    lbl->attach( ensureBelow, selgrp );
    lbl->setAlignment( Alignment::Left );
    prefxfld_ = new uiGenInput( this, tr("Prefix") );
    prefxfld_->attach( alignedBelow, selgrp );
    prefxfld_->attach( ensureBelow, lbl );
    postfxfld_ = new uiGenInput( this, tr("Postfix") );
    postfxfld_->attach( rightOf, prefxfld_ );

    postFinalize().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
}


BufferString uiStratSynthExport::getWinTitle( const StratSynth& ss ) const
{
    BufferString ret( "" );
    return ret;
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
    geomsel_->valuechanged.notify( mCB(this,uiStratSynthExport,geomSel) );
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


void uiStratSynthExport::getExpObjs()
{
    if ( !ss_.nrSynthetics() )
	return;

    postsds_.erase(); presds_.erase(); sslvls_.erase();
    for ( int idx=0; idx<ss_.nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = ss_.getSyntheticByIdx( idx );
	(sd->isPS() ? presds_ : postsds_) += sd;
    }

    if ( postsds_.isEmpty() )
	return;

    const SyntheticData* sd = postsds_.first();
    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	const Strat::Level& lvl = lvls.getLevel( idx );
	auto* ssl = new StratSynthLevel( lvl.name(), lvl.color() );
	ss_.getLevelDepths( lvl, 1, ssl->zvals_ );
	for ( int trcidx=0; trcidx<ssl->zvals_.size(); trcidx++ )
	{
	    const TimeDepthModel* tdmodel = sd->getTDModel( trcidx );
	    ssl->zvals_[trcidx] = tdmodel
			? tdmodel->getTime( ssl->zvals_[trcidx] )
			: mUdf(float);
	}
	sslvls_ += ssl;
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


void uiStratSynthExport::getCornerPoints( Coord& start,Coord& stop )
{
    for ( int idx=0; idx<ss_.nrSynthetics(); idx++ )
    {
	const SyntheticData* sd = ss_.getSyntheticByIdx( idx );
	if ( !idx )
	    continue;

	mDynamicCastGet(const PostStackSyntheticData*,postsd,sd);
	if ( postsd )
	{
	    const SeisTrcBuf& trcbuf = postsd->postStackPack().trcBuf();
	    if ( trcbuf.size() > 1 )
	    {
		start = trcbuf.get(0)->info().coord;
		stop = trcbuf.get( trcbuf.size()-1 )->info().coord;
		return;
	    }
	}

	mDynamicCastGet(const PreStackSyntheticData*,presd,sd);
	if ( !presd )
	    continue;

	const int nrgathers = presd->preStackPack().getGathers().size();
	if ( nrgathers < 1 )
	    continue;

	start = presd->getTrace( 0 )->info().coord;
	stop = presd->getTrace( nrgathers-1 )->info().coord;
	return;
    }
}


bool uiStratSynthExport::createAndWrite2DGeometry( const TypeSet<Coord>& ptlist,
						   Pos::GeomID geomid ) const
{
    Survey::Geometry* geom = Survey::GMAdmin().getGeometry( geomid );
    Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d )
	return false;

    RefMan<Survey::Geometry2D> newgeom = geom2d;
    int synthmodelsz = mUdf(int);
    if ( postsds_.isEmpty() )
    {
	if ( !presds_.isEmpty() )
	{
	    mDynamicCastGet(const PreStackSyntheticData*,presd,presds_[0]);
	    synthmodelsz = presd->preStackPack().getGathers().size();
	}
    }
    else
    {
	mDynamicCastGet(const PostStackSyntheticData*,postsd,postsds_[0]);
	synthmodelsz = postsd->postStackPack().trcBuf().size();
    }

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
	    if ( synthmodelsz <= trcnr )
		break;
	}

	trcnr++;
	newgeom->add( stoppos, trcnr, -1.f );
	if ( synthmodelsz <= trcnr )
	    break;
    }

    const ZSampling zrg = postsds_.isEmpty()
	      ? (presds_.isEmpty() ? SI().zRange()
				   : presds_.first()->getTrace(0)->zRange())
	      : postsds_.first()->getTrace(0)->zRange();
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
    if ( createnew && (presds_.isEmpty() && postsds_.isEmpty()) )
	mErrRet(tr("Cannot create horizon without a geometry. Select any "
		   "synthetic data to create a new geometry or use existing "
		   "2D line"), false);
    const BufferString linenm( createnew ? newlinenmfld_->text()
					 : existlinenmsel_->getInput() );
    const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
    if ( !Survey::is2DGeom(geomid) )
	return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    const Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( !geom2d || geom2d->isEmpty() )
	return false;

    const StepInterval<Pos::TraceID> trcnrrg = geom2d->data().trcNrRange();
    for ( int horidx=0; horidx<sslvls_.size(); horidx++ )
    {
	const StratSynthLevel* stratlvl = sslvls_[horidx];
	BufferString hornm( stratlvl->name() );
	addPrePostFix( hornm );
	EM::ObjectID emid = em.createObject( EM::Horizon2D::typeStr(),hornm );
	mDynamicCastGet(EM::Horizon2D*,horizon2d,em.getObject(emid));
	if ( !horizon2d ) continue;
	horizon2d->geometry().addLine( geomid );
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

void uiStratSynthExport::removeNonSelected()
{
    TypeSet<int> selids;

    if ( poststcksel_->isChecked() )
    {
	selids = poststcksel_->selidxs_;
	for ( int idx=postsds_.size()-1; idx>=0; idx-- )
	{
	    if ( !selids.isPresent(idx) )
		postsds_.removeSingle( idx );
	}
    }
    else
	postsds_.erase();

    if ( prestcksel_->isChecked() )
    {
	selids = prestcksel_->selidxs_;
	for ( int idx=presds_.size()-1; idx>=0; idx-- )
	{
	    if ( !selids.isPresent(idx) )
		presds_.removeSingle( idx );
	}
    }
    else
	presds_.erase();

    if ( horsel_->isChecked() )
    {
	selids = horsel_->selidxs_;
	for ( int idx=sslvls_.size()-1; idx>=0; idx-- )
	{
	    if ( !selids.isPresent(idx) )
		sslvls_.removeSingle( idx );
	}
    }
    else
	sslvls_.erase();
}


bool uiStratSynthExport::acceptOK( CallBacker* )
{
    removeNonSelected();

    if ( presds_.isEmpty() && postsds_.isEmpty() && sslvls_.isEmpty() )
    {
	getExpObjs();
	mErrRet( tr("Nothing selected for export"), false );
    }

    const bool useexisting = selType()==uiStratSynthExport::Existing;
    if ( !useexisting && postsds_.isEmpty() )
    {
	getExpObjs();
	mErrRet(tr("No post stack selected. Since a new geometry will be "
		   "created you need to select at least one post stack data to "
		   "create a 2D line geometry."), false);
    }

    const BufferString linenm =
		    crnewfld_->getBoolValue() ? newlinenmfld_->text()
					      : existlinenmsel_->getInput();
    if ( linenm.isEmpty() )
    {
	getExpObjs();
	mErrRet( tr("No line name specified"), false );
    }

    if ( !getGeometry(linenm) )
    {
	getExpObjs();
	return false;
    }

    const Pos::GeomID gid = Survey::GM().getGeomID( linenm );
    const Survey::Geometry2D& geom2d = Survey::GM().get2D( gid );
    const int nrgeompos = geom2d.data().positions().size();

    int synthmodelsz = nrgeompos;
    if ( !postsds_.isEmpty() )
    {
	mDynamicCastGet(const PostStackSyntheticData*,postsd,postsds_[0]);
	synthmodelsz = postsd->postStackPack().trcBuf().size();
    }
    else if ( !presds_.isEmpty() )
    {
	mDynamicCastGet(const PreStackSyntheticData*,presd,presds_[0]);
	synthmodelsz = presd->preStackPack().getGathers().size();
    }

    if ( nrgeompos < synthmodelsz )
	uiMSG().warning(tr("The geometry of the line could not accommodate \n"
			   "all the traces from the synthetics. Some of the \n"
			   "end traces will be clipped"));
    SeparString prepostfix;
    prepostfix.add( prefxfld_->text() );
    prepostfix.add( postfxfld_->text() );
    ObjectSet<const SyntheticData> sds( postsds_ );
    sds.append( presds_ );
    if ( !sds.isEmpty() )
    {
	StratSynthExporter synthexp( sds, gid, prepostfix );
	uiTaskRunner taskrunner( this );
	const bool res = TaskRunner::execute( &taskrunner, synthexp );
	if ( !res )
	    return false;
    }

    createHor2Ds();
    if ( !SI().has2D() )
	uiMSG().warning(tr("You need to change survey type to 'Both 2D and 3D'"
			   " in survey setup to display the 2D line"));
    return true;
}
