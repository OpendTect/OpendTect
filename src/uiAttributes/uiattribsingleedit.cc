/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/

#include "uiattribsingleedit.h"
#include "uiattrdesced.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistrings.h"
#include "od_helpids.h"


uiSingleAttribEd::uiSingleAttribEd( uiParent* p, Attrib::Desc& ad, bool isnew,
		const TypeSet<DataPack::FullID>* dpids )
    : uiDialog(p,Setup(isnew ? tr("Add attribute") : tr("Edit attribute"),
		    tr("Define attribute parameters"),
                    mODHelpKey(mSingleAttribEdHelpID) ))
    , desc_(ad)
    , nmchgd_(false)
    , anychg_(false)
{
    desced_ = uiAF().create( this, desc_.attribName(), desc_.is2D() );
    if ( dpids )
	desced_->setDataPackInp( *dpids );
    desced_->setDesc( &desc_ );

    namefld_ = new uiGenInput( this, uiStrings::sName(), desc_.userRef() );
    namefld_->attach( alignedBelow, desced_ );
}


uiSingleAttribEd::~uiSingleAttribEd()
{
}


bool uiSingleAttribEd::acceptOK()
{
    const BufferString oldnm( desc_.userRef() );
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() )
	{ uiMSG().error( uiStrings::phrEnterValidName() ); return false; }

    if ( oldnm != newnm )
    {
	const Attrib::DescSet& descset = *desc_.descSet();
	const int sz = descset.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    const Attrib::Desc& desc = *descset.desc( idx );
	    if ( &desc != &desc_ && newnm == desc.userRef() )
	    {
		uiMSG().error( tr("Specified name is already used") );
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
