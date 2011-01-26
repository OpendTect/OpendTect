/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribsetbuild.cc,v 1.10 2011-01-26 12:29:14 cvshelene Exp $";

#include "uiattribsetbuild.h"
#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattribsingleedit.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribprocessor.h"
#include "linekey.h"
#include "survinfo.h"
#include "ioobj.h"


uiAttribDescSetBuild::Setup::Setup( bool for2d )
    : is2d_(for2d)
    , showps_(true)
    , singletraceonly_(true)
    , showusingtrcpos_(true)
    , showdepthonlyattrs_(!SI().zIsTime())
    , showtimeonlyattrs_(SI().zIsTime())
    , showhidden_(false)
    , showsteering_(false)
{
}


uiAttribDescSetBuild::uiAttribDescSetBuild( uiParent* p,
			const uiAttribDescSetBuild::Setup& su )
    : uiGroup(p,"DescSet build group")
    , descset_(*new Attrib::DescSet(su.is2d_))
    , setup_(su)
    , usrchg_(false)
    , ctio_(*mMkCtxtIOObj(AttribDescSet))
{
    availattrfld_ = new uiListBox( this, "Available attributes" );
    fillAvailAttrFld();

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
		    "Add attribute", mCB(this,uiAttribDescSetBuild,addReq) );
    addbut->attach( centeredRightOf, availattrfld_ );

    defattrfld_ = new uiListBox( this, "Defined attributes" );
    defattrfld_->attach( rightTo, availattrfld_ );
    defattrfld_->attach( ensureRightOf, addbut );
    defattrfld_->selectionChanged.notify(
	    		mCB(this,uiAttribDescSetBuild,defSelChg) );
    defattrfld_->doubleClicked.notify( mCB(this,uiAttribDescSetBuild,edReq) );

    edbut_ = new uiToolButton( this, "edit.png",
		    "Edit attribute", mCB(this,uiAttribDescSetBuild,edReq) );
    edbut_->attach( rightOf, defattrfld_ );
    rmbut_ = new uiToolButton( this, "trashcan.png",
		    "Remove attribute", mCB(this,uiAttribDescSetBuild,rmReq) );
    rmbut_->attach( alignedBelow, edbut_ );
    uiToolButton* openbut = new uiToolButton( this, "openset.png",
		    "Open stored attribute set",
		    mCB(this,uiAttribDescSetBuild,openReq) );
    openbut->attach( alignedBelow, rmbut_ );
    savebut_ = new uiToolButton( this, "save.png",
		    "Save attribute set",
		    mCB(this,uiAttribDescSetBuild,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    setHAlignObj( defattrfld_ );
    defSelChg();
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete ctio_.ioobj;
    delete &descset_;
    delete &ctio_;
}


void uiAttribDescSetBuild::defSelChg( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    const bool havesel = selidx >= 0;
    const char* attrnm = havesel ? defattrfld_->textOfItem(selidx) : "";
    const Attrib::DescID descid = descset_.getID( attrnm, true );

    edbut_->setSensitive( havesel );
    savebut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel && !descset_.isAttribUsed(descid) );

    if ( havesel )
    {
	const Attrib::Desc& desc = *descset_.getDesc( descid );
	const int fldidx = availattrnms_.indexOf( desc.attribName() );
	availattrfld_->setCurrentItem( fldidx );
    }
}


void uiAttribDescSetBuild::fillAvailAttrFld()
{
    BufferStringSet dispnms;
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	const uiAttrDescEd::DomainType domtyp
	    	= (uiAttrDescEd::DomainType)uiAF().domainType(idx);
	if ( !setup_.showdepthonlyattrs_ && domtyp == uiAttrDescEd::Depth )
	    continue;
	if ( !setup_.showtimeonlyattrs_ && domtyp == uiAttrDescEd::Time )
	    continue;
	const uiAttrDescEd::DimensionType dimtyp
	    	= (uiAttrDescEd::DimensionType)uiAF().dimensionType(idx);
	if ( setup_.is2d_ && dimtyp == uiAttrDescEd::Only3D )
	    continue;
	if ( !setup_.is2d_ && dimtyp == uiAttrDescEd::Only2D )
	    continue;

	const char* attrnm = uiAF().getAttribName( idx );
	const Attrib::Desc* desc = Attrib::PF().getDesc( attrnm );
	if ( !desc )
	    { pErrMsg("attrib in uiAF() but not in PF()"); continue; }
	if ( setup_.singletraceonly_
			&& desc->locality()==Attrib::Desc::MultiTrace )
	    continue;
	if ( !setup_.showps_ && desc->isPS() )
	    continue;
	if ( !setup_.showusingtrcpos_ && desc->usesTracePosition() )
	    continue;
	if ( !setup_.showhidden_ && desc->isHidden() )
	    continue;
	if ( !setup_.showsteering_ && desc->isSteering() )
	    continue;

	availattrnms_.add( attrnm );
	dispnms.add( uiAF().getDisplayName(idx) );
    }

    int* idxs = dispnms.getSortIndexes();
    dispnms.useIndexes( idxs );
    availattrnms_.useIndexes( idxs );
    availattrfld_->addItems( dispnms );
    availattrfld_->doubleClicked.notify( mCB(this,uiAttribDescSetBuild,addReq));
}


void uiAttribDescSetBuild::fillDefAttribFld()
{
    const BufferString prevsel( defattrfld_->getText() );
    defattrfld_->setEmpty();

    const int sz = descset_.nrDescs();
    for ( int idx=0; idx<sz; idx++ )
    {
	const Attrib::Desc& desc = *descset_.desc( idx );
	if ( !desc.isStored() && (!desc.isHidden() || setup_.showhidden_) )
	    defattrfld_->addItem( desc.userRef() );
    }

    if ( defattrfld_->isPresent(prevsel) )
	defattrfld_->setCurrentItem( prevsel );
    else if ( !defattrfld_->isEmpty() )
	defattrfld_->setCurrentItem( 0 );
}


bool uiAttribDescSetBuild::doAttrEd( Attrib::Desc& desc, bool isnew )
{
    uiSingleAttribEd dlg( this, desc, isnew );
    dlg.setDataPackSelection( dpfids_ );
    if ( !dlg.go() )
	return false;

    if ( dlg.anyChange() )
    {
	fillDefAttribFld();
	usrchg_ = true;
	return true;
    }
    return false;
}


void uiAttribDescSetBuild::addReq( CallBacker* )
{
    const int selidx = availattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = availattrnms_.get( selidx );
    Attrib::Desc* desc = PF().createDescCopy( attrnm );
    desc->setUserRef( "" );
    desc->setDescSet( &descset_ );
    descset_.addDesc( desc );
    const Attrib::DescID did = desc->id();

    if ( !doAttrEd(*desc,true) )
	descset_.removeDesc( did );

//test
    BufferString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan( did );

    DataPointSet* dps = descset_.createDataPointSet(
					    Attrib::DescSetup().hidden(false) );
    PtrMan<Processor> proc = descset_.is2D() ?
			    aem->createScreenOutput2D( errmsg, *d2dh )
			    : aem->createDataCubesOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return;
    }

    proc->setName( "computing attributes on DataPacks" );
    uiTaskRunner dlg( this );
    dlg.execute(*proc);
}


void uiAttribDescSetBuild::edReq( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = defattrfld_->textOfItem( selidx );
    const Attrib::DescID id = descset_.getID( attrnm, true );
    doAttrEd( *descset_.getDesc(id), false );
}


void uiAttribDescSetBuild::rmReq( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = defattrfld_->textOfItem( selidx );
    descset_.removeDesc( descset_.getID(attrnm,true) );
    usrchg_ = true;

    int newselidx = selidx;
    if ( newselidx >= defattrfld_->size()-1 )
	newselidx--;
    if ( newselidx >= 0 )
	defattrfld_->setCurrentItem( newselidx );
    fillDefAttribFld();
}


void uiAttribDescSetBuild::openReq( CallBacker* )
{
    if ( usrchg_ && !uiMSG().askGoOn("Current work not saved. Continue?") )
	return;

    if ( doAttrSetIO(true) )
	fillDefAttribFld();
}


void uiAttribDescSetBuild::saveReq( CallBacker* )
{
    doAttrSetIO( false );
}


void uiAttribDescSetBuild::setDataPackInp( const TypeSet<DataPack::FullID>& ids)
{
    dpfids_ = ids;
}


bool uiAttribDescSetBuild::doAttrSetIO( bool forread )
{
    ctio_.ctxt.forread = forread;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    BufferString emsg;
    const bool res = forread
	? AttribDescSetTranslator::retrieve(descset_,dlg.ioObj(),emsg)
	: AttribDescSetTranslator::store(descset_,dlg.ioObj(),emsg);
    if ( res )
	usrchg_ = false;
    else
	uiMSG().error( emsg );

    return res;
}


//for testing purpose

Attrib::EngineMan* uiAttribDescSetBuild::createEngineMan(
						const Attrib::DescID& did )
{
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    TypeSet<Attrib::SelSpec> attribspecs;
    Attrib::SelSpec sp( 0, did );
    attribspecs += sp;

    CubeSampling cs;
    LineKey lk;
    aem->setAttribSet( &descset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setLineKey( lk );
    aem->setCubeSampling( cs );
    return aem;
}
