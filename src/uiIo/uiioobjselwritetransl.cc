/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjselwritetransl.h"
#include "uiioobjsel.h"

#include "ctxtioobj.h"
#include "transl.h"
#include "pixmap.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"


uiIOObjSelWriteTranslator::uiIOObjSelWriteTranslator( uiParent* p,
				const CtxtIOObj& ctio, bool withopts )
    : uiGroup(p,"Write Translator selector")
    , ctxt_(*new IOObjContext(ctio.ctxt))
    , selfld_(0)
    , optsbut_(0)
    , lbl_(0)
{
    const ObjectSet<const Translator>& alltrs = ctio.ctxt.trgroup->templates();
    for ( int idx=0; idx<alltrs.size(); idx++ )
    {
	const Translator* transl = alltrs[idx];
	if ( IOObjSelConstraints::isAllowedTranslator(
		    transl->userName(),ctio.ctxt.toselect.allowtransls_)
	  && transl->isUserSelectable( false ) )
	    trs_ += transl;
    }
    if ( trs_.size() < 2 )
	return;

    selfld_ = new uiComboBox( this, "Write translator field" );
    if ( withopts )
	lbl_ = new uiLabel( this, "Write to", selfld_ );

    int cur = 0;
    for ( int idx=0; idx<trs_.size(); idx++ )
    {
	const Translator& transl = *trs_[idx];
	const BufferString trnm( transl.userName() );
	if ( ctio.ioobj && trnm == ctio.ioobj->translator() )
	    cur = idx;

	selfld_->addItem( trnm );

	BufferString icnm( transl.iconName() );
	if ( !icnm.isEmpty() )
	{
	    const BufferString smllicnm( icnm, "_24x24.png" );
	    if ( ioPixmap::isPresent(smllicnm) )
		icnm = smllicnm;
	    if ( ioPixmap::isPresent(icnm) )
		selfld_->setPixmap( ioPixmap(icnm), idx );
	}
    }
    selfld_->setCurrentItem( cur );

    setHAlignObj( selfld_ );
}


uiIOObjSelWriteTranslator::~uiIOObjSelWriteTranslator()
{
    delete &ctxt_;
}


uiObject* uiIOObjSelWriteTranslator::endObj( bool left )
{
    if ( !selfld_ )
	return 0;

    if ( left )
    {
	if ( lbl_ )
	    return lbl_;
    }
    else if ( optsbut_ )
	return optsbut_;

    return selfld_;
}


int uiIOObjSelWriteTranslator::translIdx() const
{
    int translidx = -1;
    if ( selfld_ )
    {
	const int selidx = selfld_->currentItem();
	if ( selidx >= 0 )
	    translidx = ctxt_.trgroup->templates().indexOf( trs_[selidx] );
    }
    return translidx;
}


const Translator* uiIOObjSelWriteTranslator::selectedTranslator() const
{
    int translidx = translIdx();
    if ( translidx < 0 )
	translidx = ctxt_.trgroup->defTranslIdx();
    return ctxt_.trgroup->templates()[ translidx ];
}


IOObj* uiIOObjSelWriteTranslator::mkEntry( const char* nm ) const
{
    CtxtIOObj ctio( ctxt_ );
    ctio.ioobj = 0; ctio.setName( nm );
    ctio.fillObj( false, translIdx() );
    return ctio.ioobj;
}


void uiIOObjSelWriteTranslator::use( const IOObj& ioobj )
{
    if ( !selfld_ )
	return;

    selfld_->setCurrentItem( ioobj.translator() );
}
