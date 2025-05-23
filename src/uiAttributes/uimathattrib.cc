/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimathattrib.h"
#include "mathattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribstorprovider.h"
#include "ioman.h"
#include "mathformula.h"
#include "od_helpids.h"
#include "seisioobjinfo.h"

#include "uiattrsel.h"
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimathformula.h"
#include "uirockphysform.h"
#include "uitoolbutton.h"

#include <math.h>

using namespace Attrib;

mInitAttribUI(uiMathAttrib,Attrib::Mathematics,"Mathematics",sKeyBasicGrp())

uiMathAttrib::uiMathAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mMathAttribHelpID) )
	, form_(*new Math::Formula(true,Attrib::Mathematics::getSpecVars()))
{
    uiAttrSelData asd( is2d );
    uiMathFormula::Setup mfsu( tr("Formula") );
    mfsu.withunits( false ).maxnrinps( 8 ).withsubinps( true )
	.stortype( "Attribute calculation" );
    formfld_ = new uiMathFormula( this, form_, mfsu );
    formfld_->exprFld()->setPlaceholderText(
		toUiString("nearstk + c0 * farstk") );
    mAttachCB( formfld_->inpSet, uiMathAttrib::inpSel );
    updateNonSpecInputs();

    const CallBack rockphyscb( mCB(this,uiMathAttrib,rockPhysReq) );
    uiToolButtonSetup tbsu( "rockphys", tr("Use rockphysics formula"),
			    rockphyscb, uiStrings::sRockPhy());
    formfld_->addButton( tbsu );
    setHAlignObj( formfld_ );
}


uiMathAttrib::~uiMathAttrib()
{
    detachAllNotifiers();
    delete &form_;
}


void uiMathAttrib::formSel( CallBacker* )
{
    updateNonSpecInputs();
}


DataPack::FullID uiMathAttrib::getInputDPID( int inpidx ) const
{
    DataPack::FullID undefid;
    if ( dpfids_.isEmpty() )
	return undefid;

    if ( inpidx<0 || inpidx>=formfld_->nrInpFlds() )
	return undefid;

    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	DataPack::FullID dpfid = dpfids_[idx];
	BufferString dpnm = DataPackMgr::nameOf( dpfid );
	if ( dpnm == formfld_->inpFld(inpidx)->getInput() )
	    return dpfid;
    }

    return undefid;
}



void uiMathAttrib::inpSel( CallBacker* cb )
{
    if ( !ads_ || !dpfids_.isEmpty() )
	return;

    mDynamicCastGet(uiMathExpressionVariable*,inpfld,cb);
    if ( !inpfld || !inpfld->isActive() ||
	  inpfld->isConst() || inpfld->isSpec() )
	return;

    inpfld->setNonSpecSubInputs( BufferStringSet() );

    RefMan<Desc> inpdesc = ads_->getDescFromUIListEntry( inpfld->getInput() );
    if ( !inpdesc || !inpdesc->isStored() )
	return;

    const MultiID mid = inpdesc->getStoredID( false );
    ConstPtrMan<IOObj> inpobj = IOM().get( mid );
    if ( !inpobj )
	return;

    const SeisIOObjInfo seisinfo( *inpobj );
    if ( seisinfo.nrComponents() > 1 )
    {
	BufferStringSet nms;
	seisinfo.getComponentNames( nms );
	nms.insertAt( new BufferString("ALL"), 0 );
	inpfld->setNonSpecSubInputs( nms );
    }
}


void uiMathAttrib::rockPhysReq( CallBacker* )
{
    uiDialog rpdlg( this, uiDialog::Setup(uiStrings::sRockPhy(),
					  mODHelpKey(mrockPhysReqHelpID)) );
    auto* rpform = new uiRockPhysForm( &rpdlg );
    if ( !rpdlg.go() )
	return;

    rpform->getFormulaInfo( form_ );
}


bool uiMathAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Attrib::Mathematics::attribName() )
	return false;

    mIfGetString( Attrib::Mathematics::expressionStr(), expression,
		  formfld_->setText(expression) );

    formSel(nullptr);

    int constidx = 0;
    TypeSet<int> inpindexesinuse;
    if ( desc.getParam(Attrib::Mathematics::cstStr()) )
    {
	mDescGetConstParamGroup(Attrib::DoubleParam,cstset,desc,
				Attrib::Mathematics::cstStr());
	while ( constidx<cstset->size() )
	{
	    bool found = false;
	    const ValParam& param = (ValParam&)(*cstset)[constidx];
	    for ( int iinp=constidx; iinp<form_.nrInputs() && !found; iinp++ )
	    {
		if ( !form_.isConst(iinp) || inpindexesinuse.isPresent(iinp) )
		    continue;

		for ( int idx=0; !found; idx++ )
		{
		    BufferString cststr ( "c", idx );
		    if ( caseInsensitiveEqual( form_.variableName(iinp),cststr))
		    {
			form_.setInputDef( iinp, toString(param.getDValue()) );
			formfld_->inpFld(iinp)->use( form_ );
			constidx++;
			found = true;
			inpindexesinuse += iinp;
		    }
		}
	    }
	}
    }

    if ( form_.isRecursive()
	    && desc.getValParam(Attrib::Mathematics::recstartvalsStr()) )
    {
	FileMultiString recvalsstr = desc.getValParam(
		Attrib::Mathematics::recstartvalsStr())->getStringValue(0);
	for ( int idx=0; idx<recvalsstr.size(); idx++ )
	{
	    const double val = recvalsstr.getDValue( idx );
	    form_.recStartVals()[idx] = mIsUdf(val) ? 0 : val;
    }
    }

    return true;
}


void uiMathAttrib::updateNonSpecInputs()
{
    BufferStringSet inpnms;
    if ( !dpfids_.isEmpty() )
    {
	for ( int idx=0; idx<dpfids_.size(); idx++ )
	    inpnms.add( DataPackMgr::nameOf(dpfids_[idx]) );
    }
    else if ( ads_ )
	ads_->fillInUIInputList( inpnms );

    formfld_->setNonSpecInputs( inpnms );
}


bool uiMathAttrib::setInput( const Desc& desc )
{
    if ( dpfids_.isEmpty() && !ads_ )
	setDescSet( desc.descSet() );

    updateNonSpecInputs();
    int varinplastidx = 0;
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	ConstRefMan<Desc> inpdsc = desc.getInput( idx );
	if ( inpdsc )
	{
	    BufferString refstr = inpdsc->isStored()
				? BufferString ( "[",inpdsc->userRef(),"]")
				: BufferString( inpdsc->userRef() );
	    if ( inpdsc->isStoredInMem() )
	    {
		const MultiID dbky = inpdsc->getStoredID();
		const DataPack::FullID dpfid( dbky );
		refstr = DataPackMgr::nameOf( dpfid );
	    }

	    for ( int varinpidx = varinplastidx; varinpidx<form_.nrInputs();
		  varinpidx ++ )

		if ( !form_.isConst(varinpidx) && !form_.isSpec(varinpidx) )
		{
		    form_.setInputDef( varinpidx, refstr );
		    formfld_->inpFld(varinpidx)->use( form_ );
		    varinplastidx = varinpidx+1;

		    const MultiID mid = inpdsc->getStoredID( false );
		    ConstPtrMan<IOObj> inpobj = IOM().get( mid );
		    if ( !inpobj )
			break;

		    SeisIOObjInfo seisinfo( *inpobj );
		    if ( seisinfo.nrComponents() > 1 )
		    {
			BufferStringSet nms;
			seisinfo.getComponentNames( nms );
			nms.insertAt( new BufferString("ALL"), 0 );
			formfld_->setNonSpecSubInputs( nms, varinpidx );
			formfld_->inpFld(varinpidx)->selectSubInput(
						inpdsc->selectedOutput()+1 );
		    }
		    else
			formfld_->setNonSpecSubInputs(
				BufferStringSet(), varinpidx );
		    break;
		}
	}
    }

    return true;
}


bool uiMathAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Attrib::Mathematics::attribName() )
	return false;

    mSetString( Attrib::Mathematics::expressionStr(),
		formfld_->exprFld()->text() );

    int nrconsts = form_.nrConsts();
    if ( nrconsts )
    {
	mDescGetParamGroup(DoubleParam,cstset,desc,
			   Attrib::Mathematics::cstStr())
	cstset->setSize( nrconsts );
	int constidx = -1;
	for ( int idx=0; idx<form_.nrInputs(); idx++ )
	{
	    if ( form_.isConst(idx) )
	    {
		constidx++;
		DoubleParam& dparam = (DoubleParam&)(*cstset)[constidx];
		dparam.setValue( formfld_->getConstVal(idx) );
    }
	}
    }

    if ( form_.isRecursive() )
    {
	FileMultiString fms;
	for ( int idx=0; idx<form_.maxRecShift(); idx++ )
	    fms.add( form_.recStartVals()[idx] );

	mSetString( Attrib::Mathematics::recstartvalsStr(), fms );
    }

    return true;
}


bool uiMathAttrib::getInput( Desc& desc )
{
    int attrinpidx = -1;
    if ( !formfld_->checkValidNrInputs() )
	return false;

    for ( int idx=0; idx<form_.nrInputs(); idx++ )
    {
	if ( !form_.isConst(idx) && !form_.isSpec(idx) )
	{
	    attrinpidx++;
	    if ( attrinpidx >= desc.nrInputs() )
		return false;

	    RefMan<Attrib::Desc> inpdesc;
	    if ( dpfids_.isEmpty() )
	    {
		inpdesc = desc.descSet()->getDescFromUIListEntry(
					formfld_->inpFld(idx)->getInput() );
	    }
	    else
	    {
		const DataPack::FullID inpdpfid = getInputDPID( attrinpidx );
		if ( !inpdpfid.isValid() )
		    return false;

		inpdesc = Attrib::PF().createDescCopy(
					    StorageProvider::attribName() );
		Attrib::ValParam* param =
		    inpdesc->getValParam( Attrib::StorageProvider::keyStr() );
		param->setValue( inpdpfid.asMultiID() );
		if ( desc.descSet() )
		    desc.descSet()->addDesc( inpdesc.ptr() );
	    }

	    const bool res = desc.setInput( attrinpidx, inpdesc.ptr() );
	    if ( !res )
		return false;
	}
    }

    return true;
}


void uiMathAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( !curDesc() ) return;

    int cstinpidx = -1;
    for ( int idx=0; idx<form_.nrInputs(); idx++ )
    {
	if ( form_.isConst(idx) )
	{
	    cstinpidx++;
	    params += EvalParam( form_.variableName(idx),
				 Attrib::Mathematics::cstStr(), 0, cstinpidx );
	}
    }
}
