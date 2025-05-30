/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiosurface.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uistratlvlsel.h"
#include "uitable.h"

#include "ctxtioobj.h"
#include "embodytr.h"
#include "emfaultstickset.h"
#include "emhorizon.h"
#include "emioobjinfo.h"
#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"
#include "survinfo.h"

#define mIdxShift2D	2
#define mIdxShift3D	4

const int cListHeight = 5;

// uiIOSurface

uiIOSurface::uiIOSurface( uiParent* p, bool forread, const char* tp,
			  const ZDomain::Info* zinfo )
    : uiGroup(p,"Surface selection")
    , attrSelChange(this)
    , forread_(forread)
{
    const StringView typ( tp );
    if ( typ == EMHorizon2DTranslatorGroup::sGroupName() )
	ctio_ = mMkCtxtIOObj(EMHorizon2D);
    else if ( typ == EMHorizon3DTranslatorGroup::sGroupName() )
	ctio_ = mMkCtxtIOObj(EMHorizon3D);
    else if ( typ == EMFaultStickSetTranslatorGroup::sGroupName() )
	ctio_ = mMkCtxtIOObj(EMFaultStickSet);
    else if ( typ == EMFault3DTranslatorGroup::sGroupName() )
	ctio_ = mMkCtxtIOObj(EMFault3D);
    else if ( typ == EMFaultSet3DTranslatorGroup::sGroupName() )
	ctio_ = mMkCtxtIOObj(EMFaultSet3D);
    else
	ctio_ = new CtxtIOObj( EMBodyTranslatorGroup::ioContext() );

    if ( zinfo )
    {
	const ZDomain::Info& siinfo = SI().zDomainInfo();
	ctio_->ctxt_.requireZDomain( *zinfo, siinfo == *zinfo );
    }

    if ( forread_ )
	mAttachCB( postFinalize(), uiIOSurface::objSel );
}


uiIOSurface::~uiIOSurface()
{
    detachAllNotifiers();
    delete ctio_->ioobj_;
    delete ctio_;
}


void uiIOSurface::mkAttribFld( bool labelabove )
{
    const BufferString trnm =
	ctio_->ctxt_.trgroup_ ? ctio_->ctxt_.trgroup_->groupName().buf() : "";
    uiString lbl = trnm == EMHorizon3DTranslatorGroup::sGroupName()
	? tr("Horizon Data") : tr("Calculated attributes");
    uiListBox::Setup su( OD::ChooseZeroOrMore, lbl,
	labelabove ? uiListBox::AboveMid : uiListBox::LeftTop );
    attribfld_ = new uiListBox( this, su, "surfacedata" );
    attribfld_->setStretch( 2, 2 );
    attribfld_->itemChosen.notify( mCB(this,uiIOSurface,attrSel) );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Available patches"),
	labelabove ? uiListBox::AboveMid : uiListBox::LeftTop );
    sectionfld_ = new uiListBox( this, su, "sections" );
    sectionfld_->setPrefHeightInChar( mCast(float,cListHeight) );
    sectionfld_->setStretch( 2, 2 );
    sectionfld_->selectionChanged.notify(
					mCB(this,uiIOSurface,ioDataSelChg) );
}


void uiIOSurface::mkRangeFld( bool multisubsel )
{
    BufferString username = ctio_->ctxt_.trgroup_->groupName();
    const bool is2d = username == EMHorizon2DTranslatorGroup::sGroupName();

    uiPosSubSel::Setup su( is2d, false );
    if ( multisubsel )
	su.choicetype( uiPosSubSel::Setup::VolumeTypes );
    rgfld_ = new uiPosSubSel( this, su );
    rgfld_->selChange.notify( mCB(this,uiIOSurface,ioDataSelChg) );
    if ( sectionfld_ ) rgfld_->attach( ensureBelow, sectionfld_ );
}


void uiIOSurface::mkObjFld( const uiString& lbl )
{
    ctio_->ctxt_.forread_ = forread_;
    objfld_ = new uiIOObjSel( this, *ctio_, lbl );
    if ( forread_ )
	objfld_->selectionDone.notify( mCB(this,uiIOSurface,objSel) );
}


bool uiIOSurface::fillFields( const MultiID& id, bool showerrmsg )
{
    EM::SurfaceIOData sd;
    if ( forread_ )
    {
	EM::IOObjInfo oi( id );
	uiString errmsg;
	if ( !oi.getSurfaceData(sd,errmsg) )
	{
	    if ( showerrmsg )
		uiMSG().error( errmsg );

	    return false;
	}
    }
    else
    {
	if ( id.isUdf() )
	    return false;

	const EM::ObjectID emid = EM::EMM().getObjectID( id );
	mDynamicCastGet(EM::Surface*,emsurf,EM::EMM().getObject(emid));
	if ( emsurf )
	    sd.use(*emsurf);
	else
	{
	    if ( showerrmsg )
		uiMSG().error( tr("Surface not loaded") );
	    return false;
	}
    }

    fillAttribFld( sd.valnames );
    fillSectionFld( sd.sections );
    fillRangeFld( sd.rg );
    return true;
}


void uiIOSurface::fillFields( const EM::ObjectID& emid )
{
    EM::SurfaceIOData sd;
    mDynamicCastGet( EM::Surface*, emsurf,EM::EMM().getObject(emid) );
    if ( emsurf )
	sd.use(*emsurf);
    else
    {
	uiMSG().error( tr("Temporal surface not existing") );
	    return;
    }

    fillAttribFld( sd.valnames );
    fillSectionFld( sd.sections );
    fillRangeFld( sd.rg );
}


void uiIOSurface::fillAttribFld( const BufferStringSet& valnames )
{
    if ( !attribfld_ ) return;

    attribfld_->setEmpty();
    attribfld_->addItems( valnames );
}


void uiIOSurface::getSelAttributes( BufferStringSet& names ) const
{
    names.erase();
    if ( attribfld_ )
	attribfld_->getChosen( names );
}


void uiIOSurface::setSelAttributes( const BufferStringSet& attribnames ) const
{
    if ( attribfld_ )
	attribfld_->setChosen( attribnames );
}


void uiIOSurface::setInput( const MultiID& mid ) const
{
    objfld_->setInput( mid );
}


void uiIOSurface::fillSectionFld( const BufferStringSet& sections )
{
    if ( !sectionfld_ ) return;

    sectionfld_->setEmpty();
    for ( int idx=0; idx<sections.size(); idx++ )
	sectionfld_->addItem( toUiString(sections[idx]->buf()) );
    sectionfld_->chooseAll( true );
}


void uiIOSurface::fillRangeFld( const TrcKeySampling& hrg )
{
    if ( !rgfld_ ) return;
    TrcKeyZSampling cs( rgfld_->envelope() );
    cs.hsamp_ = hrg;
    rgfld_->setInputLimit( cs );	// Set spinbox limits

    rgfld_->setInput( cs, SI().sampling(false) );
    // Bounds initial input values (!=limits) by survey range
    // when unedited subsel window is popped up.
}


bool uiIOSurface::haveAttrSel() const
{
    return attribfld_ ? attribfld_->nrChosen() > 0 : false;
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels ) const
{
    if ( !rgfld_ || rgfld_->isAll() )
	sels.rg = sels.sd.rg;
    else
	sels.rg = rgfld_->envelope().hsamp_;

    if ( SI().sampling(true) != SI().sampling(false) )
    {
	if ( sels.rg.isEmpty() )
	    sels.rg.init( true );

	sels.rg.limitTo( SI().sampling(true).hsamp_ );
    }

    sels.selsections.erase();
    if ( sectionfld_ )
	sectionfld_->getChosen( sels.selsections );
    else
	sels.selsections += 0;

    sels.selvalues.erase();
    if ( attribfld_ )
	attribfld_->getChosen( sels.selvalues );
}


const IOObj* uiIOSurface::selIOObj() const
{
    return objfld_->ioobj( true );
}


void uiIOSurface::objSel( CallBacker* )
{
    const IOObj* ioobj = objfld_ ? objfld_->ioobj( true ) : nullptr;
    if ( !ioobj )
	return;

    fillFields( ioobj->key(), false );
    inpChanged();
}


void uiIOSurface::attrSel( CallBacker* )
{
    attrSelChange.trigger();
}


// uiSurfaceWrite

uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const Setup& setup,
				const ZDomain::Info* zinfo )
    : uiIOSurface(p,false,setup.typ_,zinfo)
{
    surfrange_.init( false );

    if ( setup.typ_ != EMHorizon2DTranslatorGroup::sGroupName() )
    {
	if ( setup.withsubsel_ )
	    mkRangeFld();
	if ( sectionfld_ && rgfld_ )
	    rgfld_->attach( alignedBelow, sectionfld_ );
    }

    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::sGroupName() )
	mkObjFld( uiStrings::phrOutput(uiStrings::sFaultStickSet()) );
    else
	mkObjFld( uiStrings::phrOutput(setup.typname_) );

    if ( rgfld_ )
	objfld_->attach( alignedBelow, rgfld_ );

    if ( setup.withstratfld_ )
    {
	stratlvlfld_ = new uiStratLevelSel( this, true );
	stratlvlfld_->attach( alignedBelow, objfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiSurfaceWrite,stratLvlChg) );
    }

    if ( setup.withcolorfld_ )
    {
	colbut_ = new uiColorInput( this,
	   uiColorInput::Setup(OD::getRandStdDrawColor())
					.lbltxt(tr("Base color")));
	colbut_->attach( alignedBelow, objfld_ );
	if ( stratlvlfld_ ) colbut_->attach( ensureBelow, stratlvlfld_ );
    }

    if ( setup.withdisplayfld_ )
    {
       displayfld_ = new uiCheckBox( this, setup.displaytext_ );
       displayfld_->attach( alignedBelow, objfld_ );
       if ( stratlvlfld_ ) displayfld_->attach( ensureBelow, stratlvlfld_ );
       if ( colbut_ ) displayfld_->attach( ensureBelow, colbut_ );
       displayfld_->setChecked( true );
    }

    setHAlignObj( objfld_ );
    ioDataSelChg( nullptr );
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const EM::Surface& surf,
				const Setup& setup, const ZDomain::Info* zinfo )
    : uiIOSurface(p,false,setup.typ_,zinfo)
{
    surfrange_.init( false );

    if ( setup.typ_!=EMHorizon2DTranslatorGroup::sGroupName() &&
	 setup.typ_!=EMFaultStickSetTranslatorGroup::sGroupName() &&
	 setup.typ_!=EMFault3DTranslatorGroup::sGroupName() &&
	 setup.typ_!=EMBodyTranslatorGroup::sGroupName() )
    {
	if ( surf.nrSections() > 1 )
	    mkSectionFld( false );

	if ( setup.withsubsel_ )
	{
	    mkRangeFld();
	    EM::SurfaceIOData sd;
	    sd.use( surf );
	    surfrange_ = sd.rg;
	}

	if ( sectionfld_ && rgfld_ )
	    rgfld_->attach( alignedBelow, sectionfld_ );
    }

    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::sGroupName() )
	mkObjFld( uiStrings::phrOutput( uiStrings::sFaultStickSet() ) );
    else
	mkObjFld( uiStrings::phrOutput( setup.typname_ ) );

    const MultiID& surfkey = surf.multiID();
    if ( !surfkey.isUdf() )
	objfld_->setInput( surfkey );
    else
	objfld_->setInputText( surf.name() );

    if ( rgfld_ )
    {
	objfld_->attach( alignedBelow, rgfld_ );
	setHAlignObj( rgfld_ );
    }

    if ( setup.withdisplayfld_ )
    {
       displayfld_ = new uiCheckBox( this, setup.displaytext_ );
       displayfld_->attach( alignedBelow, objfld_ );
       displayfld_->setChecked( true );
    }

    if ( !fillFields(surfkey) )
	fillFields( surf.id() );

    ioDataSelChg( nullptr );
}


uiSurfaceWrite::~uiSurfaceWrite()
{}


bool uiSurfaceWrite::processInput()
{
    if ( sectionfld_ && sectionfld_->nrChosen() < 1 )
    {
	uiMSG().error( tr("Horizon has no patches") );
	return false;
    }

    const IOObj* ioobj = objfld_->ioobj();
    if ( ioobj )
	objfld_->setInputText( ioobj->name() );

    return ioobj;
}


bool uiSurfaceWrite::replaceInTree() const
{
    return displayfld_ ? displayfld_->isChecked() : false;
}


void uiSurfaceWrite::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ )
	return;

    const OD::Color col( stratlvlfld_->getColor() );
    if ( col != OD::Color::NoColor() )
	colbut_->setColor( col );
}


Strat::LevelID uiSurfaceWrite::getStratLevelID() const
{
    return stratlvlfld_ ? stratlvlfld_->getID() : Strat::LevelID::udf();
}


void uiSurfaceWrite::setColor( const OD::Color& col )
{
    if ( colbut_ )
	colbut_->setColor( col );
}


OD::Color uiSurfaceWrite::getColor() const
{
    return colbut_ ? colbut_->color() : OD::getRandStdDrawColor();
}


void uiSurfaceWrite::ioDataSelChg( CallBacker* )
{
    bool issubsel = sectionfld_ &&
		sectionfld_->size()!=sectionfld_->nrChosen();

    if ( !issubsel && rgfld_ && !rgfld_->isAll() )
    {
	const TrcKeySampling& hrg = rgfld_->envelope().hsamp_;
	issubsel = surfrange_.isEmpty() ? true :
	    !hrg.includes(surfrange_.start_) || !hrg.includes(surfrange_.stop_);
    }

    if ( displayfld_ && issubsel )
    {
	displayfld_->setChecked( false );
	displayfld_->setSensitive( false );
    }
    else if ( displayfld_ && !displayfld_->sensitive() )
    {
	displayfld_->setSensitive( true );
	displayfld_->setChecked( true );
    }
}


// uiSurfaceRead

uiSurfaceRead::uiSurfaceRead( uiParent* p, const Setup& setup,
			      const ZDomain::Info* zinfo )
    : uiIOSurface(p,true,setup.typ_,zinfo)
    , inpChange(this)
{
    if ( setup.typ_ == EMFault3DTranslatorGroup::sGroupName() )
	mkObjFld( uiStrings::phrInput(uiStrings::sFault()));
    else if ( setup.typ_ == EMFaultStickSetTranslatorGroup::sGroupName() )
	mkObjFld( uiStrings::phrInput(uiStrings::sFaultStickSet()));
    else
	mkObjFld( uiStrings::phrInput(toUiString(setup.typ_)) );

    uiGroup* attachobj = objfld_;

    if ( setup.withsectionfld_ )
	mkSectionFld( setup.withattribfld_ );

    if ( objfld_->ctxtIOObj().ioobj_ )
	objSel( nullptr );

    if ( setup.withattribfld_ )
    {
	mkAttribFld( setup.withsectionfld_ );
	attribfld_->attach( alignedBelow, objfld_ );
	if ( sectionfld_ ) sectionfld_->attach( rightTo, attribfld_ );
	attachobj = attribfld_;
	attribfld_->setMultiChoice( setup.multiattribsel_ );
    }
    else if ( setup.withsectionfld_ )
    {
	sectionfld_->attach( alignedBelow, objfld_ );
	attachobj = sectionfld_;
    }

    if ( setup.withsubsel_ )
    {
	mkRangeFld( setup.multisubsel_ );
	rgfld_->attach( alignedBelow, attachobj );
    }

    setHAlignObj( objfld_ );
}


uiSurfaceRead::~uiSurfaceRead()
{}


void uiSurfaceRead::setIOObj( const MultiID& mid )
{
    objfld_->setInput( mid );
    objSel( 0 );
}


bool uiSurfaceRead::processInput()
{
    if ( !objfld_->commitInput() )
    {
	uiMSG().error( tr("Please select input") );
	return false;
    }

    if ( sectionfld_ && sectionfld_->nrChosen()<1 )
    {
	uiMSG().error( tr("Horizon has no pataches") );
	return false;
    }

    return true;
}


// uiHorizonParSel

uiHorizonParSel::uiHorizonParSel( uiParent* p, bool is2d, bool wclear )
    : uiCompoundParSel(p,uiStrings::sHorizon(mPlural))
    , is2d_(is2d)
{
    butPush.notify( mCB(this,uiHorizonParSel,doDlg) );

    if ( wclear )
    {
	uiPushButton* clearbut = new uiPushButton( this, tr("Clear"), true );
	clearbut->activated.notify( mCB(this,uiHorizonParSel,clearPush) );
	clearbut->attach( rightOf, selbut_ );
    }

    txtfld_->setElemSzPol( uiObject::Wide );
}


uiHorizonParSel::~uiHorizonParSel()
{
}


void uiHorizonParSel::setSelected( const TypeSet<MultiID>& ids )
{
    selids_ = ids;
    updateSummary();
}


const TypeSet<MultiID>& uiHorizonParSel::getSelected() const
{ return selids_; }


BufferString uiHorizonParSel::getSummary() const
{
    SeparString ss;
    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids_[idx] );
	if ( !ioobj ) continue;

	ss.add( ioobj->name() );
    }

    return ss.buf();
}


void uiHorizonParSel::clearPush(CallBacker *)
{
    selids_.erase();
    updateSummary();
}


void uiHorizonParSel::doDlg(CallBacker *)
{
    IOObjContext ctxt = EM::Horizon::ioContext( is2d_, true );
    uiIOObjSelDlg::Setup sdsu( uiStrings::phrSelect(uiStrings::sHorizon(2)) );
			 sdsu.multisel( true );
    uiIOObjSelDlg dlg( this, sdsu, ctxt );
    dlg.selGrp()->setChosen( selids_ );
    if ( !dlg.go() ) return;

    selids_.erase();
    dlg.selGrp()->getChosen( selids_ );
}


void uiHorizonParSel::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<selids_.size(); idx++ )
	par.set( IOPar::compKey(sKey::Horizon(),idx), selids_[idx] );
}


bool uiHorizonParSel::usePar( const IOPar& par )
{
    TypeSet<MultiID> mids;
    MultiID mid;
    for ( int idx=0; ; idx++ )
    {
	const bool res = par.get( IOPar::compKey(sKey::Horizon(),idx), mid );
	if ( !res ) break;

	mids += mid;
    }

    setSelected( mids );
    updateSummary();
    return true;
}



// uiFaultParSel
class uiFSS2DLineSelDlg : public uiDialog
{ mODTextTranslationClass(uiFSS2DLineSelDlg)
public:
    uiFSS2DLineSelDlg( uiParent* p, const TypeSet<Pos::GeomID>& geomids )
	: uiDialog(p,Setup(tr("FaultStickSet selection"),
			   tr("Available for 2D lines"),mNoHelpKey))
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(EMFaultStickSet);
	const IODir iodir( ctio->ctxt_.getSelKey() );
	const IODirEntryList entlst( iodir, ctio->ctxt_ );

	for ( int idx=0; idx<entlst.size(); idx++ )
	{
	    const IOObj* obj = entlst[idx]->ioobj_;
	    if ( !obj ) continue;

	    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded(obj->key());
	    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
	    if ( !fss ) continue;

	    const int nrsticks = fss->geometry().nrSticks();

	    bool fssvalid = false;
	    for ( int gidx=0; gidx<geomids.size(); gidx++ )
	    {
		if ( fssvalid ) break;
		for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
		{
		    const Geometry::FaultStickSet* fltgeom =
			fss->geometry().geometryElement();
		    if ( !fltgeom ) continue;

		    const int sticknr = fltgeom->rowRange().atIndex( stickidx );
		    if ( !fss->geometry().pickedOn2DLine(sticknr) )
			continue;

		    if ( geomids[gidx] ==
				fss->geometry().pickedGeomID(sticknr))
		    { fssvalid = true; break; }
		}
	    }

	    if ( fssvalid )
	    {
		validfss_.add( fss->name() );
		validmids_ += obj->key();
	    }
	}

	fsslistfld_ = new uiListBox( this, "", OD::ChooseAtLeastOne );
	fsslistfld_->setNrLines( validmids_.size()+1 );
	fsslistfld_->setFieldWidth( 20 );
	fsslistfld_->addItems( validfss_ );
    }

    void getSelected( BufferStringSet& nms, TypeSet<MultiID>& mids )
    {
	TypeSet<int> selids;
	fsslistfld_->getChosen( selids );
	for ( int idx=0; idx<selids.size(); idx++ )
	{
	    nms.add( *validfss_[selids[idx]] );
	    mids += validmids_[selids[idx]];
	}
    }

    void setSelectedItems( BufferStringSet sel )
    { fsslistfld_->setChosen(sel); }


    uiListBox*		fsslistfld_;
    BufferStringSet	validfss_;
    TypeSet<MultiID>	validmids_;
};


static int getUpdateOptIdx( int curoptidx, bool is2d, bool toui )
{
   if ( curoptidx <= 1 )
      return curoptidx;

  int cursel = curoptidx;
  if ( toui )
      cursel -= is2d ? mIdxShift2D : mIdxShift3D;
  else
      cursel += is2d ? mIdxShift2D : mIdxShift3D;
  return cursel;
}


class uiFaultOptSel: public uiDialog
{ mODTextTranslationClass(uiFaultOptSel)
public:
    uiFaultOptSel( uiParent* p, uiFaultParSel& fltpar )
	: uiDialog(p,Setup(
		    tr("%1 selection").arg(
			fltpar.type() == EM::ObjectType::Flt3D
			 ? uiStrings::sFault()
			 : (EM::isFaultStickSet( fltpar.type() )
			     ? uiStrings::sFaultStickSet()
			     : uiStrings::sFaultSet())),
		    mODHelpKey(mFaultOptSelHelpID)))
	, fltpar_(fltpar)
    {
	const uiString fltnm = fltpar.type() == EM::ObjectType::Flt3D
			     ? uiStrings::sFault()
			    : (EM::isFaultStickSet( fltpar_.type() )
				     ? uiStrings::sFaultStickSet()
				     : uiStrings::sFaultSet() );
	table_ = new uiTable( this, uiTable::Setup().rowgrow(true).
		rowdesc(fltnm).defrowlbl("").selmode(uiTable::Multi).
		rightclickdisabled(true), "Fault Boundary Table");
	uiStringSet collbls;
        collbls.add( uiStrings::sName() ).add( tr("Boundary Type") );
	table_->setColumnLabels( collbls );
	table_->setTableReadOnly( true );
	table_->setPrefHeight( 150 );
	table_->setPrefWidth( 350 );
	table_->setColumnResizeMode( uiTable::ResizeToContents );
	table_->setColumnStretchable( 1, true );
	table_->setSelectionBehavior( uiTable::SelectRows );

	auto* addbut = new uiPushButton( this, uiStrings::sAdd(),
					 mCB(this,uiFaultOptSel,addCB), false );
	addbut->attach( rightOf, table_ );

	removebut_ = new uiPushButton( this, uiStrings::sRemove(),
				       mCB(this,uiFaultOptSel,removeCB), true );
	removebut_->attach( alignedBelow, addbut );

	for ( int idx=0; idx<fltpar_.selfaultids_.size(); idx++ )
	{
	    PtrMan<IOObj> ioobj = IOM().get( fltpar_.selfaultids_[idx] );
	    if ( ioobj )
		addObjEntry( idx, *ioobj, fltpar_.optids_[idx] );
	}
    }

    void addCB( CallBacker* )
    {
	PtrMan<CtxtIOObj> objio = fltpar_.type() == EM::ObjectType::Flt3D
			    ? mMkCtxtIOObj(EMFault3D)
			    : (EM::isFaultStickSet( fltpar_.type() )
				    ? mMkCtxtIOObj(EMFaultStickSet)
				    : mMkCtxtIOObj(EMFaultSet3D));
	uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
	uiIOObjSelDlg dlg( this, sdsu, *objio );
	if ( dlg.go() != uiDialog::Accepted )
	    return;

	const int nrsel = dlg.nrChosen();
	for ( int idx=0; idx<nrsel; idx++ )
	{
	    const MultiID& mid = dlg.chosenID( idx );
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( !ioobj || fltpar_.selfaultnms_.isPresent(ioobj->name()) )
		continue;

	    addObjEntry( fltpar_.selfaultids_.size(), *ioobj,
		    fltpar_.defaultoptidx_ );
	}

	removebut_->setSensitive( fltpar_.selfaultids_.size() );
    }

    void addObjEntry( int row, const IOObj& ioobj, int optidx )
    {
	if ( row==table_->nrRows() )
	    table_->insertRows( row, 1 );

	const bool isfss = EM::isFaultStickSet( fltpar_.type() );
	auto* actopts = new uiComboBox( nullptr, "Boundary Type" );
	actopts->disabFocus();
	actopts->addItems( fltpar_.optnms_ );
	mAttachCB( actopts->selectionChanged, uiFaultOptSel::optCB );
	const int cursel = getUpdateOptIdx( optidx, isfss, true );
	actopts->setCurrentItem( cursel );
	table_->setCellObject( RowCol(row,1), actopts );

	const char * fltnm = ioobj.name();
	table_->setText( RowCol(row,0), fltnm );

	if ( fltpar_.selfaultnms_.isPresent(fltnm) )
	    return;

	fltpar_.selfaultids_ += ioobj.key();
	fltpar_.selfaultnms_.add( fltnm );
	fltpar_.optids_ += optidx;
    }

    void removeCB( CallBacker* )
    {
	TypeSet<int> selrows;
	table_->getSelectedRows( selrows );

	const int firstselrow = selrows.isEmpty() ? -1 : selrows[0];
	for ( int idx=selrows.size()-1; idx>=0; idx-- )
	{
	    const int currow = selrows[idx];
	    if ( currow==-1 ) continue;

	    if ( currow<fltpar_.selfaultids_.size() )
	    {
		fltpar_.selfaultids_.removeSingle( currow );
		fltpar_.selfaultnms_.removeSingle( currow );
		fltpar_.optids_.removeSingle( currow );
	    }

	    table_->removeRow( currow );
	}

	removebut_->setSensitive( fltpar_.selfaultids_.size() );
	const int newselrow = firstselrow < fltpar_.selfaultids_.size()
	    ? firstselrow : firstselrow-1;
	table_->selectRow( newselrow );
    }

    void optCB( CallBacker* cb )
    {
	mDynamicCastGet(uiComboBox*,selbox,cb)
	if ( !selbox )
	    return;

	const RowCol selrc = table_->getCell( selbox );
	if ( selrc.row()<0 || !table_->isRowSelected(selrc.row()) )
	    return;

	const int selitm = selbox->currentItem();

	TypeSet<int> selrows;
	table_->getSelectedRows( selrows );
	for ( int ridx=0; ridx<selrows.size(); ridx++ )
	{
	    const int currow = selrows[ridx];
	    mDynamicCastGet(uiComboBox*,curselbox,
		    table_->getCellObject(RowCol(currow,1)) );
	    if ( curselbox != selbox )
		curselbox->setValue( selitm );
	}
    }

    bool acceptOK( CallBacker* ) override
    {
	const bool isfss = EM::isFaultStickSet( fltpar_.type() );
	for ( int idx=0; idx<fltpar_.optids_.size(); idx++ )
	{
	    mDynamicCastGet(uiComboBox*,selbox,
		    table_->getCellObject(RowCol(idx,1)) );

	    const int cursel = selbox->currentItem();
	    const int optidx = getUpdateOptIdx( cursel, isfss, false );
	    fltpar_.optids_[idx] = optidx;
	}

	return true;
    }

    uiFaultParSel&	fltpar_;
    uiTable*		table_;
    uiPushButton*	removebut_;
};


//uiFaultParSel

uiFaultParSel::uiFaultParSel( uiParent* p, EM::ObjectType typ,
			      bool useoptions )
    : uiCompoundParSel(p,toUiString("***************"))
      //Hack So that textfld_ label is correctly updated
    , selChange(this)
    , objtype_(typ)
    , useoptions_(useoptions)
    , defaultoptidx_(0)
{
    mAttachCB( butPush, uiFaultParSel::doDlg );
    setSelText( typ == EM::ObjectType::Flt3D
		? uiStrings::sFault(mPlural)
		: (EM::isFaultStickSet(typ)
		    ? uiStrings::sFaultStickSet(mPlural)
		    : uiStrings::sFaultSet(mPlural)) );

    clearbut_ = new uiPushButton( this, uiStrings::sClear(),
				  mCB(this,uiFaultParSel,clearPush), true );
    clearbut_->attach( rightOf, selbut_ );

    txtfld_->setElemSzPol( uiObject::Wide );
    setHAlignObj( txtfld_ );
}


uiFaultParSel::~uiFaultParSel()
{
    detachAllNotifiers();
}


EM::ObjectType uiFaultParSel::type() const
{
    return objtype_;
}


void uiFaultParSel::hideClearButton( bool yn )
{
    clearbut_->display( !yn, true );
}


void uiFaultParSel::setSelectedFaults( const TypeSet<MultiID>& ids,
				       const TypeSet<FaultTrace::Act>* act )
{
    selfaultids_.erase();
    selfaultnms_.erase();
    optids_.erase();

    const EM::ObjectType typ = type();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ids[idx] );
	if ( !ioobj )
	    continue;

	const EM::IOObjInfo info( *ioobj.ptr() );
	if ( !info.isOK() || info.type() != typ )
	    continue;

	selfaultnms_.add( ioobj->name() );
	selfaultids_ += ids[idx];

	optids_ += act && act->validIdx(idx) ? (*act)[idx]
					     : FaultTrace::AllowCrossing;
    }

    updSummary( nullptr );
    selChange.trigger();
}


void uiFaultParSel::clearPush( CallBacker* )
{
    selfaultnms_.erase();
    selfaultids_.erase();
    optids_.erase();
    updSummary( nullptr );
    selChange.trigger();
}


void uiFaultParSel::setEmpty()
{
    clearPush( nullptr );
}


void uiFaultParSel::setGeomIDs( const TypeSet<Pos::GeomID>& geomids )
{
    geomids_ = geomids;
}


void uiFaultParSel::doDlg( CallBacker* )
{
    if ( useoptions_ && !optnms_.isEmpty() )
    {
	uiFaultOptSel dlg( this, *this );
	if ( dlg.go() != uiDialog::Accepted )
	    return;
    }
    else if ( !geomids_.isEmpty() )
    {
	uiFSS2DLineSelDlg dlg( this, geomids_ );
	dlg.setSelectedItems( selfaultnms_ );
	if ( dlg.go() != uiDialog::Accepted )
	    return;

	selfaultnms_.erase();
	selfaultids_.erase();
	dlg.getSelected( selfaultnms_, selfaultids_ );
    }
    else
    {
	const EM::ObjectType typ = type();
	PtrMan<CtxtIOObj> ctio = typ == EM::ObjectType::Flt3D
			       ? mMkCtxtIOObj(EMFault3D)
			       : (EM::isFaultStickSet(typ)
				       ? mMkCtxtIOObj(EMFaultStickSet)
				       : mMkCtxtIOObj(EMFaultSet3D));
	const uiString fltnm = typ == EM::ObjectType::Flt3D
			     ? uiStrings::sFault()
			     : (EM::isFaultStickSet(typ)
				     ? uiStrings::sFaultStickSet()
				     : uiStrings::sFaultSet() );
	uiIOObjSelDlg::Setup sdsu( uiStrings::phrSelect(fltnm) );
			     sdsu.multisel( true );
	uiIOObjSelDlg dlg( this, sdsu, *ctio );
	dlg.selGrp()->getListField()->setChosen( selfaultnms_ );
	if ( dlg.go() != uiDialog::Accepted )
	    return;

	selfaultnms_.erase();
	selfaultids_.erase();
	uiIOObjSelGrp* selgrp = dlg.selGrp();
	selgrp->getListField()->getChosen( selfaultnms_ );
	for ( int idx=0; idx<selfaultnms_.size(); idx++ )
	    selfaultids_ += selgrp->chosenID(idx);
    }

    selChange.trigger();
}


BufferString uiFaultParSel::getSummary() const
{
    const bool addopt = useoptions_ && !optnms_.isEmpty();
    BufferString summ;
    const bool isfss = EM::isFaultStickSet( type() );
    for ( int idx=0; idx<selfaultnms_.size(); idx++ )
    {
	summ += selfaultnms_.get(idx);
	if ( addopt )
	{
	    const int optnmidx = getUpdateOptIdx( optids_[idx], isfss, true );
	    summ += " (";
	    summ += optnms_[optnmidx]->buf();
	    summ += ")";
	}

	summ += idx == selfaultnms_.size()-1 ? "." : ", ";
    }
    return summ.isEmpty() ? BufferString(" - ") : summ;
}


void uiFaultParSel::setActOptions( const BufferStringSet& opts, int dftoptidx )
{
    optnms_.erase();
    optnms_ = opts;
    defaultoptidx_ = opts.validIdx(dftoptidx) ? dftoptidx : 0;
}


// uiAuxDataGrp

uiAuxDataGrp::uiAuxDataGrp( uiParent* p, bool forread )
    : uiGroup(p,"AuxData Group")
    , inpfld_(nullptr)
{
    listfld_ = new uiListBox( this, uiStrings::sHorizonData() );
    listfld_->setHSzPol( uiObject::Wide );

    if ( !forread )
    {
	mAttachCB( listfld_->selectionChanged, uiAuxDataGrp::selChg );

	inpfld_ = new uiGenInput( this, uiStrings::sName() );
	inpfld_->setElemSzPol( uiObject::Wide );
	inpfld_->attach( alignedBelow, listfld_ );
    }
}


uiAuxDataGrp::~uiAuxDataGrp()
{
    detachAllNotifiers();
}


void uiAuxDataGrp::setKey( const MultiID& key )
{
    EM::IOObjInfo info( key );
    BufferStringSet nms;
    info.getAttribNames( nms );
    listfld_->addItems( nms );
}


void uiAuxDataGrp::setDataName( const char* nm )
{
    listfld_->setCurrentItem( nm );
    selChg( nullptr );
}


const char* uiAuxDataGrp::getDataName() const
{
    if ( inpfld_ )
	return inpfld_->text();

    return listfld_->getText();
}


void uiAuxDataGrp::selChg( CallBacker* )
{
    if ( inpfld_ )
	inpfld_->setText( listfld_->getText() );
}



// uiAuxDataSel
uiAuxDataSel::uiAuxDataSel( uiParent* p, const char* typ, bool withobjsel,
			    bool forread )
    : uiGroup(p,"AuxDataSel")
    , objfld_(nullptr)
    , objtype_(typ)
    , key_(MultiID::udf())
    , forread_(forread)
{
    if ( withobjsel )
    {
	objfld_ = new uiHorizon3DSel( this, true );
	mAttachCB( objfld_->selectionDone, uiAuxDataSel::objSelCB );
    }

    uiString seltxt = forread ? uiStrings::sInput() : uiStrings::sOutput();
    seltxt.append( uiStrings::sHorizonData() );
    auxdatafld_ = new uiIOSelect( this, uiIOSelect::Setup(seltxt),
				  mCB(this,uiAuxDataSel,auxSelCB) );
    if ( objfld_ )
	auxdatafld_->attach( alignedBelow, objfld_ );
    setHAlignObj( auxdatafld_ );

    mAttachCB( postFinalize(), uiAuxDataSel::finalizeCB );
}


uiAuxDataSel::~uiAuxDataSel()
{
    detachAllNotifiers();
}


void uiAuxDataSel::finalizeCB( CallBacker* )
{
    const IOObj* ioobj = objfld_ ? objfld_->ioobj(true) : nullptr;
    if ( !ioobj )
	return;

    setKey( ioobj->key() );
}


void uiAuxDataSel::setKey( const MultiID& key )
{
    key_ = key;

    EM::IOObjInfo info( key );
    BufferStringSet nms;
    info.getAttribNames( nms );
    auxdatafld_->setEntries( nms, nms );
}


void uiAuxDataSel::setDataName( const char* nm )
{
    auxdatafld_->setInputText( nm );
}


MultiID uiAuxDataSel::getKey() const
{
    return objfld_ ? objfld_->key() : key_;
}


const char* uiAuxDataSel::getDataName() const
{
    return auxdatafld_->getInput();
}


void uiAuxDataSel::objSelCB( CallBacker* )
{
    key_ = objfld_ ? objfld_->key() : MultiID::udf();
}


void uiAuxDataSel::auxSelCB( CallBacker* )
{
    uiDialog dlg( this,
	uiDialog::Setup(tr("Select Horizon Data"),mTODOHelpKey));
    auto* grp = new uiAuxDataGrp( &dlg, forread_ );
    grp->setKey( key_ );
    BufferString datanm = auxdatafld_->getInput();
    grp->setDataName( datanm );
    if ( !dlg.go() )
	return;

    datanm = grp->getDataName();
    setDataName( datanm );
}



// uiBodySel
uiBodySel::uiBodySel( uiParent* p, bool forread,
			    const uiIOObjSel::Setup& setup )
    : uiIOObjSel(p,mRWIOObjContext(EMBody,forread),setup)
{
    if ( setup.seltxt_.isEmpty() )
	setLabelText( forread
		     ? uiStrings::phrInput( uiStrings::sGeobody() )
		     : uiStrings::phrOutput( uiStrings::sGeobody() ) );
    fillEntries();
}


uiBodySel::uiBodySel( uiParent* p, bool forread )
    : uiIOObjSel(p,mRWIOObjContext(EMBody,forread),Setup())
{
    setLabelText( forread
		 ? uiStrings::phrInput( uiStrings::sGeobody() )
		 : uiStrings::phrOutput( uiStrings::sGeobody() ) );
    fillEntries();
}


uiBodySel::~uiBodySel()
{}



// uiHorizonSel

uiHorizonSel::uiHorizonSel( uiParent* p, bool is2d, const ZDomain::Info* zinfo,
			    bool forread, const uiIOObjSel::Setup& setup )
    : uiIOObjSel(p,EM::Horizon::ioContext(is2d,zinfo,forread),setup)
{
    if ( setup.seltxt_.isEmpty() )
	setLabelText( forread
		     ? uiStrings::phrInput( uiStrings::sHorizon() )
		     : uiStrings::phrOutput( uiStrings::sHorizon() ) );
    fillEntries();
}


uiHorizonSel::uiHorizonSel( uiParent* p, bool is2d,
			    bool forread, const uiIOObjSel::Setup& setup )
    : uiIOObjSel(p,EM::Horizon::ioContext(is2d,forread),setup)
{
    if ( setup.seltxt_.isEmpty() )
	setLabelText( forread
		     ? uiStrings::phrInput( uiStrings::sHorizon() )
		     : uiStrings::phrOutput( uiStrings::sHorizon() ) );
    fillEntries();
}


uiHorizonSel::~uiHorizonSel()
{}


// uiHorizon3DSel
uiHorizon3DSel::uiHorizon3DSel( uiParent* p, const ZDomain::Info* zinfo,
			    bool forread, const uiIOObjSel::Setup& setup )
    : uiHorizonSel(p,false,zinfo,forread,setup)
{
}


uiHorizon3DSel::uiHorizon3DSel( uiParent* p,
			    bool forread, const uiIOObjSel::Setup& setup )
    : uiHorizonSel(p,false,forread,setup)
{
}


uiHorizon3DSel::~uiHorizon3DSel()
{}


//uiFaultSel
static uiString getLabelText( bool forread, EM::ObjectType type )
{
    uiString showstr;
    if ( type == EM::ObjectType::Flt3D )
	showstr = uiStrings::sFault();
    else if ( EM::isFaultStickSet(type) )
	showstr = uiStrings::sFaultStickSet();
    else
	showstr = uiStrings::sFaultSet();

    return forread ? uiStrings::phrInput( showstr )
		   : uiStrings::phrOutput( showstr );
}


static IOObjContext ioContext( bool isforread,
		EM::ObjectType type, const ZDomain::Info* zinfo )
{

    IOObjContext ctxt( nullptr );
    if ( type == EM::ObjectType::Flt3D )
	ctxt = mIOObjContext(EMFault3D);
    else if ( EM::isFaultStickSet(type) )
	ctxt = mIOObjContext(EMFaultStickSet);
    else
	ctxt = mIOObjContext(EMFaultSet3D);

    ctxt.forread_ = isforread;
    if ( zinfo )
    {
	const ZDomain::Info& siinfo = SI().zDomainInfo();
	ctxt.requireZDomain( *zinfo, siinfo == *zinfo );
    }

    return ctxt;
}


uiFaultSel::uiFaultSel( uiParent* p, EM::ObjectType type,
			const ZDomain::Info* zinfo, bool isforread,
			const uiIOObjSel::Setup& su )
    : uiIOObjSel(p,ioContext(isforread,type,zinfo),su)
{
    if ( su.seltxt_.isEmpty() )
	setLabelText( getLabelText(isforread,type) );

    fillEntries();
}


uiFaultSel::uiFaultSel( uiParent* p, EM::ObjectType type,
			bool isforread, const uiIOObjSel::Setup& su )
    : uiFaultSel(p,type,nullptr,isforread,su)
{
}


uiFaultSel::~uiFaultSel()
{}
