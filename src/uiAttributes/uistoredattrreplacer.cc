/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2008
 RCS:           $Id: uistoredattrreplacer.cc,v 1.1 2008-06-18 08:18:48 cvssatyaki Exp $
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
    DescSet& attrset, bool is2d )
    : is2d_(is2d)
    , attrset_(attrset)
    , parent_(parent)
{
    getStoredIds();

    int noofseis = 0;
    int noofsteer = 0;
    for ( int idx=0; idx<storedids_.size(); idx++ )
    {
	Desc* desc = attrset_.getDesc( storedids_[idx] );
	if ( desc->dataType() == Seis::Dip )
	    noofsteer++;
	if ( desc->dataType() == Seis::Ampl ||
			desc->dataType() == Seis::UnknowData )
	    noofseis++;
    }

    const bool multiinpcube = ( noofseis > 1 ) || ( noofsteer > 1 );
    replaceStoredAtrributes( multiinpcube );
}


void uiStoredAttribReplacer::replaceStoredAtrributes( bool multiinpcube )
{
    bool found2d = false;
    int count = 0;
    BufferStringSet usrrefs;
    for ( int idnr=0; idnr<storedids_.size(); idnr++ )
    {
	if ( multiinpcube )
	    count = 0;
	usrrefs.erase();
	const DescID storedid = storedids_[idnr];

	getUserRef( storedid, usrrefs );
	if ( usrrefs.isEmpty() )
	    continue;

	Desc* ad = attrset_.getDesc( storedid );
	const bool issteer = ad->dataType() == Seis::Dip;
	if ( count > 0 )
	    break;
        uiAttrInpDlg dlg( parent_, usrrefs, issteer, is2d_, multiinpcube );
        if ( dlg.go() )
        {
	    if ( multiinpcube )
	    {
		ad->changeStoredID( dlg.getKey() );
		ad->setUserRef( dlg.getUserRef() );
		if ( issteer )
		{
		    Desc* adcrld = 
			attrset_.getDesc( DescID(storedid.asInt()+1, true) );
		    if ( ad && ad->isStored() )
		    {
			adcrld->changeStoredID( dlg.getKey() );
			BufferString bfstr = dlg.getUserRef();
			bfstr += "_crline_dip";
			adcrld->setUserRef( bfstr.buf() );
		    }
		}
		if ( !found2d && dlg.is2D() )
		    found2d = true;
	    }
	    else
	    {
		ad->changeStoredID( dlg.getSeisKey() );
		ad->setUserRef( dlg.getSeisRef() );
		Desc* adsteer = 
			attrset_.getDesc( 
				DescID(storedids_[idnr+1].asInt()+1, true) );
		adsteer->changeStoredID( dlg.getSteerKey() );
		BufferString bfstr = dlg.getSteerRef();
		bfstr += "_crline_dip";
		adsteer->setUserRef( bfstr.buf() );
	    }
	}
	count++;
    }

    attrset_.removeUnused( true );
    if ( found2d )
    {
	for ( int idx=0; idx< attrset_.nrDescs(); idx++ )
	    attrset_.desc(idx)->set2D(true);
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
