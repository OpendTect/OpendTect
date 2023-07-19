/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiioobjinserter.h"

#include "ctxtioobj.h"
#include "iopar.h"
#include "transl.h"

#include "uimenu.h"
#include "uitoolbutton.h"
#include "settings.h"


// uiIOObjInserter
mImplFactory(uiIOObjInserter,uiIOObjInserter::factory);

uiIOObjInserter::uiIOObjInserter( const Translator& trl )
    : objInserterd(this)
    , transl_(trl)
    , ctxt_(*new IOObjContext(nullptr))
{}


uiIOObjInserter::~uiIOObjInserter()
{}


bool uiIOObjInserter::isPresent( const TranslatorGroup& grp )
{
    const ObjectSet<const Translator>& tpls = grp.templates();

    for ( int idx=0; idx<tpls.size(); idx++ )
	if ( isPresent(*tpls[idx]) )
	    return true;

    return false;
}


static int getEnabledInserters( BufferStringSet& sel )
{
    Settings::common().get( "Ui.Inserters.Enable", sel );
    if ( sel.isEmpty() || sel.get(0) == sKey::All() )
	return 1;

    return sel.get(0) == sKey::None() ? -1 : 0;
}


bool uiIOObjInserter::allDisabled()
{
    BufferStringSet dum;
    return getEnabledInserters(dum) == -1;
}


bool uiIOObjInserter::isDisabled() const
{
    BufferStringSet enabled;
    const int res = getEnabledInserters( enabled );
    if ( res != 0 )
	return res < 0;

    return !enabled.isPresent( name() );
}


void uiIOObjInserter::setIOObjCtxt( const IOObjContext& ctio )
{
    ctxt_ = ctio;
}


uiButton* uiIOObjInserter::createInsertButton( uiParent* p,
					const CtxtIOObj& ctio,
					ObjectSet<uiIOObjInserter>& insertset,
					const BufferStringSet& transltoavoid )
{
    if ( uiIOObjInserter::allDisabled() )
	return nullptr;

    ManagedObjectSet<uiToolButtonSetup> tbsetups;
    const ObjectSet<const Translator>& tpls = ctio.ctxt_.trgroup_->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
    {
	const BufferString trgrpnm = tpls[idx]->typeName();
	if ( transltoavoid.isPresent(trgrpnm.buf()) )
	    continue;

	uiIOObjInserter* inserter = uiIOObjInserter::create( *tpls[idx] );
	if ( !inserter || inserter->isDisabled() )
	{
	    delete inserter;
	    continue;
	}



	inserter->setIOObjCtxt( ctio.ctxt_ );
	uiToolButtonSetup* tbsu = inserter->getButtonSetup();
	if ( !tbsu )
	{
	    delete inserter;
	    continue;
	}

	insertset.add( inserter );
	tbsetups.add( tbsu );
    }

    if ( tbsetups.isEmpty() )
	return nullptr;

    if ( tbsetups.size() == 1 )
	return new uiToolButton( p, *tbsetups.first() );

    auto* menu = new uiMenu( nullptr );
    auto* tb = new uiToolButton( p, "inserter", toUiString("Import from"),
				 CallBack() );
    for ( auto* tbsu : tbsetups )
    {
	auto* item = new uiAction( tbsu->tooltip_, tbsu->cb_, tbsu->icid_ );
	menu->insertAction( item );
    }

    tb->setMenu( menu, uiToolButton::InstantPopup );
    return tb;
}


void uiIOObjInserter::addInsertersToDlg( uiParent* p,
					 CtxtIOObj& ctio,
					 ObjectSet<uiIOObjInserter>& insertset,
					 ObjectSet<uiButton>& buttonset,
					 const BufferStringSet& transltoavoid )
{
    if ( uiIOObjInserter::allDisabled() )
	return;

    const ObjectSet<const Translator>& tpls
			= ctio.ctxt_.trgroup_->templates();
    for ( int idx=0; idx<tpls.size(); idx++ )
    {
	uiIOObjInserter* inserter = uiIOObjInserter::create( *tpls[idx] );
	const BufferString trgrpnm = tpls[idx]->typeName();
	if ( !inserter || inserter->isDisabled() ||
				(transltoavoid.indexOf(trgrpnm)>=0) )
	    continue;

	inserter->setIOObjCtxt( ctio.ctxt_ );
	uiToolButtonSetup* tbsu = inserter->getButtonSetup();
	if ( !tbsu )
	    { delete inserter; continue; }

	uiButton* but = tbsu->getButton( p, true );
	if ( but )
	    buttonset += but;

	delete tbsu;
	insertset += inserter;
    }
}
