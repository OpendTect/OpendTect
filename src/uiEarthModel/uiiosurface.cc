/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiiosurface.h"

#include "uipossubsel.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uitable.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "embodytr.h"
#include "emfaultstickset.h"
#include "emsurface.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"
#include "survinfo.h"

const int cListHeight = 5;

uiIOSurface::uiIOSurface( uiParent* p, bool forread, const char* tp )
    : uiGroup(p,"Surface selection")
    , ctio_( 0 )
    , sectionfld_(0)
    , attribfld_(0)
    , rgfld_(0)
    , attrSelChange(this)
    , forread_(forread)
    , objfld_(0)
{
    const FixedString typ( tp );
    if ( typ == EMHorizon2DTranslatorGroup::keyword() )
	ctio_ = mMkCtxtIOObj(EMHorizon2D);
    else if ( typ == EMHorizon3DTranslatorGroup::keyword() )
	ctio_ = mMkCtxtIOObj(EMHorizon3D);
    else if ( typ == EMFaultStickSetTranslatorGroup::keyword() )
	ctio_ = mMkCtxtIOObj(EMFaultStickSet);
    else if ( typ == EMFault3DTranslatorGroup::keyword() )
	ctio_ = mMkCtxtIOObj(EMFault3D);
    else
	ctio_ = new CtxtIOObj( polygonEMBodyTranslator::getIOObjContext() );

    postFinalise().notify( mCB(this,uiIOSurface,objSel) );
}


uiIOSurface::~uiIOSurface()
{
    delete ctio_->ioobj; delete ctio_;
}


void uiIOSurface::mkAttribFld( bool labelabove )
{
    attribfld_ = new uiLabeledListBox( this, tr("Calculated attributes"),
		OD::ChooseAtLeastOne, labelabove ? uiLabeledListBox::AboveMid
						  : uiLabeledListBox::LeftTop );
    attribfld_->setStretch( 2, 2 );
    attribfld_->box()->selectionChanged.notify( mCB(this,uiIOSurface,attrSel) );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    sectionfld_ = new uiLabeledListBox( this, tr("Available patches"),
	    OD::ChooseAtLeastOne, labelabove ? uiLabeledListBox::AboveMid
					      : uiLabeledListBox::LeftTop );
    sectionfld_->setPrefHeightInChar( mCast(float,cListHeight) );
    sectionfld_->setStretch( 2, 2 );
    sectionfld_->box()->selectionChanged.notify(
					mCB(this,uiIOSurface,ioDataSelChg) );
}


void uiIOSurface::mkRangeFld( bool multisubsel )
{
    BufferString username = ctio_->ctxt.trgroup->userName();
    const bool is2d = username == EMHorizon2DTranslatorGroup::keyword();

    uiPosSubSel::Setup su( is2d, false );
    if ( multisubsel )
	su.choicetype( uiPosSubSel::Setup::OnlySeisTypes );
    rgfld_ = new uiPosSubSel( this, su );
    rgfld_->selChange.notify( mCB(this,uiIOSurface,ioDataSelChg) );
    if ( sectionfld_ ) rgfld_->attach( ensureBelow, sectionfld_ );
}


void uiIOSurface::mkObjFld( const uiString& lbl )
{
    ctio_->ctxt.forread = forread_;
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


void uiIOSurface::fillAttribFld( const BufferStringSet& valnames )
{
    if ( !attribfld_ ) return;

    attribfld_->box()->setEmpty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attribfld_->box()->addItem( valnames[idx]->buf() );
}


void uiIOSurface::getSelAttributes( BufferStringSet& names ) const
{
    names.erase();
    if ( attribfld_ )
	attribfld_->box()->getChosen( names );
}


void uiIOSurface::setSelAttributes( const BufferStringSet& attribnames ) const
{
    if ( attribfld_ )
	attribfld_->box()->setChosen( attribnames );
}


void uiIOSurface::setInput( const MultiID& mid ) const
{
    objfld_->setInput( mid );
}


void uiIOSurface::fillSectionFld( const BufferStringSet& sections )
{
    if ( !sectionfld_ ) return;

    sectionfld_->box()->setEmpty();
    for ( int idx=0; idx<sections.size(); idx++ )
	sectionfld_->box()->addItem( sections[idx]->buf() );
    sectionfld_->box()->chooseAll( true );
}


void uiIOSurface::fillRangeFld( const HorSampling& hrg )
{
    if ( !rgfld_ ) return;
    CubeSampling cs( rgfld_->envelope() );
    cs.hrg = hrg;
    rgfld_->setInputLimit( cs );	// Set spinbox limits

    rgfld_->setInput( cs, SI().sampling(false) );
    // Bounds initial input values (!=limits) by survey range
    // when unedited subsel window is popped up.
}


bool uiIOSurface::haveAttrSel() const
{
    return attribfld_->box()->nrChosen() > 0;
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels ) const
{
    if ( !rgfld_ || rgfld_->isAll() )
	sels.rg = sels.sd.rg;
    else
	sels.rg = rgfld_->envelope().hrg;

    if ( SI().sampling(true) != SI().sampling(false) )
    {
	if ( sels.rg.isEmpty() )
	    sels.rg.init( true );
	sels.rg.limitTo( SI().sampling(true).hrg );
    }

    sels.selsections.erase();
    if ( sectionfld_ )
	sectionfld_->box()->getChosen( sels.selsections );
    else
	sels.selsections += 0;

    sels.selvalues.erase();
    if ( attribfld_ )
	attribfld_->box()->getChosen( sels.selvalues );
}


IOObj* uiIOSurface::selIOObj() const
{
    objfld_->commitInput();
    return ctio_->ioobj;
}


void uiIOSurface::objSel( CallBacker* )
{
    if ( !objfld_ ) return;

    objfld_->commitInput();
    IOObj* ioobj = objfld_->ctxtIOObj().ioobj;
    if ( !ioobj ) return;

    fillFields( ioobj->key() );
    inpChanged();
}


void uiIOSurface::attrSel( CallBacker* )
{
    attrSelChange.trigger();
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p,
				const uiSurfaceWrite::Setup& setup )
    : uiIOSurface(p,false,setup.typ_)
    , displayfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
{
    surfrange_.init( false );

    if ( setup.typ_ != EMHorizon2DTranslatorGroup::keyword() )
    {
	if ( setup.withsubsel_ )
	    mkRangeFld();
	if ( sectionfld_ && rgfld_ )
	    rgfld_->attach( alignedBelow, sectionfld_ );
    }

    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( "Output Stickset" );
    else
	mkObjFld( BufferString("Output ",setup.typ_) );


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
	    uiColorInput::Setup(getRandStdDrawColor()).lbltxt("Base color") );
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
    ioDataSelChg( 0 );
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const EM::Surface& surf,
				const uiSurfaceWrite::Setup& setup )
    : uiIOSurface(p,false,setup.typ_)
    , displayfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
{
    surfrange_.init( false );

    if ( setup.typ_!=EMHorizon2DTranslatorGroup::keyword() &&
	 setup.typ_!=EMFaultStickSetTranslatorGroup::keyword() &&
	 setup.typ_!=EMFault3DTranslatorGroup::keyword() &&
	 setup.typ_!=polygonEMBodyTranslator::sKeyUserName() )
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

    if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( tr("Output Stickset") );
    else
	mkObjFld( tr("Output %1").arg( setup.typname_ ) );

    if ( rgfld_ )
    {
	objfld_->attach( alignedBelow, rgfld_ );
	setHAlignObj( rgfld_ );
    }

    if ( setup.withdisplayfld_ )
    {
       displayfld_ = new uiCheckBox( this, "Replace in tree" );
       displayfld_->attach( alignedBelow, objfld_ );
       displayfld_->setChecked( true );
    }

    fillFields( surf.multiID() );

    ioDataSelChg( 0 );
}


bool uiSurfaceWrite::processInput()
{
    if ( sectionfld_ && sectionfld_->box()->nrChosen() < 1 )
	{ uiMSG().error( "Horizon has no patches" ); return false; }

    if ( !objfld_->commitInput() )
    {
	if ( objfld_->isEmpty() )
	    uiMSG().error( "Please select Output" );
	return false;
    }

    return true;
}


bool uiSurfaceWrite::replaceInTree() const
{ return displayfld_ ? displayfld_->isChecked() : false; }


void uiSurfaceWrite::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const Color col( stratlvlfld_->getColor() );
    if ( col != Color::NoColor() )
	colbut_->setColor( col );
}


int uiSurfaceWrite::getStratLevelID() const
{
    return stratlvlfld_ ? stratlvlfld_->getID() : -1;
}


void uiSurfaceWrite::setColor( const Color& col )
{ if ( colbut_ ) colbut_->setColor( col ); }


Color uiSurfaceWrite::getColor() const
{
    return colbut_ ? colbut_->color() : getRandStdDrawColor();
}


void uiSurfaceWrite::ioDataSelChg( CallBacker* )
{
    bool issubsel = sectionfld_ &&
		sectionfld_->box()->size()!=sectionfld_->box()->nrChosen();

    if ( !issubsel && rgfld_ && !rgfld_->isAll() )
    {
	const HorSampling& hrg = rgfld_->envelope().hrg;
	issubsel = surfrange_.isEmpty() ? true :
	    !hrg.includes(surfrange_.start) || !hrg.includes(surfrange_.stop);
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


uiSurfaceRead::uiSurfaceRead( uiParent* p, const Setup& setup )
    : uiIOSurface(p,true,setup.typ_)
    , inpChange(this)
{
    if ( setup.typ_ == EMFault3DTranslatorGroup::keyword() )
	mkObjFld( "Input Fault" );
    else if ( setup.typ_ == EMFaultStickSetTranslatorGroup::keyword() )
	mkObjFld( "Input Stickset" );
    else
	mkObjFld( BufferString("Input ",setup.typ_) );

    uiGroup* attachobj = objfld_;

    if ( setup.withsectionfld_ )
	mkSectionFld( setup.withattribfld_ );

    if ( objfld_->ctxtIOObj().ioobj )
	objSel(0);

    if ( setup.withattribfld_ )
    {
	mkAttribFld( setup.withsectionfld_ );
	attribfld_->attach( alignedBelow, objfld_ );
	if ( sectionfld_ ) sectionfld_->attach( rightTo, attribfld_ );
	attachobj = attribfld_;
	attribfld_->box()->setMultiChoice( setup.multiattribsel_ );
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


void uiSurfaceRead::setIOObj( const MultiID& mid )
{
    objfld_->setInput( mid );
    objSel( 0 );
}


bool uiSurfaceRead::processInput()
{
    if ( !objfld_->commitInput() )
	{ uiMSG().error( "Please select input" ); return false; }

    if ( sectionfld_ && sectionfld_->box()->nrChosen()<1 )
	{ uiMSG().error( "Horizon has no pataches" ); return false; }

    return true;
}


// uiFaultParSel
class uiFSS2DLineSelDlg : public uiDialog
{
public:
    uiFSS2DLineSelDlg( uiParent* p, const TypeSet<Pos::GeomID>& geomids )
	: uiDialog(p,uiDialog::Setup("FaultStickSet selection",
		    "Available for 2D lines",mNoHelpKey))
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(EMFaultStickSet);
	const IODir iodir( ctio->ctxt.getSelKey() );
	const IODirEntryList entlst( iodir, ctio->ctxt );

	for ( int idx=0; idx<entlst.size(); idx++ )
	{
	    const IOObj* obj = entlst[idx]->ioobj_;
	    if ( !obj ) continue;

	    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded(obj->key());
	    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
	    if ( !fss ) continue;

	    EM::SectionID sid = fss->sectionID(0);
	    const int nrsticks = fss->geometry().nrSticks( sid );

	    bool fssvalid = false;
	    for ( int gidx=0; gidx<geomids.size(); gidx++ )
	    {
		if ( fssvalid ) break;
		for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
		{
		    const Geometry::FaultStickSet* fltgeom =
			fss->geometry().sectionGeometry( sid );
		    if ( !fltgeom ) continue;

		    const int sticknr = fltgeom->rowRange().atIndex( stickidx );
		    if ( !fss->geometry().pickedOn2DLine(sid, sticknr) )
			continue;

		    if ( geomids[gidx] ==
				fss->geometry().pickedGeomID(sid,sticknr))
		    { fssvalid = true; break; }
		}
	    }

	    if ( fssvalid )
	    {
		validfss_.add( fss->name() );
		validmids_ += obj->key();
	    }
	}

	fsslistfld_ = new uiListBox(this,"",OD::ChooseAtLeastOne,
				validmids_.size()+1,20 );
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


class uiFaultOptSel: public uiDialog
{
public:
    uiFaultOptSel( uiParent* p, uiFaultParSel& fltpar )
	: uiDialog(p,uiDialog::Setup(
		    fltpar.is2d_ ? "FaultStickSet selection":"Fault selection",
		    mNoDlgTitle,mTODOHelpKey) )
	, fltpar_(fltpar)
    {
	table_ = new uiTable( this, uiTable::Setup().rowgrow(true).fillrow(
		    true).rightclickdisabled(true).selmode(uiTable::Single),"");
	const char* fltnm = fltpar.is2d_ ? "FaultStickSet" : "Fault";
	const char* collbls[] = { fltnm, "Boundary Type", 0 };
	table_->setColumnLabels( collbls );
	table_->setLeftMargin( 0 );
	table_->setSelectionBehavior( uiTable::SelectRows );
	table_->setColumnResizeMode( uiTable::ResizeToContents );
	table_->setRowResizeMode( uiTable::Interactive );
	table_->setColumnStretchable( 0, true );
	table_->setColumnStretchable( 1, true );

	uiPushButton* addbut = new uiPushButton( this, "&Add",
		mCB(this,uiFaultOptSel,addCB), false );
	addbut->attach( rightOf, table_ );

	removebut_ = new uiPushButton( this, "&Remove",
		mCB(this,uiFaultOptSel,removeCB), true );
	removebut_->attach( alignedBelow, addbut );

	for ( int idx=0; idx<fltpar_.selfaultids_.size(); idx++ )
	{
	    PtrMan<IOObj> ioobj = IOM().get( fltpar_.selfaultids_[idx] );
	    addObjEntry( idx, *ioobj, fltpar_.optids_[idx] );
	}
    }

    void addCB( CallBacker* )
    {
	PtrMan<CtxtIOObj> objio = fltpar_.is2d_ ? mMkCtxtIOObj(EMFaultStickSet)
						: mMkCtxtIOObj(EMFault3D);
	uiIOObjSelDlg dlg( this, *objio, 0, true );
	if ( !dlg.go() )
	    return;

	const int nrsel = dlg.nrChosen();
	for ( int idx=0; idx<nrsel; idx++ )
	{
	    const MultiID& mid = dlg.chosenID( idx );
	    PtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( !ioobj ) continue;

	    addObjEntry( fltpar_.selfaultids_.size(), *ioobj,
		    fltpar_.defaultoptidx_ );
	}

	removebut_->setSensitive( fltpar_.selfaultids_.size() );
    }

    void addObjEntry( int row, const IOObj& ioobj, int optidx )
    {
	if ( row==table_->nrRows() )
	    table_->insertRows( row, 1 );

	uiComboBox* actopts = new uiComboBox( 0, fltpar_.optnms_, 0 );
	actopts->selectionChanged.notify( mCB(this,uiFaultOptSel,optCB) );
	actopts->setCurrentItem( optidx );
	table_->setCellObject( RowCol(row,1), actopts );

	const char * fltnm = ioobj.name();
	table_->setText( RowCol(row,0), fltnm );
	table_->setCellReadOnly( RowCol(row,0), true );

	if ( fltpar_.selfaultnms_.isPresent(fltnm) )
	    return;

	fltpar_.selfaultids_ += ioobj.key();
	fltpar_.selfaultnms_.add( fltnm );
	fltpar_.optids_ += optidx;
    }

    void removeCB( CallBacker* )
    {
	const int currow = table_->currentRow();
	if ( currow==-1 ) return;

	if ( currow<fltpar_.selfaultids_.size() )
	{
	    fltpar_.selfaultids_.removeSingle( currow );
	    fltpar_.selfaultnms_.removeSingle( currow );
	    fltpar_.optids_.removeSingle( currow );
	}

	table_->removeRow( currow );
	removebut_->setSensitive( fltpar_.selfaultids_.size() );
    }

    void optCB( CallBacker* cb )
    {
	for ( int idx=0; idx<fltpar_.optids_.size(); idx++ )
	{
	    mDynamicCastGet(uiComboBox*, selbox,
		    table_->getCellObject(RowCol(idx,1)) );
	    if ( selbox==cb )
	    {
		fltpar_.optids_[idx] = selbox->currentItem();
		return;
	    }
	}
    }

    uiFaultParSel&	fltpar_;
    uiTable*		table_;
    uiPushButton*	removebut_;
};



uiFaultParSel::uiFaultParSel( uiParent* p, bool is2d, bool useoptions )
    : uiCompoundParSel(p,"Faults","Select")
    , is2d_(is2d)
    , selChange(this)
    , useoptions_(useoptions)
    , defaultoptidx_(0)
{
    butPush.notify( mCB(this,uiFaultParSel,doDlg) );
    uiPushButton* clearbut = new uiPushButton( this, "Clear", true );
    clearbut->activated.notify( mCB(this,uiFaultParSel,clearPush) );
    clearbut->attach( rightOf, selbut_ );
    txtfld_->setElemSzPol( uiObject::Wide );
}


void uiFaultParSel::setSelectedFaults( const TypeSet<MultiID>& ids,
				       const TypeSet<FaultTrace::Act>* act )
{
    selfaultids_.erase();
    selfaultnms_.erase();
    optids_.erase();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ids[idx] );
	if ( !ioobj ) continue;

	selfaultnms_.add( ioobj->name() );
	selfaultids_ += ids[idx];
	optids_ += act && act->validIdx(idx) ? (*act)[idx]
					     : FaultTrace::AllowCrossing;
    }
    updSummary(0);
    selChange.trigger();
}


void uiFaultParSel::clearPush( CallBacker* )
{
    selfaultnms_.erase();
    selfaultids_.erase();
    optids_.erase();

    updSummary(0);
    selChange.trigger();
}


void uiFaultParSel::setGeomIDs( const TypeSet<Pos::GeomID>& geomids )
{
    geomids_.erase();
    geomids_ = geomids;
}


void uiFaultParSel::doDlg( CallBacker* )
{
    if ( useoptions_ && !optnms_.isEmpty() )
    {
	uiFaultOptSel dlg( this, *this );
	if ( !dlg.go() ) return;
    }
    else if ( is2d_ && geomids_.size() )
    {
	uiFSS2DLineSelDlg dlg( this, geomids_ );
	dlg.setSelectedItems( selfaultnms_ );
	if ( !dlg.go() ) return;

	selfaultnms_.erase();
	selfaultids_.erase();
	dlg.getSelected( selfaultnms_, selfaultids_ );
    }
    else
    {
	PtrMan<CtxtIOObj> ctio = is2d_ ? mMkCtxtIOObj(EMFaultStickSet)
				       : mMkCtxtIOObj(EMFault3D);
	uiIOObjSelDlg dlg( this, *ctio, "Select Faults", true );
	dlg.selGrp()->getListField()->setChosen( selfaultnms_ );
	if ( !dlg.go() ) return;

	selfaultnms_.erase(); selfaultids_.erase();
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
    for ( int idx=0; idx<selfaultnms_.size(); idx++ )
    {
	summ += selfaultnms_.get(idx);
	if ( addopt )
	{
	    summ += " (";
	    summ += optnms_[optids_[idx]]->buf();
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




