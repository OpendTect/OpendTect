/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/

#include "uiattribsetbuild.h"
#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattribsingleedit.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uiprestackattrib.h"
#include "uistoredattrreplacer.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "attribparambase.h"
#include "attribfactory.h"
#include "survinfo.h"
#include "ctxtioobj.h"


uiAttribDescSetBuild::Setup::Setup( bool for2d )
    : is2d_(for2d)
    , showps_(true)
    , singletraceonly_(true)
    , showusingtrcpos_(true)
    , showdepthonlyattrs_(!SI().zIsTime())
    , showtimeonlyattrs_(SI().zIsTime())
    , showhidden_(false)
    , showsteering_(false)
    , issynth_(false)
{
}


uiAttribDescSetBuild::uiAttribDescSetBuild( uiParent* p,
			const uiAttribDescSetBuild::Setup& su )
    : uiBuildListFromList(p, uiBuildListFromList::Setup(false,
		uiStrings::sType().toLower(), uiStrings::sAttribute().toLower())
			.withtitles(true), "DescSet build group")
    , descset_(*new Attrib::DescSet(su.is2d_))
    , attrsetup_(su)
    , uipsattrdesced_(0)
    , anychg_(false)
{
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
    if ( !havesel )
	return;

    const Attrib::DescID descid = descset_.getID( attrnm, true );
    rmbut_->setSensitive( descid.isValid() && !descset_.isAttribUsed(descid) );
}


bool uiAttribDescSetBuild::handleUnsaved()
{
    if ( !anychg_ && !usrchg_ )
	return true;

    const int res = uiMSG().question(tr("Seismic Attribute Set not saved."
					"\n\nDo you want to save it now?"),
				     tr("Yes (store)"), tr("No (discard)"),
				     uiStrings::sCancel());
    if ( res == 0 )
	return true;
    if ( res == -1 )
	return false;

    return ioReq( true );
}


void uiAttribDescSetBuild::fillAvailable()
{
    uiStringSet dispnms;
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
	if ( attrsetup_.issynth_ && !uiAF().isSyntheticSupported(idx) )
	    continue;

	const char* attrnm = uiAF().getAttribName( idx );
	const Attrib::Desc* desc = Attrib::PF().getDesc( attrnm );
	if ( !desc )
	    { pErrMsg("attrib in uiAF() but not in PF()"); continue; }

	if ( attrsetup_.singletraceonly_ && !desc->isSingleTrace() )
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
    uiString attrdispnm; const char* attrnm = 0;
    if ( isadd )
    {
	const uiString* curavsel = curAvSel();
	if ( !curavsel || curavsel->isEmpty() )
	    return;
	attrdispnm = *curavsel;
    }
    else
    {
	attrnm = curDefSel();
	if ( !attrnm || !*attrnm )
	    return;
    }

    Attrib::DescID did;
    if ( attrnm )
	did = descset_.getID( attrnm, true );
    else
    {
	attrnm = uiAF().attrNameOf( attrdispnm );
	Attrib::Desc* desc = Attrib::PF().createDescCopy( attrnm );
	if ( !desc )
	    { pErrMsg("Huh"); return; }
	desc->setUserRef( "" );
	desc->setDescSet( &descset_ );
	descset_.addDesc( desc );
	did = desc->id();
    }

    Attrib::Desc& desc = *descset_.getDesc( did );
    const auto& dpsel = desc.isPS() ? psdpfids_ : dpfids_;
    uiSingleAttribEd dlg( this, desc, isadd, &dpsel );
    bool success = dlg.go();
    if ( success && desc.isPS() )
    {
	for ( int idx=descset_.size()-1; idx>=0; idx-- )
	{
	    Attrib::Desc* tmpdesc = descset_.desc(idx);
	    if ( tmpdesc->isStoredInMem() )
	    {
		const char* idval;
		mGetStringFromDesc( (*tmpdesc), idval,
				    Attrib::StorageProvider::keyStr() )
		const StringPair strpair( idval );
		BufferString bstring = strpair.first();
		const DataPack::FullID fid
		    = DataPack::FullID::getFromString( bstring.buf()+1 );
		if ( psdpfids_.isPresent(fid) )
		    descset_.removeDesc( tmpdesc->id() );
	    }
	}
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


uiString uiAttribDescSetBuild::avFromDef( const char* attrnm ) const
{
    Attrib::DescID did( descset_.getID(attrnm,true) );
    if ( !did.isValid() )
	return uiString::empty();
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
    PtrMan<CtxtIOObj> ctio = descset_.getCtxtIOObj( forread );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    const bool is2d = attrsetup_.is2d_;
    Attrib::DescSet descset( is2d );
    uiRetVal uirv = forread
	? descset_.load( dlg.ioObj()->key() )
	: descset_.store( dlg.ioObj()->key() );

    usrchg_ = false;
    if ( forread && ( !dpfids_.isEmpty() || !psdpfids_.isEmpty() ) )
    {
	uiStoredAttribReplacer replacerdlg( this, &descset_ );
	replacerdlg.setDataPackIDs( dpfids_ );
	replacerdlg.go();
    }

    return true;
}
