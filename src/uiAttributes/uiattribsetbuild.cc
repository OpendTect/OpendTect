/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribsetbuild.cc,v 1.2 2011-01-06 16:19:09 cvsbert Exp $";

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
{
    availattrfld_ = new uiListBox( this, "Available attributes" );
    fillAvailAttrFld();

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
		    "Add attribute", mCB(this,uiAttribDescSetBuild,addReq) );
    addbut->attach( centeredRightOf, availattrfld_ );

    defattrfld_ = new uiListBox( this, "Defined attributes" );
    defattrfld_->attach( rightTo, availattrfld_ );
    defattrfld_->attach( ensureRightOf, addbut );
    defattrfld_->doubleClicked.notify( mCB(this,uiAttribDescSetBuild,edReq) );
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete &descset_;
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


void uiAttribDescSetBuild::addReq( CallBacker* )
{
    const int selidx = availattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = availattrnms_.get( selidx );
    uiMSG().error( "TODO: add requested for ", attrnm );
}


void uiAttribDescSetBuild::edReq( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = defattrfld_->textOfItem( selidx );
    const Attrib::DescID id = descset_.getID( attrnm, true );
    Attrib::Desc& desc = *descset_.getDesc( id );
    uiMSG().error( attrnm, ": TODO edit for type ", desc.attribName() );

    uiSingleAttribEd dlg( this, desc, false );
    if ( dlg.go() )
    {
	if ( dlg.anyChange() )
	    fillDefAttribFld();
    }
}


void uiAttribDescSetBuild::fillDefAttribFld()
{
    //TODO
}


void uiAttribDescSetBuild::rmReq( CallBacker* )
{
    const int selidx = defattrfld_->currentItem();
    if ( selidx < 0 ) return;

    const char* attrnm = defattrfld_->textOfItem( selidx );
    uiMSG().error( attrnm, ": TODO remove" );
}
