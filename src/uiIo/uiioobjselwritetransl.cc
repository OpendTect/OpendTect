/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiioobjselwritetransl.h"
#include "uiioobjsel.h"

#include "ctxtioobj.h"
#include "transl.h"
#include "ioman.h"
#include "pixmap.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"

mImplFactory1Param(uiIOObjTranslatorWriteOpts,uiParent*,
		   uiIOObjTranslatorWriteOpts::factory);


uiIOObjTranslatorWriteOpts::uiIOObjTranslatorWriteOpts( uiParent* p,
							const Translator& trl )
    : uiGroup(p,BufferString("Write options group for ",getName4Factory(trl)))
    , transl_(trl)
{
}


const char* uiIOObjTranslatorWriteOpts::getName4Factory( const Translator& trl )
{
    mDeclStaticString( ret );
    ret.set( trl.userName() ).add( " [" )
	.add( trl.group()->userName() ).add( "]" );
    return ret.str();
}


uiIOObjSelWriteTranslator::uiIOObjSelWriteTranslator( uiParent* p,
				const CtxtIOObj& ctio, bool withopts )
    : uiGroup(p,"Write Translator selector")
    , ctxt_(*new IOObjContext(ctio.ctxt))
    , selfld_(0)
    , lbl_(0)
{
    optflds_.allowNull( true );
    const TranslatorGroup& trgrp = *ctio.ctxt.trgroup;
    const ObjectSet<const Translator>& alltrs = trgrp.templates();
    for ( int idx=0; idx<alltrs.size(); idx++ )
    {
	const Translator* trl = alltrs[idx];
	if ( IOObjSelConstraints::isAllowedTranslator(
		    trl->userName(),ctio.ctxt.toselect.allowtransls_)
	  && trl->isUserSelectable( false ) )
	    trs_ += trl;
    }
    if ( trs_.size() < 1 )
    {
	if ( alltrs.isEmpty() )
	{
	    pErrMsg(BufferString("No translator for",trgrp.userName()));
	}
	return;
    }

    if ( trs_.size() > 1 )
	mkSelFld( ctio, withopts );

    uiIOObjTranslatorWriteOpts* firstoptfld = 0;
    if ( withopts )
    {
	for ( int idx=0; idx<trs_.size(); idx++ )
	{
	    uiIOObjTranslatorWriteOpts* fld =
		uiIOObjTranslatorWriteOpts::create( this, *trs_[idx] );
	    optflds_ += fld;
	    if ( fld && !firstoptfld )
		firstoptfld = fld;
	    if ( selfld_ && fld )
		fld->attach( alignedBelow, selfld_ );
	}
    }

    if ( selfld_ )
	setHAlignObj( selfld_ );
    else if ( firstoptfld )
	setHAlignObj( firstoptfld );
}


void uiIOObjSelWriteTranslator::mkSelFld( const CtxtIOObj& ctio, bool withopts )
{
    selfld_ = new uiComboBox( this, "Write translator field" );
    if ( withopts )
	lbl_ = new uiLabel( this, "Write to", selfld_ );

    int cur = 0;
    for ( int idx=0; idx<trs_.size(); idx++ )
    {
	const Translator& trl = *trs_[idx];
	const BufferString trnm( trl.userName() );
	if ( ctio.ioobj && trnm == ctio.ioobj->translator() )
	    cur = idx;

	selfld_->addItem( trnm );

	BufferString icnm( trl.iconName() );
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

    const CallBack selchgcb( mCB(this,uiIOObjSelWriteTranslator,selChg) );
    selfld_->selectionChanged.notify( selchgcb );
    postFinalise().notify( selchgcb );
}


uiIOObjSelWriteTranslator::~uiIOObjSelWriteTranslator()
{
    delete &ctxt_;
}


bool uiIOObjSelWriteTranslator::isEmpty() const
{
    if ( selfld_ )
	return false;

    for ( int idx=0; idx<optflds_.size(); idx++ )
	if ( optflds_[idx] )
	    return false;

    return true;
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


void uiIOObjSelWriteTranslator::setTranslator( const Translator* trl )
{
    if ( !trl || !selfld_ )
	return;

    const int tridx = trs_.indexOf( trl );
    if ( tridx >= 0 )
	selfld_->setCurrentItem( tridx );
}


const Translator* uiIOObjSelWriteTranslator::selectedTranslator() const
{
    int translidx = translIdx();
    if ( translidx < 0 )
	translidx = ctxt_.trgroup->defTranslIdx();
    return ctxt_.trgroup->templates()[ translidx ];
}


bool uiIOObjSelWriteTranslator::hasSelectedTranslator( const IOObj& ioobj) const
{
    const Translator* trl = selectedTranslator();
    if ( !trl )
	return ioobj.translator().isEmpty();

    return ioobj.translator() == trl->userName()
	&& ioobj.group() == trl->group()->userName();
}


uiIOObjTranslatorWriteOpts* uiIOObjSelWriteTranslator::getCurOptFld() const
{
    int selidx = -1;
    if ( selfld_ )
	selidx = selfld_->currentItem();
    else
    {
	if ( !optflds_.isEmpty() )
	    selidx = 0;
    }
    return !optflds_.validIdx(selidx) ? 0
	 : const_cast<uiIOObjTranslatorWriteOpts*>(optflds_[selidx]);
}


void uiIOObjSelWriteTranslator::selChg( CallBacker* )
{
    if ( !selfld_ )
	return;

    const int selidx = selfld_->currentItem();
    if ( !optflds_.validIdx(selidx) )
	return;

    for ( int idx=0; idx<optflds_.size(); idx++ )
    {
	uiIOObjTranslatorWriteOpts* fld = optflds_[idx];
	if ( fld )
	    fld->display( idx == selidx );
    }
}


IOObj* uiIOObjSelWriteTranslator::mkEntry( const char* nm ) const
{
    CtxtIOObj ctio( ctxt_ );
    ctio.ioobj = 0; ctio.setName( nm );
    ctio.fillObj( false, translIdx() );
    if ( ctio.ioobj )
	updatePars( *ctio.ioobj );
    return ctio.ioobj;
}


void uiIOObjSelWriteTranslator::updatePars( IOObj& ioobj ) const
{
    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    if ( fld )
    {
	fld->fill( ioobj.pars() );
	IOM().commitChanges( ioobj );
    }
}


void uiIOObjSelWriteTranslator::use( const IOObj& ioobj )
{
    if ( selfld_ )
	selfld_->setCurrentItem( ioobj.translator() );

    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    if ( fld )
	fld->use( ioobj.pars() );

    selChg( 0 );
}
