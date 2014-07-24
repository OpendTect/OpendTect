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


#define mDefTranslInstanceFn(typnm,trgrptyp,trtypnm) \
static const Translator& get##typnm##TranslatorInstance() \
{ \
    return *trgrptyp##TranslatorGroup::theInst().getTemplate( \
			trtypnm##trgrptyp##Translator::translKey(), true ); \
}

mDefTranslInstanceFn(CBVSVol,SeisTrc,CBVS)
mDefTranslInstanceFn(CBVSPS3D,SeisPS3D,CBVS)
mDefTranslInstanceFn(SEGYDirectVol,SeisTrc,SEGYDirect)
mDefTranslInstanceFn(SEGYDirectPS3D,SeisPS3D,SEGYDirect)



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


uiCBVSPS3DOpts::uiCBVSPS3DOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,getCBVSPS3DTranslatorInstance())
{
    stortypfld_ = new uiGenInput( this, "Storage",
		 StringListInpSpec(DataCharacteristics::UserTypeNames()) );
    stortypfld_->setValue( (int)DataCharacteristics::Auto );
    setHAlignObj( stortypfld_ );
}


void uiCBVSPS3DOpts::use( const IOPar& iop )
{
    const char* res = iop.find( sKey::DataStorage() );
    if ( res && *res )
	stortypfld_->setValue( (int)(*res - '0') );
}


bool uiCBVSPS3DOpts::fill( IOPar& iop ) const
{
    const int dctyp = stortypfld_->getIntValue();
    iop.set( sKey::DataStorage(), DataCharacteristics::UserTypeNames()[dctyp] );
    return true;
}


void uiCBVSPS3DOpts::initClass()
{
    factory().addCreator( create,
			  getName4Factory(getCBVSPS3DTranslatorInstance()) );
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


#define mNrInlKey SEGYDirectSeisPS3DTranslator::sKeyNrInlPerFile()

uiSEGYDirectPS3DOpts::uiSEGYDirectPS3DOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,getSEGYDirectPS3DTranslatorInstance())
{
    parsfld_ = new uiSEGYFilePars( this, false, 0, false );

    nrinlpfilefld_ = new uiGenInput( this, mNrInlKey, IntInpSpec(100,1) );
    nrinlpfilefld_->attach( alignedBelow, parsfld_ );

    setHAlignObj( parsfld_ );
}



void uiSEGYDirectPS3DOpts::use( const IOPar& iop )
{
    parsfld_->usePar( iop );
    int nrinl = nrinlpfilefld_->getIntValue();
    iop.get( mNrInlKey, nrinl );
    nrinlpfilefld_->setValue( nrinl );
}


bool uiSEGYDirectPS3DOpts::fill( IOPar& iop ) const
{
    parsfld_->fillPar( iop );
    iop.set( mNrInlKey, nrinlpfilefld_->getIntValue() );
    return true;
}


void uiSEGYDirectPS3DOpts::initClass()
{
    factory().addCreator( create,
		      getName4Factory(getSEGYDirectPS3DTranslatorInstance()) );
}
