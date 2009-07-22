/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistoredattrreplacer.cc,v 1.10 2009-07-22 16:01:37 cvsbert Exp $";

#include "uistoredattrreplacer.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "linekey.h"
#include "bufstringset.h"

#include "uiattrinpdlg.h"
#include "uimsg.h"

using namespace Attrib;

uiStoredAttribReplacer::uiStoredAttribReplacer( uiParent* parent,
						DescSet& attrset )
    : is2d_(attrset.is2D())
    , attrset_(attrset)
    , parent_(parent)
    , noofseis_(0)
    , noofsteer_(0)
{
    getStoredIds();

    for ( int idx=0; idx<storedids_.size(); idx++ )
    {
	Desc* desc = attrset_.getDesc( storedids_[idx].firstid_ );
	if ( desc->dataType() == Seis::Dip )
	    noofsteer_++;
	if ( desc->dataType() == Seis::Ampl ||
	     desc->dataType() == Seis::UnknowData )
	    noofseis_++;
    }
}


void uiStoredAttribReplacer::go()
{
    const bool singleseissteer = noofseis_<=1 && noofsteer_<=1;
    singleseissteer ? handleSingleInput() : handleMultiInput();
    attrset_.removeUnused( true );
}


void uiStoredAttribReplacer::handleSingleInput()
{
    if ( storedids_.isEmpty() ) return;

    Desc* ad = attrset_.getDesc( storedids_[0].firstid_ );
    if ( !ad ) { uiMSG().error( "Cannot replace stored entries" ); return; }

    const bool hassteer = noofsteer_ > 0;
    const bool hasseis = noofseis_ > 0;
    const bool firstisdip = ad->dataType() == Seis::Dip;
    const int seisidx = firstisdip ? 1 : 0;
    const int steeridx = firstisdip ? 0 : 1;

    uiAttrInpDlg dlg( parent_, hasseis, hassteer, attrset_.is2D() );
    if ( !dlg.go() )
    {
	attrset_.removeAll( true );
	return;
    }

    if ( hasseis )
    {
	ad = attrset_.getDesc( storedids_[seisidx].firstid_ );
	ad->changeStoredID( dlg.getSeisKey() );
	ad->setUserRef( dlg.getSeisRef() );
    }

    if ( hassteer )
    {
	StoredEntry storeentry = storedids_[steeridx];
	const int ouputidx = attrset_.getDesc(
		DescID(storeentry.firstid_.asInt(),true))->
		    selectedOutput();
	Desc* adsteerinl = new Desc( "Inline Desc" );
	Desc* adsteercrl = new Desc( "Cross Line Desc" );
	if ( ouputidx == 0 )
	{
	    adsteerinl = attrset_.getDesc(
		    DescID(storeentry.firstid_.asInt(),true) );
	    adsteercrl = attrset_.getDesc(
		    DescID(storeentry.secondid_.asInt(),true) );
	}
	else
	{
	    adsteerinl = attrset_.getDesc(
		    DescID(storeentry.secondid_.asInt(),true));
	    adsteercrl = attrset_.getDesc(
		    DescID(storeentry.firstid_.asInt(),true));
	}
	adsteerinl->changeStoredID( dlg.getSteerKey() );
	BufferString bfstr = dlg.getSteerRef();
	bfstr += "_inline_dip";
	adsteerinl->setUserRef( bfstr.buf() );

	adsteercrl->changeStoredID( dlg.getSteerKey() );
	bfstr = dlg.getSteerRef();
	bfstr += "_crline_dip";
	adsteercrl->setUserRef( bfstr.buf() );
    }
}


void uiStoredAttribReplacer::handleMultiInput()
{
    BufferStringSet usrrefs;
    for ( int idnr=0; idnr<storedids_.size(); idnr++ )
    {
	usrrefs.erase();
	StoredEntry storeentry = storedids_[idnr];
	const DescID storedid = storeentry.firstid_;

	getUserRef( storedid, usrrefs );
	if ( usrrefs.isEmpty() )
	    continue;

	Desc* ad = attrset_.getDesc( storedid );
	const bool issteer = ad->dataType() == Seis::Dip;
	uiAttrInpDlg dlg( parent_, usrrefs, issteer, is2d_ );
	if ( !dlg.go() )
	{
	    attrset_.removeAll( true );
	    return;
	}

	if ( !issteer )
	{
	    ad->changeStoredID( dlg.getKey() );
	    ad->setUserRef( dlg.getUserRef() );
	}
	else
	{
	    const int ouputidx = attrset_.getDesc(
		    DescID(storeentry.firstid_.asInt(),true))->
			selectedOutput();
	    Desc* adsteerinl = new Desc( "Inline Desc" );
	    Desc* adsteercrl = new Desc( "Cross Line Desc" );
	    if ( ouputidx == 0 )
	    {
		adsteerinl = attrset_.getDesc(
			DescID(storeentry.firstid_.asInt(),true) );
		adsteercrl = attrset_.getDesc(
			DescID(storeentry.secondid_.asInt(),true) );
	    }
	    else
	    {
		adsteerinl = attrset_.getDesc(
			DescID(storeentry.secondid_.asInt(),true));
		adsteercrl = attrset_.getDesc(
			DescID(storeentry.firstid_.asInt(),true));
	    }
	    adsteerinl->changeStoredID( dlg.getKey() );
	    BufferString bfstr = dlg.getUserRef();
	    bfstr += "_inline_dip";
	    adsteerinl->setUserRef( bfstr.buf() );

	    adsteercrl->changeStoredID( dlg.getKey() );
	    bfstr = dlg.getUserRef();
	    bfstr += "_crline_dip";
	    adsteercrl->setUserRef( bfstr.buf() );
	}
    }
}



void uiStoredAttribReplacer::getUserRef( const DescID& storedid,
					 BufferStringSet& usrref ) const
{
    for ( int idx=0; idx<attrset_.nrDescs(); idx++ )
    {
	const DescID descid = attrset_.getID( idx );
	Desc* ad = attrset_.getDesc( descid );
	if ( !ad || ad->isStored() || ad->isHidden() ) continue;

	if ( hasInput(*ad,storedid) )
	    usrref.addIfNew( ad->userRef() );
    }
}


void uiStoredAttribReplacer::getStoredIds()
{
    TypeSet<LineKey> linekeys;
    for ( int idx=0; idx<attrset_.nrDescs(); idx++ )
    {
	const DescID descid = attrset_.getID( idx );
	Desc* ad = attrset_.getDesc( descid );
        if ( !ad || !ad->isStored() ) continue;

	const ValParam* keypar = ad->getValParam( StorageProvider::keyStr() );
	LineKey lk( keypar->getStringValue() );
	if ( !linekeys.addIfNew(lk) )
	{
	    for ( int idy=0; idy<storedids_.size(); idy++ )
	    {
		if ( lk == storedids_[idy].lk_ )
		{
		    storedids_[idy].secondid_ = descid;
		    break;
		}
	    }
	}
	else
	    storedids_ += StoredEntry( descid, lk );
    }
}


bool uiStoredAttribReplacer::hasInput( const Desc& desc,
				       const DescID& id ) const
{
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	const Desc* inp = desc.getInput( idx );
	if ( !inp )
	{
	    if ( desc.inputSpec(idx).enabled )
		return false;
	    else
		continue;
	}

	if ( inp->id() == id ) return true;
    }

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	const Desc* inp = desc.getInput( idx );
	if ( inp && inp->isHidden() && inp->nrInputs() )
	return hasInput( *inp, id );
    }
    return false;
}
