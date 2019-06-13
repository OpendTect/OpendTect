/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2008
________________________________________________________________________

-*/


#include "uiattremout.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "dbkey.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "od_helpids.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uimsg.h"
#include "uimultoutsel.h"

using namespace Attrib;

uiAttrEMOut::uiAttrEMOut( uiParent* p, const DescSet& ad,
			  const NLAModel* model, const DBKey& dbky,
			  const char* dlgnm )
    : uiBatchProcDlg(p,uiString::empty(),false, Batch::JobSpec::AttribEM )
    , ads_(new Attrib::DescSet(ad))
    , nlamodel_(0)
    , nlaid_(dbky)
{
    if ( model )
	nlamodel_ = model->clone();

    setTitleText( uiString::empty() );
    attrfld_ = new uiAttrSel( pargrp_, *ads_, uiAttrSel::sQuantityToOutput() );
    attrfld_->setNLAModel( nlamodel_ );
    attrfld_->selectionChanged.notify( mCB(this,uiAttrEMOut,attribSel) );
}


uiAttrEMOut::~uiAttrEMOut()
{
    delete ads_;
    delete nlamodel_;
}


void uiAttrEMOut::getDescNames( BufferStringSet& nms ) const
{
    nms = outdescnms_;
}


bool uiAttrEMOut::prepareProcessing()
{
    if ( !attrfld_->haveSelection() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("the output quantity")) );
	return false;
    }

    return true;
}


Attrib::DescSet* uiAttrEMOut::getTargetDescSet(
	TypeSet<Attrib::DescID>& outdescids, const char* outputnm )
{
    if ( nlamodel_ && attrfld_->outputNr()>=0 )
    {
	if ( nlaid_.isInvalid() )
	{
	    uiMSG().error(tr("NN needs to be stored before it can be used") );
	    return 0;
	}

	if ( !addNLA(nladescid_) )
	    return 0;
    }

    TypeSet<int> seloutputs;
    const DescID targetid =
	nladescid_.isValid() ? nladescid_ : attrfld_->attribID();
    const Attrib::Desc* seldesc = ads_->getDesc( targetid );
    if ( seldesc )
    {
	uiMultOutSel dlg( this, *seldesc );
	if ( dlg.doDisp() )
	{
	    if ( dlg.go() )
	    {
		dlg.getSelectedOutputs( seloutputs );
		dlg.getSelectedOutNames( outdescnms_ );
	    }
	    else
		return 0;
	}
    }

    Attrib::DescSet* ret = ads_->optimizeClone( targetid );
    if ( !ret )
	return 0;

    if ( !seloutputs.isEmpty() )
    {
	const BufferString prefix( outputnm, " - " );
	for ( int idx=0; idx<outdescnms_.size(); idx++ )
	{
	    BufferString& nm = outdescnms_.get( idx );
	    nm.insertAt( 0, prefix.buf() );
	}

	if ( outdescnms_.size()==1 )
	    outdescnms_.get(0).set( outputnm );

	ret->createAndAddMultOutDescs( targetid, seloutputs,
					outdescnms_, outdescids );
    }
    else
    {
	outdescnms_.add( outputnm );
	outdescids += targetid;
    }

    return ret;
}


bool uiAttrEMOut::fillPar( IOPar& iopar )
{
    outdescids_.setEmpty();

    BufferString outputnm;
    iopar.get( sKey::Target(), outputnm );
    DescSet* clonedset = getTargetDescSet( outdescids_, outputnm.buf() );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    iopar.mergeComp( attrpar, SeisTrcStorOutput::attribkey() );

    if ( attrfld_->is2D() )
    {
	DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	const Desc* desc = nlamodel_ ? descset.getFirstStored()
				     : clonedset->getFirstStored();
	DBKey storedid = desc ? desc->getStoredID() : DBKey::getInvalid();
	if ( storedid.isValid() )
	    iopar.set( "Input Line Set", storedid );
    }

    ads_->removeDesc( nladescid_ );
    return true;
}


void uiAttrEMOut::fillOutPar( IOPar& iopar, const char* outtyp,
			      const char* idlbl, const char* outid )
{
    iopar.set( IOPar::compKey( sKey::Output(), sKey::Type()), outtyp );
    BufferString key;
    BufferString tmpkey;
    mDefineStaticLocalObject( const BufferString, keybase,
			      ( IOPar::compKey(sKey::Output(), 0) ) );
    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    key = IOPar::compKey( tmpkey.buf(), DescSet::highestIDStr() );
    iopar.set( key, outdescids_.size() );

    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    for ( int idx=0; idx<outdescids_.size(); idx++ )
    {
	key = IOPar::compKey( tmpkey.buf(), idx );
	iopar.set( key, outdescids_[idx].getI() );
    }

    key = IOPar::compKey( keybase.buf(), idlbl );
    iopar.set( key, outid );
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrEMOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    const int outputnr = attrfld_->outputNr();
    uiRetVal uirv;
    EngineMan::addNLADesc( defstr, id, *ads_, outputnr, nlamodel_, uirv );
    if ( !uirv.isEmpty() )
	mErrRet( uirv );

    return true;
}


void uiAttrEMOut::updateAttributes( const Attrib::DescSet& descset,
				    const NLAModel* nlamodel,
				    const DBKey& nlaid )
{
    delete ads_;
    ads_ = new Attrib::DescSet( descset );
    if ( nlamodel )
    {
	delete nlamodel_;
	nlamodel_ = nlamodel->clone();
    }

    attrfld_->setNLAModel( nlamodel_ );
    nlaid_ = nlaid;
    attrfld_->setDescSet( ads_ );
}
