/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattribsetbuild.h"
#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattribsingleedit.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiprestackattrib.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribstorprovider.h"
#include "attribparambase.h"
#include "attribfactory.h"
#include "keystrs.h"
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
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"type","attribute")
	    .withtitles(true), "DescSet build group")
    , descset_(*new Attrib::DescSet(su.is2d_))
    , attrsetup_(su)
    , ctio_(*mMkCtxtIOObj(AttribDescSet))
    , uipsattrdesced_(0)			  
    , anychg_(false)
{
    descset_.setCouldBeUsedInAnyDimension( true );
    fillAvailable();
}


uiAttribDescSetBuild::~uiAttribDescSetBuild()
{
    delete ctio_.ioobj;
    delete &descset_;
    delete &ctio_;
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

    const int res = uiMSG().question( "Seismic Attribute Set not saved.\n"
	"Do you want to save it now?", "Yes (store)", "No (discard)", "Cancel");
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
		    const char* idval;
		    mGetStringFromDesc( (*tmpdesc), idval,
			    		Attrib::StorageProvider::keyStr() )
		    const LineKey lk( idval );
		    BufferString bstring = lk.lineName();
		    const char* linenm = bstring.buf();
		    const MultiID mid( linenm+1 );
		    if ( psdpfids_.indexOf( mid ) >=0 )
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
    ctio_.ctxt.forread = forread;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    const bool is2d = attrsetup_.is2d_;

    BufferString emsg;
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
	    emsg = "Can not load Attribute Set:\n";
	    emsg += "Attribute Set is";
	    emsg += isdesc2d ? "2D" : "3D" ;
	    emsg += ". Current definition is ";
	    emsg += is2d ? "2D" : "3D"; 
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

    if ( res )
	usrchg_ = false;
    else
	uiMSG().error( emsg );

    if ( forread && ( !dpfids_.isEmpty() || !psdpfids_.isEmpty() ) )
    {
	for ( int iattr=0; iattr<descset_.size(); iattr++ )
	{
	    Attrib::Desc& desc = *descset_.desc( iattr );
		
	    if ( !desc.isStoredInMem() ) continue;

	    Attrib::ValParam* vp = desc.getValParam(
		    		Attrib::StorageProvider::keyStr() );
	    const MultiID descid( vp->getStringValue(0) + 1 );
	    if ( ( !desc.isPS() && dpfids_.indexOf(descid) < 0 
			        && !psdpfids_.isEmpty() )
		|| ( desc.isPS() && psdpfids_.indexOf( descid ) < 0 
				     && !dpfids_.size() ) )
	    {
		BufferString fidstr = "#";
		fidstr += desc.isPS() ? psdpfids_[0] : dpfids_[0];
		vp->setValue( fidstr.buf() );
	    }
	}
    }

    return res;
}
