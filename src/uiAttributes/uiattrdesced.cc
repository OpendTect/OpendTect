/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/




#include "uiattrdesced.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uilabel.h"
#include "uisteeringsel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribprovider.h"
#include "attribstorprovider.h"
#include "attribfactory.h"
#include "iopar.h"
#include "survinfo.h"

using namespace Attrib;

const char* uiAttrDescEd::timegatestr()	        { return "Time gate"; }
const char* uiAttrDescEd::stepoutstr()	        { return "Stepout"; }
const char* uiAttrDescEd::frequencystr()        { return "Frequency"; }
const char* uiAttrDescEd::filterszstr()	        { return "Filter size"; }

uiString uiAttrDescEd::sOtherGrp()	{ return uiStrings::sOther(); }
uiString uiAttrDescEd::sBasicGrp()	{ return uiStrings::sBasic(); }
uiString uiAttrDescEd::sFilterGrp()	{ return uiStrings::sFilter(mPlural); }
uiString uiAttrDescEd::sFreqGrp()	{ return uiStrings::sFrequency(); }
uiString uiAttrDescEd::sStatsGrp()	{ return uiStrings::sStatistics(); }
uiString uiAttrDescEd::sDipGrp()	{ return uiStrings::sDip(); }
uiString uiAttrDescEd::sPositionGrp()	{ return uiStrings::sPosition(mPlural);}
uiString uiAttrDescEd::sExperimentalGrp() { return tr("Experimental"); }
uiString uiAttrDescEd::sPatternGrp()	{ return uiStrings::sPattern(mPlural); }
uiString uiAttrDescEd::sTraceMatchGrp()	{ return tr("Trace Match"); }


uiString uiAttrDescEd::sInputTypeError( int inp )
{
    return tr("The suggested attribute for input %1 "
            "is incompatible with the input (wrong datatype)")
            .arg( toString( inp ) );
}

uiString uiAttrDescEd::sDefLabel()
{
    return uiAttrSel::sDefLabel();
}


const char* uiAttrDescEd::getInputAttribName( uiAttrSel* inpfld,
					      const Desc& desc )
{
    Attrib::DescID did = inpfld->attribID();
    Attrib::Desc* attrd = desc.descSet()->getDesc(did);

    return attrd ? attrd->attribName().buf() : "";
}


uiAttrDescEd::uiAttrDescEd( uiParent* p, bool is2d, const HelpKey& helpkey )
    : uiGroup(p)
    , desc_(0)
    , ads_(0)
    , is2d_(is2d)
    , helpkey_(helpkey)
    , zdomaininfo_(0)
    , descChanged(this)
    , descSetChanged(this)
{
}


uiAttrDescEd::~uiAttrDescEd()
{
}


void uiAttrDescEd::setDesc( Attrib::Desc* desc )
{
    if ( desc_ != desc )
    {
	desc_ = desc;
	if ( desc_ )
	{
	    setDescSet( desc_->descSet() );
	    setParameters( *desc );
	    setInput( *desc );
	    setOutput( *desc );
	}
	descChanged.trigger();
    }
}


void uiAttrDescEd::setZDomainInfo( const ZDomain::Info* info )
{ zdomaininfo_ = info; }

const ZDomain::Info* uiAttrDescEd::getZDomainInfo() const
{ return zdomaininfo_; }


void uiAttrDescEd::setDataPackInp( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;

    if ( desc_ )
	setInput( *desc_ );
}


void uiAttrDescEd::setDescSet( Attrib::DescSet* ds )
{
    if ( !ds || ads_ == ds )
	return;

    ads_ = ds;
    setInitialDefaults( *ads_ );
    descSetChanged.trigger();
}


void uiAttrDescEd::setInitialDefaults( const Attrib::DescSet& ds )
{
    Desc* desc = Attrib::PF().createDescCopy( attribName() );
    if ( desc )
	setParameters( *desc );
    desc->unRef();
}


uiRetVal uiAttrDescEd::fillInp( uiAttrSel* fld, Attrib::Desc& desc, int inpnr )
{
    if ( !fld || inpnr >= desc.nrInputs() || !desc.descSet() )
	return uiRetVal::OK();

    const DescID attribid = fld->attribID();

    const Attrib::Desc* oldinpdesc = desc.getInput( inpnr );
    if ( oldinpdesc )
	chtr_.set( oldinpdesc->id(), attribid );
    else
	chtr_.setChanged( true );

    const Attrib::Desc* newinpdesc = desc.descSet()->getDesc( attribid );
    if ( newinpdesc == &desc )
	return uiRetVal( tr("Cannot allow recursive input for %1 '%2'.\n"
		  "Please select a different input")
		  .arg( desc.attribName() )
		  .arg( desc.userRef() ) );
    else if ( !desc.setInput(inpnr,newinpdesc) )
	return uiRetVal( tr("The suggested attribute for input %1 (%2)\n"
		  "is incompatible with the input (wrong datatype)")
		  .arg( toString(inpnr) )
		  .arg( newinpdesc ? newinpdesc->userRef() : "<no input>" ) );

    mDynamicCastGet(const uiImagAttrSel*,imagfld,fld)
    if ( imagfld )
    {
	newinpdesc = desc.descSet()->getDesc( imagfld->imagID() );
	desc.setInput( inpnr+1, newinpdesc );
    }

    return uiRetVal::OK();
}


uiRetVal uiAttrDescEd::fillInp( uiSteeringSel* fld, Attrib::Desc& desc,
				int inpnr )
{
    if ( !fld || inpnr >= desc.nrInputs() || !desc.descSet() )
	return uiRetVal::OK();

    const DescID descid = fld->descID();
    const Attrib::Desc* inpdesc = desc.getInput( inpnr );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), descid );
    else if ( fld->willSteer() )
	chtr_.setChanged( true );

    if ( !desc.setInput( inpnr, desc.descSet()->getDesc(descid) ) )
	return uiRetVal( sInputTypeError(inpnr) );

    return uiRetVal::OK();
}


uiRetVal uiAttrDescEd::fillInp( uiSteerAttrSel* fld, Attrib::Desc& desc,
				int inpnr )
{
    if ( !fld || inpnr >= desc.nrInputs() || !desc.descSet() )
	return uiRetVal::OK();

    const DescID inlid = fld->inlDipID();
    const Attrib::Desc* inpdesc = desc.getInput( inpnr );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), inlid );
    else
	chtr_.setChanged( true );

    if ( !desc.setInput( inpnr, desc.descSet()->getDesc(inlid) ) )
	return uiRetVal( sInputTypeError(inpnr) );

    const DescID crlid = fld->crlDipID();
    inpdesc = desc.getInput( inpnr+1 );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), crlid );
    else
	chtr_.setChanged( true );

    desc.setInput( inpnr+1, desc.descSet()->getDesc(crlid) );
    return uiRetVal::OK();
}


void uiAttrDescEd::fillOutput( Attrib::Desc& desc, int selout )
{
    if ( chtr_.set(desc.selectedOutput(),selout) )
	desc.selectOutput( selout );
}


uiAttrSel* uiAttrDescEd::createInpFld( bool is2d, const uiString& txt )
{
    const uiAttrSelData asd( is2d );
    return new uiAttrSel( this, asd.attrSet(), txt, asd.attribid_ );
}


uiAttrSel* uiAttrDescEd::createInpFld( const uiAttrSelData& asd,
				       const uiString& txt )
{
    return new uiAttrSel( this, asd, txt );
}


uiImagAttrSel* uiAttrDescEd::createImagInpFld( bool is2d )
{
    uiAttrSelData asd( is2d );
    return new uiImagAttrSel( this, uiString::empty(), asd );
}


void uiAttrDescEd::putInp( uiAttrSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    if ( !dpfids_.isEmpty() )
	inpfld->setDataPackInputs( dpfids_ );

    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( inpdesc )
	inpfld->setDesc( inpdesc );
    else
    {
	Attrib::DescID defaultdid = ad.descSet()->ensureDefStoredPresent();
	Attrib::Desc* defaultdesc = ad.descSet()->getDesc( defaultdid );
	if ( !defaultdesc )
	    inpfld->setDescSet( ad.descSet() );
	else
	    inpfld->setDesc( defaultdesc );
    }
}


void uiAttrDescEd::putInp( uiSteerAttrSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( inpdesc )
	inpfld->setDesc( inpdesc );
    else
	inpfld->setDescSet( ad.descSet() );
}


void uiAttrDescEd::putInp( uiSteeringSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( inpdesc )
	inpfld->setDesc( inpdesc );
    else
	inpfld->setDescSet( ad.descSet() );
}


uiString uiAttrDescEd::zDepLabel( const uiString& pre,
				  const uiString& post ) const
{
    BufferString lbl;
    uiString zstr( zIsTime() ? uiStrings::sTime() : uiStrings::sDepth() );
    uiString ret;
    if ( !pre.isEmpty() && !post.isEmpty() )
    {
	return toUiString( "%1 %2 %3 (%4)" )
	    .arg( pre )
	    .arg( zstr.toLower() )
	    .arg( post )
	    .arg( SI().zUnitString() );
    }

    if ( !pre.isEmpty() )
    {
	zstr.toLower( true );
	return toUiString("%1 %2 (%3)").arg(pre).arg(zstr)
						.arg(SI().zUnitString());
    }

    if ( !post.isEmpty() )
	return toUiString("%1 %2 (%3)").arg(zstr).arg(post)
						.arg(SI().zUnitString());


    return toUiString("%1 (%2)").arg(zstr).arg(SI().zUnitString());
}


bool uiAttrDescEd::zIsTime() const
{
    return SI().zIsTime();
}


uiRetVal uiAttrDescEd::errMsgs( Attrib::Desc* desc )
{
    uiRetVal uirv = uiRetVal::OK();
    if ( !desc )
	return uirv;

    if ( Desc::isError( desc->satisfyLevel() ) )
	uirv = desc->errMsg();

    uirv.add( areUIParsOK() );
    return uirv;
}


uiRetVal uiAttrDescEd::commit( Attrib::Desc* editdesc )
{
    if ( !editdesc )
	editdesc = desc_;
    if ( !editdesc )
	return uiRetVal::OK();

    getParameters( *editdesc );
    uiRetVal uirv = Provider::prepare( *editdesc );
    if ( !uirv.isOK() )
	return uirv;

    editdesc->updateParams();	//needed before getInput to set correct input nr
    uirv = getInput( *editdesc );
    if ( !uirv.isOK() )
	return uirv;
    if ( !getOutput(*editdesc) )
	return uiRetVal( tr("Cannot get attribute output") );

    editdesc->updateParams();	//needed after getInput to update inputs' params
    return errMsgs( editdesc );
}


bool uiAttrDescEd::getOutput( Attrib::Desc& desc )
{
    desc.selectOutput( 0 );
    return true;
}


bool uiAttrDescEd::getInputDPID( uiAttrSel* inpfld,
				 DataPack::FullID& inpdpfid ) const
{
    StringPair inpstr( inpfld->getAttrName() );
    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	DataPack::FullID dpfid = dpfids_[idx];
	BufferString dpnm = DataPackMgr::nameOf( dpfid );
	if ( inpstr.first() == dpnm )
	    { inpdpfid = dpfid; return true; }
    }

    return false;
}


Desc* uiAttrDescEd::getInputDescFromDP( uiAttrSel* inpfld ) const
{
    if ( !dpfids_.size() )
    {
	pErrMsg( "No datapacks present to form Desc" );
	return 0;
    }

    DataPack::FullID inpdpfid;
    if ( !getInputDPID(inpfld,inpdpfid) )
	return 0;

    BufferString dpidstr( "#" );
    dpidstr.add( inpdpfid.toString() );
    Desc* inpdesc = Attrib::PF().createDescCopy( StorageProvider::attribName());
    Attrib::ValParam* param =
	inpdesc->getValParam( Attrib::StorageProvider::keyStr() );
    param->setValue( dpidstr.buf() );
    return inpdesc;
}
