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
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "embodytr.h"
#include "emfaultstickset.h"
#include "emsurface.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "randcolor.h"
#include "survinfo.h"

const int cListHeight = 5;

uiIOSurface::uiIOSurface( uiParent* p, bool forread, const char* typ )
    : uiGroup(p,"Surface selection")
    , ctio_( 0 )
    , sectionfld_(0)
    , attribfld_(0)
    , rgfld_(0)
    , attrSelChange(this)
    , forread_(forread)
    , objfld_(0)
{
    if ( !strcmp(typ,EMHorizon2DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMHorizon2D);
    else if (!strcmp(typ,EMHorizon3DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMHorizon3D);
    else if ( !strcmp(typ,EMFaultStickSetTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMFaultStickSet);
    else if ( !strcmp(typ,EMFault3DTranslatorGroup::keyword()) )
	ctio_ = mMkCtxtIOObj(EMFault3D);
    else
	ctio_ = new CtxtIOObj( polygonEMBodyTranslator::getIOObjContext() );

    postFinalise().notify( mCB(this,uiIOSurface,objSel) );
}


uiIOSurface::~uiIOSurface()
{
    delete ctio_->ioobj; delete ctio_;
}


void uiIOSurface::mkAttribFld()
{
    attribfld_ = new uiLabeledListBox( this, "Calculated attributes", true,
				      uiLabeledListBox::AboveMid );
    attribfld_->setStretch( 1, 1 );
    attribfld_->box()->selectionChanged.notify( mCB(this,uiIOSurface,attrSel) );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    sectionfld_ = new uiLabeledListBox( this, "Available patches", true,
				     labelabove ? uiLabeledListBox::AboveMid 
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


void uiIOSurface::mkObjFld( const char* lbl )
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
	const char* res = oi.getSurfaceData( sd );
	if ( res )
	{
	    if ( showerrmsg )
		uiMSG().error( res );
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
		uiMSG().error( "Surface not loaded" );
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
    if ( !attribfld_ )
	return;

    attribfld_->box()->getSelectedItems( names );
}


void uiIOSurface::setSelAttributes( const BufferStringSet& attribnames ) const
{
    if ( !attribfld_)
	return;

    if ( attribnames.size() == 0 )
	attribfld_->box()->setCurrentItem( -1 );

    attribfld_->box()->setSelectedItems( attribnames );
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
    sectionfld_->box()->selectAll( true );
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
    for ( int idx=0; idx<attribfld_->box()->size(); idx++ )
    {
	if ( attribfld_->box()->isSelected(idx) )
	    return true;
    }
    return false;
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels ) const
{
    if ( !rgfld_ || rgfld_->isAll() )
	sels.rg.init( false );
    else
	sels.rg = rgfld_->envelope().hrg;

    if ( SI().sampling(true) != SI().sampling(false) )
    {
	if ( sels.rg.isEmpty() )
	    sels.rg.init( true );
	sels.rg.limitTo( SI().sampling(true).hrg );
    }
	
    sels.selsections.erase();
    int nrsections = sectionfld_ ? sectionfld_->box()->size() : 1;
    for ( int idx=0; idx<nrsections; idx++ )
    {
	if ( nrsections == 1 || sectionfld_->box()->isSelected(idx) )
	    sels.selsections += idx;
    }

    sels.selvalues.erase();
    int nrattribs = attribfld_ ? attribfld_->box()->size() : 0;
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	if ( attribfld_->box()->isSelected(idx) )
	    sels.selvalues += idx;
    }
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
	mkObjFld( "Output Stickset" );
    else
	mkObjFld( BufferString("Output ",setup.typ_) );

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
    if ( sectionfld_ && !sectionfld_->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

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
		sectionfld_->box()->size()!=sectionfld_->box()->nrSelected();

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
	mkAttribFld();
	attribfld_->attach( alignedBelow, objfld_ );
	sectionfld_->attach( rightTo, attribfld_ );
	attachobj = attribfld_;
	attribfld_->box()->setMultiSelect( setup.multiattribsel_ );
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
    {
	uiMSG().error( "Please select input" );
	return false;
    }

    if ( sectionfld_ && !sectionfld_->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    return true;
}


// uiFaultParSel
class uiFSS2DLineSelDlg : public uiDialog
{
public:
    uiFSS2DLineSelDlg( uiParent* p, const TypeSet<PosInfo::GeomID>& geoids )    
    	: uiDialog(p,uiDialog::Setup("FaultStickSet selection",
		    "Available for 2D lines",mNoHelpID))
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(EMFaultStickSet);
	IOM().to( ctio->ctxt.getSelKey() );
	IODirEntryList entlst( IOM().dirPtr(), ctio->ctxt );

	for ( int idx=0; idx<entlst.size(); idx++ )
	{
	    const IOObj* obj = entlst[idx]->ioobj;
	    if ( !obj ) continue;

	    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded(obj->key());
	    mDynamicCastGet(EM::FaultStickSet*,fss,emobj);
	    if ( !fss ) continue;

	    EM::SectionID sid = fss->sectionID(0);
	    const int nrsticks = fss->geometry().nrSticks( sid );
	
	    bool fssvalid = false;
	    for ( int gidx=0; gidx<geoids.size(); gidx++ )
	    {
		if ( fssvalid ) break;

    		S2DPOS().setCurLineSet(geoids[gidx].lsid_);
    		PosInfo::Line2DData linegeom;
    		if ( !S2DPOS().getGeometry(geoids[gidx].lineid_,linegeom) )
    		    return;

		for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
		{
		    const Geometry::FaultStickSet* fltgeom =
			fss->geometry().sectionGeometry( sid );
		    if ( !fltgeom ) continue;
		    
		    const int sticknr = fltgeom->rowRange().atIndex( stickidx );
		    if ( !fss->geometry().pickedOn2DLine(sid, sticknr) )
			continue;
	    
		    const MultiID* lsid =
			fss->geometry().pickedMultiID(sid,sticknr);
		    if ( !lsid ) continue;
		    
		    PtrMan<IOObj> lsobj = IOM().get( *lsid );
		    if ( !lsobj ) continue;
		    
		    const char* lnnm = fss->geometry().pickedName(sid,sticknr);
		    if ( !lnnm ) continue;

		    if ( geoids[gidx]==S2DPOS().getGeomID(lsobj->name(),lnnm) )
		    {
			fssvalid = true;
			break;
		    }
		}
	    }

	    if ( fssvalid )
	    {
		validfss_.add( fss->name() );
		validmids_ += obj->key();
	    }
	}

	fsslistfld_ = new uiListBox(this,"",true,validmids_.size()+1,20);
	fsslistfld_->addItems( validfss_ );
    }

    void getSelected( BufferStringSet& nms, TypeSet<MultiID>& mids ) 
    {
	TypeSet<int> selids;
	fsslistfld_->getSelectedItems( selids );
	for ( int idx=0; idx<selids.size(); idx++ )
	{
	    nms.add( *validfss_[selids[idx]] );
	    mids += validmids_[selids[idx]];
	}
    }

    void setSelectedItems( BufferStringSet sel )
    { fsslistfld_->setSelectedItems(sel); }


    uiListBox*		fsslistfld_;
    BufferStringSet	validfss_;
    TypeSet<MultiID>	validmids_;
};


uiFaultParSel::uiFaultParSel( uiParent* p, bool is2d )
    : uiCompoundParSel(p,"Faults","Select")
    , is2d_(is2d)
    , selChange(this)
{
    butPush.notify( mCB(this,uiFaultParSel,doDlg) );
    uiPushButton* clearbut = new uiPushButton( this, "Clear", true );
    clearbut->activated.notify( mCB(this,uiFaultParSel,clearPush) );
    clearbut->attach( rightOf, selbut_ );
}


void uiFaultParSel::setSelectedFaults( const TypeSet<MultiID>& ids )
{
    selfaultids_.erase();
    selfaultnms_.erase();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ids[idx] );
	if ( !ioobj ) continue;

	selfaultnms_.add( ioobj->name() );
	selfaultids_ += ids[idx];
    }
    updSummary(0);
    selChange.trigger();
}


void uiFaultParSel::clearPush( CallBacker* )
{
    selfaultnms_.erase();
    selfaultids_.erase();
    updSummary(0);
    selChange.trigger();
}


void uiFaultParSel::set2DGeomIds( const TypeSet<PosInfo::GeomID>& nids )
{
    geomids_.erase();
    geomids_ = nids;
}


void uiFaultParSel::doDlg( CallBacker* )
{
    if ( is2d_ && geomids_.size() )
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
	dlg.selGrp()->getListField()->setSelectedItems( selfaultnms_ );
	if ( !dlg.go() ) return;

	selfaultnms_.erase();
	selfaultids_.erase();
	uiIOObjSelGrp* selgrp = dlg.selGrp();
	selgrp->processInput();
	selgrp->getListField()->getSelectedItems( selfaultnms_ );
	for ( int idx=0; idx<selfaultnms_.size(); idx++ )
	    selfaultids_ += selgrp->selected(idx);
    }

    selChange.trigger();
}


BufferString uiFaultParSel::getSummary() const
{
    BufferString summ;
    for ( int idx=0; idx<selfaultnms_.size(); idx++ )
    {
	summ += selfaultnms_.get(idx);
	summ += idx == selfaultnms_.size()-1 ? "." : ", ";
    }
    return summ.isEmpty() ? BufferString(" - ") : summ;
}
