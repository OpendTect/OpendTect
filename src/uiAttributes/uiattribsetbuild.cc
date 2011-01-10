/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribsetbuild.cc,v 1.3 2011-01-10 13:30:13 cvsbert Exp $";

#include "uiattribsetbuild.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uiattrdesced.h"
#include "uiattribsingleedit.h"
#include "uiattribfactory.h"
#include "uimsg.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "survinfo.h"


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
	    		mCB(this,uiAttribDescSetBuild,updButStates) );
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

    updButStates();
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete &descset_;
}


void uiAttribDescSetBuild::updButStates( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    const bool havesel = selidx >= 0;
    edbut_->setSensitive( havesel );
    savebut_->setSensitive( havesel );
    bool rmsens = havesel;
    if ( rmsens )
    {
	const char* attrnm = defattrfld_->textOfItem( selidx );
	const Attrib::DescID id = descset_.getID( attrnm, true );
	if ( descset_.isAttribUsed(id) )
	    rmsens = false;
    }
    rmbut_->setSensitive( rmsens );
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

    updButStates();
}


void uiAttribDescSetBuild::setDataPackInp( const DataPack::FullID& fdpid )
{
    //TODO
}


bool uiAttribDescSetBuild::doAttrEd( Attrib::Desc& desc, bool isnew )
{
    uiSingleAttribEd dlg( this, desc, false );
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
    descset_.addDesc( desc );

    if ( !doAttrEd(*desc,false) )
	descset_.removeDesc( desc->id() );
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

    uiMSG().error( "TODO: open attr set" );
    // usrchg_ = false;
}


void uiAttribDescSetBuild::saveReq( CallBacker* )
{
    uiMSG().error( "TODO: save attr set" );
    // usrchg_ = false;
}
