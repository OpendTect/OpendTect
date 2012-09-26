/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattribsingleedit.h"
#include "uiattrdesced.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSingleAttribEd::uiSingleAttribEd( uiParent* p, Attrib::Desc& ad, bool isnew )
    : uiDialog(p,Setup(isnew ? "Add attribute" : "Edit attribute",
		    "Define attribute parameters","110.3.1"))
    , desc_(ad)
    , setman_(new Attrib::DescSetMan(ad.is2D(),ad.descSet(),false))
    , nmchgd_(false)
    , anychg_(false)
{
    desced_ = uiAF().create( this, desc_.attribName(), desc_.is2D(), false );
    desced_->setDesc( &desc_, setman_ );

    namefld_ = new uiGenInput( this, "Name", desc_.userRef() );
    namefld_->attach( alignedBelow, desced_ );
}


uiSingleAttribEd::~uiSingleAttribEd()
{
    delete setman_;
}


void uiSingleAttribEd::setDataPackSelection(
			const TypeSet<DataPack::FullID>& ids )
{
    desced_->setDataPackInp( ids );
}


bool uiSingleAttribEd::acceptOK( CallBacker* )
{
    const BufferString oldnm( desc_.userRef() );
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() )
	{ uiMSG().error( "Please enter a valid name" ); return false; }

    if ( oldnm != newnm )
    {
	const Attrib::DescSet& descset = *desc_.descSet();
	const int sz = descset.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const Attrib::Desc& desc = *descset.desc( idx );
	    if ( &desc != &desc_ && newnm == desc.userRef() )
	    {
		uiMSG().error("The name is already used for another attribute");
		return false;
	    }
	}
    }

    const char* msg = desced_->commit();
    if ( msg && *msg )
	{ uiMSG().error( msg ); return false; }

    desc_.setUserRef( newnm );

    anychg_ = true;
    nmchgd_ = oldnm != newnm;
    return true;
}
