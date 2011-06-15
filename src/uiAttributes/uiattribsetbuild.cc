/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribsetbuild.cc,v 1.15 2011-06-15 09:04:16 cvsbert Exp $";

#include "uiattribsetbuild.h"
#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattribsingleedit.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribstorprovider.h"
#include "attribparambase.h"
#include "attribfactory.h"
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
    : uiBuildListFromList(p,uiBuildListFromList::Setup(false,"attribute",false),
	    		  "DescSet build group")
    , descset_(*new Attrib::DescSet(su.is2d_))
    , attrsetup_(su)
    , ctio_(*mMkCtxtIOObj(AttribDescSet))
{
    fillAvailable();

    uiToolButton* openbut = new uiToolButton( this, "openset.png",
		    "Open stored attribute set",
		    mCB(this,uiAttribDescSetBuild,openReq) );
    openbut->attach( alignedBelow, lowestStdBut() );
    savebut_ = new uiToolButton( this, "save.png",
		    "Save attribute set",
		    mCB(this,uiAttribDescSetBuild,saveReq) );
    savebut_->attach( alignedBelow, openbut );

    descset_.setCouldBeUsedInAnyDimension( true );
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete ctio_.ioobj;
    delete &descset_;
    delete &ctio_;
}


void uiAttribDescSetBuild::defSelChg( CallBacker* cb )
{
    uiBuildListFromList::defSelChg( cb );

    const char* attrnm = curDefSel();
    const bool havesel = attrnm && *attrnm;
    savebut_->setSensitive( havesel );
    if ( !havesel ) return;

    const Attrib::DescID descid = descset_.getID( attrnm, true );
    rmbut_->setSensitive( descid.isValid() && !descset_.isAttribUsed(descid) );
}


void uiAttribDescSetBuild::fillAvailable()
{
    BufferStringSet dispnms;
    for ( int idx=0; idx<uiAF().size(); idx++ )
    {
	const uiAttrDescEd::DomainType domtyp
	    	= (uiAttrDescEd::DomainType)uiAF().domainType(idx);
	if ( !attrsetup_.showdepthonlyattrs_ && domtyp == uiAttrDescEd::Depth )
	    continue;
	if ( !attrsetup_.showtimeonlyattrs_ && domtyp == uiAttrDescEd::Time )
	    continue;
	const uiAttrDescEd::DimensionType dimtyp
	    	= (uiAttrDescEd::DimensionType)uiAF().dimensionType(idx);
	if ( attrsetup_.is2d_ && dimtyp == uiAttrDescEd::Only3D )
	    continue;
	if ( !attrsetup_.is2d_ && dimtyp == uiAttrDescEd::Only2D )
	    continue;

	const char* attrnm = uiAF().getAttribName( idx );
	const Attrib::Desc* desc = Attrib::PF().getDesc( attrnm );
	if ( !desc )
	    { pErrMsg("attrib in uiAF() but not in PF()"); continue; }
	if ( attrsetup_.singletraceonly_
			&& desc->locality()==Attrib::Desc::MultiTrace )
	    continue;
	if ( !attrsetup_.showps_ && desc->isPS() )
	    continue;
	if ( !attrsetup_.showusingtrcpos_ && desc->usesTracePosition() )
	    continue;
	if ( !attrsetup_.showhidden_ && desc->isHidden() )
	    continue;
	if ( !attrsetup_.showsteering_ && desc->isSteering() )
	    continue;

	availattrnms_.add( attrnm );
	dispnms.add( uiAF().getDisplayName(idx) );
    }

    int* idxs = dispnms.getSortIndexes();
    dispnms.useIndexes( idxs );
    availattrnms_.useIndexes( idxs );
    setAvailable( dispnms );
}


void uiAttribDescSetBuild::editReq( bool isadd )
{
    const char* attrnm = isadd ? curAvSel() : curDefSel();
    if ( !attrnm || !*attrnm ) return;

    Attrib::DescID did;
    if ( !isadd )
	did = descset_.getID( attrnm, true );
    else
    {
	attrnm = uiAF().attrNameOf( attrnm );
	Attrib::Desc* desc = PF().createDescCopy( attrnm );
	if ( !desc ) { pErrMsg("Huh"); return; }
	desc->setUserRef( "" );
	desc->setDescSet( &descset_ );
	descset_.addDesc( desc );
	did = desc->id();
    }

    Attrib::Desc& desc = *descset_.getDesc( did );
    uiSingleAttribEd dlg( this, desc, isadd );
    dlg.setDataPackSelection( dpfids_ );
    if ( !dlg.go() )
    {
	if ( isadd )
	    descset_.removeDesc( did );
    }
    else
    {
	const char* descnm = desc.userRef();
	if ( isadd )
	    addItem( descnm );
	else
	    setItemName( descnm );
    }
}


void uiAttribDescSetBuild::removeReq()
{
    const char* attrnm = curDefSel();
    if ( attrnm && *attrnm )
    {
	descset_.removeDesc( descset_.getID(attrnm,true) );
	removeItem();
    }
}


const char* uiAttribDescSetBuild::avFromDef( const char* attrnm ) const
{
    Attrib::DescID did( descset_.getID(attrnm,true) );
    if ( !did.isValid() ) return 0;
    const char* clssnm = descset_.getDesc(did)->attribName();
    return uiAF().dispNameOf( clssnm );
}


void uiAttribDescSetBuild::openReq( CallBacker* )
{
    if ( usrchg_ && !uiMSG().askGoOn("Current work not saved. Continue?") )
	return;

    if ( doAttrSetIO(true) )
    {
	const BufferString prevsel( curDefSel() );
	removeAll();

	const int sz = descset_.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const Attrib::Desc& desc = *descset_.desc( idx );
	    if (  !desc.isStored()
	      && (!desc.isHidden() || attrsetup_.showhidden_) )
		addItem( desc.userRef() );
	}

	setCurDefSel( prevsel.isEmpty() ? 0 : prevsel.buf() );
    }
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

    if ( forread && !dpfids_.isEmpty() )
    {
	for ( int iattr=0; iattr<descset_.size(); iattr++ )
	{
	    Attrib::Desc& desc = *descset_.desc( iattr );
	    if ( !desc.isStoredInMem() ) continue;

	    Attrib::ValParam* vp = desc.getValParam(
		    		Attrib::StorageProvider::keyStr() );
	    const MultiID descid( vp->getStringValue(0) + 1 );
	    if ( dpfids_.indexOf(descid) < 0 )
	    {
		BufferString fidstr = "#";
		fidstr += dpfids_[0];
		vp->setValue( fidstr.buf() );
	    }
	}
    }

    return res;
}
