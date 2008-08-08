/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2008
 RCS:           $Id: uistoredattrreplacer.cc,v 1.2 2008-08-08 10:57:45 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uistoredattrreplacer.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "linekey.h"
#include "sets.h"
#include "bufstringset.h"

#include "uiparent.h"
#include "uiattrinpdlg.h"

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
	Desc* desc = attrset_.getDesc( storedids_[idx] );
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
    if ( singleseissteer )
    {
	Desc* ad = attrset_.getDesc( storedids_[0] );
	const bool issteer = noofsteer_ > 0;
	const int steerid = ad->dataType() == Seis::Dip ? 0 : 1;

	uiAttrInpDlg dlg( parent_, issteer, ad->is2D() );
	if ( dlg.go() )
	{
	    if ( !issteer )
	    {
		ad->changeStoredID( dlg.getSeisKey() );
		ad->setUserRef( dlg.getSeisRef() );
	    }
	    else
	    {
		steerid == 0 ? ad = attrset_.getDesc( storedids_[1] ) : 
			       ad = attrset_.getDesc( storedids_[0] );
		ad->changeStoredID( dlg.getSeisKey() );
		ad->setUserRef( dlg.getSeisRef() );

		Desc* adsteer = attrset_.getDesc( 
		    DescID(storedids_[steerid].asInt(), true) );
		adsteer->changeStoredID( dlg.getSteerKey() );
		BufferString bfstr = dlg.getSteerRef();
		bfstr += "_crline_dip";
		adsteer->setUserRef( bfstr.buf() );
	    }
	}
    }
    else
    {
	BufferStringSet usrrefs;
	for ( int idnr=0; idnr<storedids_.size(); idnr++ )
	{
	    usrrefs.erase();
	    const DescID storedid = storedids_[idnr];

	    getUserRef( storedid, usrrefs );
	    if ( usrrefs.isEmpty() )
		continue;

	    Desc* ad = attrset_.getDesc( storedid );
	    const bool issteer = ad->dataType() == Seis::Dip;
	    uiAttrInpDlg dlg( parent_, usrrefs, issteer, is2d_ );
	    if ( dlg.go() )
	    {
		if ( !issteer )
		{
		    ad->changeStoredID( dlg.getKey() );
		    ad->setUserRef( dlg.getUserRef() );
		}
		else
		{
		    ad->changeStoredID( dlg.getKey() );
		    BufferString bfstr = dlg.getUserRef();
		    bfstr += "_crline_dip";
		    ad->setUserRef( bfstr.buf() );
		}
	    }
	}
    }

    attrset_.removeUnused( true );
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
	if ( !linekeys.addIfNew(lk) ) continue;

	storedids_ += descid;
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
