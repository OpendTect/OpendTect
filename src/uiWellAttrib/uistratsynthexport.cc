/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthexport.h"

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
#include "survinfo.h"
#include "surv2dgeom.h"
#include "stratsynth.h"
#include "stratsynthexp.h"
#include "stratsynthlevel.h"
#include "stratlevel.h"
#include "syntheticdataimpl.h"
#include "velocitycalc.h"
#include "zdomain.h"


#define mErrRet( msg, rettyp ) \
{ \
    uiMSG().error( msg ); \
    return rettyp;\
}


class uiStratSynthOutSel : public uiCheckedCompoundParSel
{
public:

uiStratSynthOutSel( uiParent* p, const char* seltxt, const BufferStringSet& nms )
    : uiCheckedCompoundParSel( p, seltxt, false, "&Select" )
    , nms_(nms)
    , nm_(seltxt)
{
    butPush.notify( mCB(this,uiStratSynthOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( BufferString("Select ",nm_), mNoDlgTitle, "110.0.9" );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, nm_ );
    lb->addItems( nms_ );
    lb->setItemsCheckable( true );
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setItemChecked( selidxs_[idx], true );
    if ( dlg.go() )
    {
	selidxs_.erase();
	for ( int idx=0; idx<lb->size(); idx++ )
	    if ( lb->isItemChecked(idx) ) selidxs_ += idx;
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

    const BufferString	nm_;
    const BufferStringSet nms_;
    TypeSet<int>	selidxs_;

    uiListBox*		listfld_;

};



uiStratSynthExport::uiStratSynthExport( uiParent* p, const StratSynth& ss )
    : uiDialog(p,uiDialog::Setup("Save synthetic seismics and horizons",
				 getWinTitle(ss), "110.0.8") )
    , ss_(ss)
    , randlinesel_(0)
{
    crnewfld_ = new uiGenInput( this, "2D Line",
			     BoolInpSpec(true,"Create New","Use existing") );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );


    uiSeisSel::Setup sssu( Seis::Line );
    sssu.enabotherdomain( false ).zdomkey( ZDomain::sKeyTime() )
	.selattr( false ).allowsetdefault( false );
    linesetsel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
	    			sssu );
    linesetsel_->attach( alignedBelow, crnewfld_ );
    linesetsel_->selectionDone.notify( mCB(this,uiStratSynthExport,lsSel) );
    newlinenmsel_ = new uiSeis2DLineNameSel( this, false );
    newlinenmsel_->attach( alignedBelow, linesetsel_ );
    existlinenmsel_ = new uiSeis2DLineNameSel( this, true );
    existlinenmsel_->attach( alignedBelow, linesetsel_ );

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
    poststcksel_ = new uiStratSynthOutSel( selgrp, "Post-stack line data",nms );
    nms.erase(); addNames( sslvls_, nms );
    horsel_ = new uiStratSynthOutSel( selgrp, "2D horizons", nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    nms.erase(); addNames( presds_, nms );
    prestcksel_ = new uiStratSynthOutSel( selgrp, "Pre-stack data", nms );
    prestcksel_->attach( alignedBelow, horsel_ );
    selgrp->setHAlignObj( poststcksel_ );
    selgrp->attach( alignedBelow, geomgrp_ );

    uiLabel* lbl = new uiLabel( this, "Output object names will be generated."
		"\nYou can specify an optional prefix and postfix for each:" );
    lbl->attach( ensureBelow, selgrp );
    lbl->setAlignment( Alignment::Left );
    prefxfld_ = new uiGenInput( this, "Prefix" );
    prefxfld_->attach( alignedBelow, selgrp );
    prefxfld_->attach( ensureBelow, lbl );
    postfxfld_ = new uiGenInput( this, "Postfix" );
    postfxfld_->attach( rightOf, prefxfld_ );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
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
    inpspec.addString( "Straight line" ); inpspec.addString( "Polygon" );
    const bool haverl = SI().has3D();
    if ( haverl )
	inpspec.addString( "Random Line" );
    geomsel_ = new uiGenInput( geomgrp_, "Geometry for line", inpspec );
    geomsel_->valuechanged.notify( mCB(this,uiStratSynthExport,geomSel) );
    geomgrp_->setHAlignObj( geomsel_ );

    BinID startbid( SI().inlRange(true).snappedCenter(),
	    	    SI().crlRange(true).start );
    Coord startcoord = SI().transform( startbid );
    BinID stopbid( SI().inlRange(true).snappedCenter(),
	    	   SI().crlRange(true).stop );
    Coord stopcoord = SI().transform( stopbid );
    coord0fld_ = new uiGenInput( geomgrp_, "Coordinates: from",
					DoubleInpSpec(), DoubleInpSpec() );
    coord0fld_->attach( alignedBelow, geomsel_ );
    coord0fld_->setValue( startcoord.x, 0 );
    coord0fld_->setValue( startcoord.y, 1 );
    coord1fld_ = new uiGenInput( geomgrp_, "to",
					DoubleInpSpec(), DoubleInpSpec() );
    coord1fld_->attach( alignedBelow, coord0fld_ );
    coord1fld_->setValue( stopcoord.x, 0 );
    coord1fld_->setValue( stopcoord.y, 1 );

    IOObjContext psctxt( mIOObjContext(PickSet) );
    psctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );
    picksetsel_ = new uiIOObjSel( geomgrp_, psctxt, "Polygon" );
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

    const SyntheticData* sd = postsds_[0];
    const ObjectSet<const TimeDepthModel>& d2t = sd->d2tmodels_;
    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	const Strat::Level& lvl = lvls.getLevel( idx );
	StratSynthLevel* ssl = new StratSynthLevel( lvl.name(), lvl.color() );
	ss_.getLevelDepths( lvl, ssl->zvals_ );
	for ( int trcidx=0; trcidx<ssl->zvals_.size(); trcidx++ )
	    ssl->zvals_[trcidx] = d2t[trcidx]->getTime( ssl->zvals_[trcidx] );
	sslvls_ += ssl;
    }
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    newlinenmsel_->display( iscreate );
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


void uiStratSynthExport::lsSel( CallBacker* )
{
    existlinenmsel_->setLineSet( linesetsel_->key() );
}


void uiStratSynthExport::create2DGeometry( const TypeSet<Coord>& ptlist,
					   PosInfo::Line2DData& geom )
{
    geom.setEmpty();
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
	Coord startpos = ptlist[idx];
	Coord stoppos = ptlist[idx+1];
	const double dist = startpos.distTo( stoppos );
	const double unitdist = mMAX( SI().inlStep() * SI().inlDistance(),
				      SI().crlStep() * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x - startpos.x ) / nrsegs;
	const double unity = ( stoppos.y - startpos.y ) / nrsegs;
	for ( int nidx=0; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x + nidx * unitx;
	    const double cury = startpos.y + nidx * unity;
	    Coord curpos( curx, cury );
	    trcnr++;
	    PosInfo::Line2DPos pos( trcnr );
	    pos.coord_ = curpos;
	    geom.add( pos );
	    if ( synthmodelsz <= trcnr )
		return;
	}

	trcnr++;
	PosInfo::Line2DPos stop2dpos( trcnr );
	stop2dpos.coord_ = stoppos;
	geom.add( stop2dpos );
	if ( synthmodelsz <= trcnr )
	    return;
    }
}


uiStratSynthExport::GeomSel uiStratSynthExport::selType() const
{
    return crnewfld_->getBoolValue()
	? (uiStratSynthExport::GeomSel)geomsel_->getIntValue()
	: uiStratSynthExport::Existing;
}


bool uiStratSynthExport::getGeometry( PosInfo::Line2DData& linegeom ) 
{
    uiStratSynthExport::GeomSel selgeom = selType();
    TypeSet<Coord> ptlist;
    switch ( selgeom )
    {
	case Existing:
	    return S2DPOS().getGeometry( linegeom );
	case StraightLine:
	{
	    ptlist += Coord(coord0fld_->getdValue(0), coord0fld_->getdValue(1));
	    ptlist += Coord(coord1fld_->getdValue(0), coord1fld_->getdValue(1));
	    break;
	}
	case Polygon:
	{
	    const IOObj* picksetobj = picksetsel_->ioobj();
	    if ( !picksetobj )
		mErrRet( "No pickset selected", false )
	    Pick::Set pickset;
	    if ( Pick::Mgr().indexOf(picksetobj->key())>0 )
		pickset = Pick::Mgr().get( picksetobj->key() );
	    else
	    {
		BufferString errmsg;
		if ( !PickSetTranslator::retrieve(
			    pickset,IOM().get(picksetobj->key()),true,errmsg) )
		    mErrRet( errmsg, false )
	    }
	    for ( int idx=0; idx<pickset.size(); idx++ )
		ptlist += pickset[idx].pos_;
	    break;
	}
	case RandomLine:
	{
	    const IOObj* randlineobj = randlinesel_->ioobj();
	    if ( !randlineobj )
		mErrRet( "No random line selected", false )
	    Geometry::RandomLineSet lset;
	    BufferString errmsg;
	    if ( !RandomLineSetTranslator::retrieve(lset,randlineobj,errmsg) )
		mErrRet( errmsg, false )
	    const ObjectSet<Geometry::RandomLine>& lines = lset.lines();
	    BufferStringSet linenames;
	    for ( int idx=0; idx<lines.size(); idx++ )
		linenames.add( lines[idx]->name() );
	    int selitem = 0;
	    if ( linenames.isEmpty() )
		mErrRet( "Random line appears to be empty", false )
	    else if ( linenames.size()>1 )
	    {
		uiSelectFromList seldlg( this,
			uiSelectFromList::Setup("Random lines",linenames) );
		seldlg.selFld()->setMultiSelect( false );
		selitem = seldlg.selFld()->nextSelected(-1);
	    }

	    const Geometry::RandomLine& rdmline = *lset.lines()[ selitem ];
	    for ( int nidx=0; nidx<rdmline.nrNodes(); nidx++ )
		ptlist += SI().transform( rdmline.nodePosition(nidx) );
	    break;
	}
    }

    create2DGeometry( ptlist, linegeom );
    return true;
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
	mErrRet( "Cannot create horizon without a geometry. Select any "
		 "synthetic data to create a new geometry or use existing "
		 "2D line", false );
    const char* linenm = createnew ? newlinenmsel_->getInput()
				   : existlinenmsel_->getInput();
    PosInfo::GeomID geomid =
	S2DPOS().getGeomID( linesetsel_->getIOObj()->name(), linenm );
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
	    horizon2d->setPos( horizon2d->sectionID(0), geomid, trcidx,
		    	       stratlvl->zvals_[trcidx], false );
	}

	PtrMan<Executor> saver = horizon2d->saver();
	uiTaskRunner tr( this );
	if ( !TaskRunner::execute(&tr,*saver) )
	    mErrRet( saver->message(), false );
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
	mErrRet( "Nothing selected for export", false );
    }

    const bool useexisting = selType()==uiStratSynthExport::Existing;
    if ( !useexisting && postsds_.isEmpty() )
    {
	getExpObjs();
	mErrRet( "No post stack selected. Since a new geometry will be created "
		 "you need to select atleast one post stack data to "
		 "create a 2D line geometry.", false );
    }

    PtrMan<const IOObj> lineobj = linesetsel_->getIOObj();
    if ( !lineobj )
    {
	getExpObjs();
	return false;
    }
	
    S2DPOS().setCurLineSet( lineobj->name() );
    BufferString linenm =
	crnewfld_->getBoolValue() ? newlinenmsel_->getInput()
				  : existlinenmsel_->getInput();
    if ( linenm.isEmpty() )
    {
	getExpObjs();
	mErrRet( "No line name specified", false );
    }

    PosInfo::Line2DData linegeom( linenm );
    if ( !getGeometry(linegeom) )
    {
	getExpObjs();
	mErrRet( "Could not find the geometry of specified line", false );
    }

    int synthmodelsz = linegeom.positions().size();
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

    if ( linegeom.positions().size() < synthmodelsz )
	uiMSG().warning( "The geometry of the line could not accomodate \n"
			 "all the traces from the synthetics. Some of the \n"
			 "end traces will be clipped" );
    SeparString prepostfix;
    prepostfix.add( prefxfld_->text() );
    prepostfix.add( postfxfld_->text() );
    ObjectSet<const SyntheticData> sds( postsds_ );
    sds.append( presds_ );
    StratSynthExporter synthexp( *linesetsel_->getIOObj(), sds, 
	    			 linegeom, prepostfix );
    uiTaskRunner tr( this );
    const bool res = TaskRunner::execute( &tr, synthexp );
    if ( !res )
	mErrRet( synthexp.errMsg(), false )
    createHor2Ds();
    if ( !SI().has2D() )
	uiMSG().warning( "You need to change survey type to 'Both 2D and 3D'"
			 " in survey setup to display the 2D line" );
    return true;
}
