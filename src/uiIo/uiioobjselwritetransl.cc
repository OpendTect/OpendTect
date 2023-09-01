/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobjselwritetransl.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"

#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "transl.h"


// uiIOObjTranslatorWriteOpts
mImplFactory1Param(uiIOObjTranslatorWriteOpts,uiParent*,
		   uiIOObjTranslatorWriteOpts::factory);


uiIOObjTranslatorWriteOpts::uiIOObjTranslatorWriteOpts( uiParent* p,
							const Translator& trl )
    : uiGroup(p,BufferString("Write options group for ",trl.getDisplayName()))
    , suggestedNameAvailble(this)
    , transl_(trl)
{
}


uiIOObjTranslatorWriteOpts::~uiIOObjTranslatorWriteOpts()
{}



// uiIOObjSelWriteTranslator
uiIOObjSelWriteTranslator::uiIOObjSelWriteTranslator( uiParent* p,
			const CtxtIOObj& ctio,
			const BufferStringSet& transltoavoid, bool withopts )
    : uiGroup(p,"Write Translator selector")
    , suggestedNameAvailble(this)
    , ctxt_(*new IOObjContext(ctio.ctxt_))
{
    optflds_.allowNull( true );
    const TranslatorGroup& trgrp = *ctio.ctxt_.trgroup_;
    const ObjectSet<const Translator>& alltrs = trgrp.templates();
    for ( int idx=0; idx<alltrs.size(); idx++ )
    {
	const Translator* trl = alltrs[idx];
	if ( transltoavoid.indexOf(trl->typeName()) >= 0 )
	    continue;
	else if ( IOObjSelConstraints::isAllowedTranslator(
		    trl->userName(),ctio.ctxt_.toselect_.allowtransls_)
					    && trl->isUserSelectable(false) )
	    trs_ += trl;
    }

    if ( trs_.size() < 1 )
    {
	if ( alltrs.isEmpty() )
	{
	    pErrMsg(BufferString("No '",trgrp.groupName(),
				 "' translator found"));
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
	    if ( fld )
	    {
		mAttachCB( fld->suggestedNameAvailble,
			   uiIOObjSelWriteTranslator::nmAvCB );
		if ( !firstoptfld )
		    firstoptfld = fld;
		if ( selfld_ )
		    fld->attach( alignedBelow, selfld_ );
	    }
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
	lbl_ = new uiLabel( this, tr("Write to"), selfld_ );

    int cur = 0;
    CommandLineParser clp;
    BufferString deftransl;
    clp.getVal( CommandLineParser::sDefTransl(), deftransl );
    for ( int idx=0; idx<trs_.size(); idx++ )
    {
	const Translator& trl = *trs_[idx];
	const BufferString trnm( trl.userName() );
	if ( (ctio.ioobj_ && trnm == ctio.ioobj_->translator()) ||
		    (!deftransl.isEmpty() && trnm == deftransl) )
	    cur = idx;

	selfld_->addItem( toUiString(trnm) );

	BufferString icnm( trl.iconName() );
	if ( !icnm.isEmpty() )
	    selfld_->setIcon( idx, icnm );
    }

    selfld_->setCurrentItem( cur );

    mAttachCB( selfld_->selectionChanged, uiIOObjSelWriteTranslator::selChg );
    mAttachCB( postFinalize(), uiIOObjSelWriteTranslator::selChg );
}


uiIOObjSelWriteTranslator::~uiIOObjSelWriteTranslator()
{
    detachAllNotifiers();
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


void uiIOObjSelWriteTranslator::updateTransFld(
					const BufferStringSet& transltoavoid )
{
    selfld_->setEmpty();
    trs_.setEmpty();
    const TranslatorGroup& trgrp = *ctxt_.trgroup_;
    const ObjectSet<const Translator>& alltrs = trgrp.templates();
    for ( int idx=0; idx<alltrs.size(); idx++ )
    {
	const Translator* trl = alltrs[idx];
	if ( transltoavoid.indexOf(trl->typeName()) >= 0 )
	    continue;

	else if ( IOObjSelConstraints::isAllowedTranslator(
	    trl->userName(),ctxt_.toselect_.allowtransls_)
	    && trl->isUserSelectable(false) )
	    trs_ += trl;
    }

    for ( int idx=0; idx<trs_.size(); idx++ )
    {
	const Translator& trl = *trs_[idx];
	const BufferString trnm( trl.userName() );
	selfld_->addItem( toUiString(trnm) );
	BufferString icnm( trl.iconName() );
	if ( !icnm.isEmpty() )
	    selfld_->setIcon( idx, icnm );
    }

    selfld_->setCurrentItem( 0 );
}


const char* uiIOObjSelWriteTranslator::suggestedName() const
{
    uiIOObjTranslatorWriteOpts* optfld = getCurOptFld();
    return optfld ? optfld->suggestedName() : "";
}


int uiIOObjSelWriteTranslator::translIdx() const
{
    int translidx = -1;
    if ( selfld_ )
    {
	const int selidx = selfld_->currentItem();
	if ( selidx >= 0 )
	    translidx = ctxt_.trgroup_->templates().indexOf( trs_[selidx] );
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
    if ( translidx < 0 && !ctxt_.deftransl_.isEmpty() )
    {
	const Translator* trl = ctxt_.trgroup_->getTemplate(
					    ctxt_.deftransl_, true );
	if ( trl )
	    return trl;
    }

    if ( translidx < 0 )
    {
	translidx = ctxt_.trgroup_->defTranslIdx();
	if ( translidx < 0 )
	    { pErrMsg( "Huh" ); translidx = 0; }
    }

    return ctxt_.trgroup_->templates()[ translidx ];
}


bool uiIOObjSelWriteTranslator::hasSelectedTranslator( const IOObj& ioobj) const
{
    const Translator* trl = selectedTranslator();
    if ( !trl )
	return ioobj.translator().isEmpty();

    return ioobj.translator() == trl->userName()
	&& ioobj.group() == trl->group()->groupName();
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
    ctio.ioobj_ = 0; ctio.setName( nm );
    ctio.fillObj( false, translIdx() );
    if ( ctio.ioobj_ )
	updatePars( *ctio.ioobj_ );
    return ctio.ioobj_;
}


void uiIOObjSelWriteTranslator::updatePars( IOObj& ioobj ) const
{
    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    if ( fld )
    {
	if ( !fld->fill(ioobj.pars()) )
	{
	    uiMSG().error( mToUiStringTodo(fld->errMsg()) );
	    return;
	}

	IOM().commitChanges( ioobj );
    }
}


bool uiIOObjSelWriteTranslator::hasSameWriteOpts(
				const uiIOObjSelWriteTranslator& writetransfld )
{
    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    uiIOObjTranslatorWriteOpts* othfld = writetransfld.getCurOptFld();
    if ( !fld || !othfld )
	return true;

    IOPar thiswritepars, othwritepars;
    fld->fill( thiswritepars );
    othfld->fill( othwritepars );

    return thiswritepars == othwritepars;
}


void uiIOObjSelWriteTranslator::resetPars()
{
    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    const Translator* selectedtrans = selectedTranslator();
    if ( !fld || !selectedtrans || !fld->isPresent(*selectedtrans) )
	return;

    PtrMan<uiIOObjTranslatorWriteOpts> defwriteopts =
		    uiIOObjTranslatorWriteOpts::create( this, *selectedtrans );
    if ( !defwriteopts )
	return;

    IOPar par;
    defwriteopts->fill( par );
    fld->use( par );
}


void uiIOObjSelWriteTranslator::use( const IOObj& ioobj )
{
    if ( selfld_ )
	selfld_->setCurrentItem( ioobj.translator().buf() );

    uiIOObjTranslatorWriteOpts* fld = getCurOptFld();
    if ( fld )
	fld->use( ioobj.pars() );

    selChg( nullptr );
}
