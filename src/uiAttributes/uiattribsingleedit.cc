/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattribsingleedit.h"
#include "uiattrdesced.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistrings.h"
#include "od_helpids.h"


uiSingleAttribEd::uiSingleAttribEd( uiParent* p, Attrib::Desc& ad, bool isnew )
    : uiDialog(p,Setup(isnew ? tr("Add attribute") : tr("Edit attribute"),
		    tr("Define attribute parameters"),
                    mODHelpKey(mSingleAttribEdHelpID) ))
    , desc_(ad)
    , setman_(new Attrib::DescSetMan(ad.is2D(),ad.descSet(),false))
    , nmchgd_(false)
    , anychg_(false)
{
    desced_ = uiAF().create( this, desc_.attribName(), desc_.is2D(), false );
    desced_->setDesc( &desc_, setman_ );

    namefld_ = new uiGenInput( this, uiStrings::sName(), desc_.userRef() );
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
	{ uiMSG().error( uiStrings::sEnterValidName() ); return false; }

    if ( oldnm != newnm )
    {
	const Attrib::DescSet& descset = *desc_.descSet();
	const int sz = descset.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const Attrib::Desc& desc = *descset.desc( idx );
	    if ( &desc != &desc_ && newnm == desc.userRef() )
	    {
	uiMSG().error(tr("The name is already used for another attribute"));
		return false;
	    }
	}
    }

    uiString msg = desced_->commit();
    if ( !msg.isEmpty() )
	{ uiMSG().error( msg ); return false; }

    desc_.setUserRef( newnm );

    anychg_ = true;
    nmchgd_ = oldnm != newnm;
    return true;
}
