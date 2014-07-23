/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiswriteopts.h"
#include "seiscbvs.h"
#include "segydirecttr.h"
#include "datachar.h"
#include "iopar.h"
#include "uigeninput.h"
#include "uisegydef.h"


static const Translator& getCBVSVolTranslatorInstance()
{
    return *SeisTrcTranslatorGroup::theInst().getTemplate(
			CBVSSeisTrcTranslator::translKey(), true );
}

static const Translator& getSEGYDirectVolTranslatorInstance()
{
    return *SeisTrcTranslatorGroup::theInst().getTemplate(
			SEGYDirectSeisTrcTranslator::translKey(), true );
}


uiCBVSVolOpts::uiCBVSVolOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,getCBVSVolTranslatorInstance())
{
    stortypfld_ = new uiGenInput( this, "Storage",
		 StringListInpSpec(DataCharacteristics::UserTypeNames()) );
    stortypfld_->setValue( (int)DataCharacteristics::Auto );

    optimdirfld_ = new uiGenInput( this, "Optimize horizontal slice access",
				   BoolInpSpec(false) );
    optimdirfld_->attach( alignedBelow, stortypfld_ );

    setHAlignObj( stortypfld_ );
}


void uiCBVSVolOpts::use( const IOPar& iop )
{
    const char* res = iop.find( sKey::DataStorage() );
    if ( res && *res )
	stortypfld_->setValue( (int)(*res - '0') );

    res = iop.find( CBVSSeisTrcTranslator::sKeyOptDir() );
    if ( res && *res )
	optimdirfld_->setValue( *res == 'H' );
}


bool uiCBVSVolOpts::fill( IOPar& iop ) const
{
    const int dctyp = stortypfld_->getIntValue();
    iop.set( sKey::DataStorage(), DataCharacteristics::UserTypeNames()[dctyp] );

    iop.update( CBVSSeisTrcTranslator::sKeyOptDir(),
			optimdirfld_->getBoolValue() ? "Horizontal" : "" );
    return true;
}


void uiCBVSVolOpts::initClass()
{
    factory().addCreator( create,
			  getName4Factory(getCBVSVolTranslatorInstance()) );
}



uiSEGYDirectVolOpts::uiSEGYDirectVolOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,getSEGYDirectVolTranslatorInstance())
{
    parsfld_ = new uiSEGYFilePars( this, false, 0, false );

    setHAlignObj( parsfld_ );
}


void uiSEGYDirectVolOpts::use( const IOPar& iop )
{
    parsfld_->usePar( iop );
}


bool uiSEGYDirectVolOpts::fill( IOPar& iop ) const
{
    parsfld_->fillPar( iop );
    return true;
}


void uiSEGYDirectVolOpts::initClass()
{
    factory().addCreator( create,
		      getName4Factory(getSEGYDirectVolTranslatorInstance()) );
}
