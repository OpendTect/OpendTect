/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattribsetbuild.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribfactory.h"
#include "attribparambase.h"
#include "attribstorprovider.h"
#include "ioobj.h"
#include "keystrs.h"
#include "survinfo.h"

#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattribsingleedit.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uiprestackattrib.h"
#include "uistoredattrreplacer.h"
#include "uitoolbutton.h"


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


uiAttribDescSetBuild::Setup::~Setup()
{}



uiAttribDescSetBuild::uiAttribDescSetBuild( uiParent* p,
			const uiAttribDescSetBuild::Setup& su )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"type","attribute")
	    .withtitles(true), "DescSet build group")
    , descset_(*new Attrib::DescSet(su.is2d_))
    , attrsetup_(su)
    , uipsattrdesced_(0)
    , anychg_(false)
{
    descset_.setCouldBeUsedInAnyDimension( true );
    fillAvailable();
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete &descset_;
}


void uiAttribDescSetBuild::defSelChg()
{
    uiBuildListFromList::defSelChg();

    const char* attrnm = curDefSel();
    const bool havesel = attrnm && *attrnm;
    if ( !havesel ) return;

    const Attrib::DescID descid = descset_.getID( attrnm, true );
    rmbut_->setSensitive( descid.isValid() && !descset_.isAttribUsed(descid) );
}


bool uiAttribDescSetBuild::handleUnsaved()
{
    if ( !anychg_ && !usrchg_ ) return true;

    const int res = uiMSG().question(tr("Seismic Attribute Set not saved."
					"\n\nDo you want to save it now?"),
				     tr("Yes (store)"), tr("No (discard)"),
	uiStrings::sCancel());
    if ( res == 0 ) return true;
    if ( res == -1 ) return false;

    return ioReq(true);
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

	dispnms.add( uiAF().getDisplayName(idx) );
    }

    dispnms.sort();
    setAvailable( dispnms );
}


void uiAttribDescSetBuild::editReq( bool isadd )
{
    const char* attrnm = isadd ? curAvSel() : curDefSel();
    if ( !attrnm || !*attrnm )
	return;

    Attrib::DescID did;
    if ( !isadd )
	did = descset_.getID( attrnm, true );
    else
    {
	attrnm = uiAF().attrNameOf( attrnm );
	RefMan<Attrib::Desc> desc = PF().createDescCopy( attrnm );
	if ( !desc )
	    { pErrMsg("Huh"); return; }

	desc->setUserRef( "" );
	desc->setDescSet( &descset_ );
	descset_.addDesc( desc );
	did = desc->id();
    }

    Attrib::Desc& desc = *descset_.getDesc( did );
    uiSingleAttribEd dlg( this, desc, isadd );
    bool success = false;
    if ( desc.isPS() )
    {
	dlg.setDataPackSelection( psdpfids_ );
	success = dlg.go();
	if ( success )
	{
	    for ( int idx=descset_.size()-1; idx>=0; idx-- )
	    {
		Desc* tmpdesc = descset_.desc(idx);
		if ( tmpdesc->isStoredInMem() )
		{
		    MultiID mid;
		    mGetMultiIDFromDesc( (*tmpdesc), mid,
					Attrib::StorageProvider::keyStr() )
		    if ( psdpfids_.isPresent( mid ) )
			descset_.removeDesc( tmpdesc->id() );
		}
	    }
	}
    }
    else
    {
	dlg.setDataPackSelection( dpfids_ );
	success = dlg.go();
    }

    if ( success )
    {
	handleSuccessfullEdit( isadd, desc.userRef() );
	anychg_ = true;
    }
    else if ( isadd )
	descset_.removeDesc( did );

}


void uiAttribDescSetBuild::removeReq()
{
    const char* attrnm = curDefSel();
    if ( attrnm && *attrnm )
    {
	anychg_ = true;
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


void uiAttribDescSetBuild::setDataPackInp( const TypeSet<DataPack::FullID>& ids,
						bool isprestack )
{
    ( isprestack ? psdpfids_ : dpfids_ ) = ids;
}


bool uiAttribDescSetBuild::ioReq( bool forsave )
{
    bool res = doAttrSetIO( !forsave );
    if ( !res )
	return false;

    if ( !forsave )
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

    anychg_ = false;
    return true;
}


bool uiAttribDescSetBuild::doAttrSetIO( bool forread )
{
    IOObjContext ctxt = mIOObjContext( AttribDescSet );
    ctxt.forread_ = forread;
    uiIOObjSelDlg dlg( this, ctxt );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    const bool is2d = attrsetup_.is2d_;

    uiString emsg;
    Attrib::DescSet descset( is2d );
    bool res = forread
	? AttribDescSetTranslator::retrieve(descset,dlg.ioObj(),emsg)
	: AttribDescSetTranslator::store(descset_,dlg.ioObj(),emsg);

    const bool isdesc2d = descset.is2D();
    const bool isdescanyd = descset.couldBeUsedInAnyDimension();

    //TODO make a 2D/3D AttribSet converter

    if ( forread )
    {
	const bool badmatch = (!isdesc2d && is2d) || (isdesc2d && !is2d);
	if ( res && badmatch && !isdescanyd )
	{
	    emsg = tr("Can not load Attribute Set:\n"
		      "Attribute Set is %1. Current definition is %2")
		 .arg(isdesc2d ? uiStrings::s2D() : uiStrings::s3D())
		 .arg(is2d ? uiStrings::s2D() : uiStrings::s3D());
	    res = false;
	}
	else if ( res )
	{
	    if ( isdescanyd )
	    {
		IOPar par; descset.fillPar( par );
		par.set( sKey::Type(), is2d ? "2D" : "3D" );
		descset.usePar( par );
	    }
	    descset_ = descset;
	}
    }


    if ( !res )
    {
	uiMSG().error( emsg );
	return false;
    }

    usrchg_ = false;
    if ( forread && ( !dpfids_.isEmpty() || !psdpfids_.isEmpty() ) )
    {
	uiStoredAttribReplacer replacerdlg( this, &descset_ );
	replacerdlg.setDataPackIDs( dpfids_ );
	replacerdlg.go();
    }

    return res;
}
