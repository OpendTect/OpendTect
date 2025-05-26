/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswriteopts.h"

#include "datachar.h"
#include "iopar.h"
#include "seiscbvs.h"
#include "seispsioprov.h"

#include "uibutton.h"
#include "uigeninput.h"

#define mCBVSVolTranslInstance mTranslTemplInstance(SeisTrc,CBVS)
#define mCBVSPS3DTranslInstance mTranslTemplInstance(SeisPS3D,CBVS)


// uiCBVSVolOpts

uiCBVSVolOpts::uiCBVSVolOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mCBVSVolTranslInstance)
{
    stortypfld_ = new uiGenInput( this, tr("Storage"),
		StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( int(DataCharacteristics::F32) );

    optimdirfld_ = new uiCheckBox( this, tr("Optimize for Z-slice viewing") );
    optimdirfld_->attach( alignedBelow, stortypfld_ );

    setHAlignObj( stortypfld_ );
}


uiCBVSVolOpts::~uiCBVSVolOpts()
{}


void uiCBVSVolOpts::usePar( const IOPar& iop )
{
    BufferString res = iop.find( sKey::DataStorage() );
    BufferString firstchar(res[0]);
    const int stortyp = res.isEmpty() ? int(DataCharacteristics::F32)
						    : firstchar.toInt();
    stortypfld_->setValue( stortyp );

    res = iop.find( CBVSSeisTrcTranslator::sKeyOptDir() );
    if ( !res.isEmpty() )
    {
	firstchar = res[0];
	optimdirfld_->setChecked( firstchar.isEqual("H") );
    }
}


bool uiCBVSVolOpts::fillPar( IOPar& iop ) const
{
    const int dctyp = stortypfld_->getIntValue();
    iop.set( sKey::DataStorage(), DataCharacteristics::UserTypeNames()[dctyp] );
    iop.update( CBVSSeisTrcTranslator::sKeyOptDir(),
			optimdirfld_->isChecked() ? "Horizontal" : "" );
    return true;
}


uiIOObjTranslatorWriteOpts* uiCBVSVolOpts::create( uiParent* p )
{
    return new uiCBVSVolOpts( p );
}


void uiCBVSVolOpts::initClass()
{
    factory().addCreator( create, mCBVSVolTranslInstance.getDisplayName() );
}


// uiCBVSPS3DOpts

uiCBVSPS3DOpts::uiCBVSPS3DOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mCBVSPS3DTranslInstance)
{
    stortypfld_ = new uiGenInput( this, tr("Storage"),
		 StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( int(DataCharacteristics::F32) );
    setHAlignObj( stortypfld_ );
}


uiCBVSPS3DOpts::~uiCBVSPS3DOpts()
{}


void uiCBVSPS3DOpts::usePar( const IOPar& iop )
{
    const BufferString res = iop.find( sKey::DataStorage() );
    const char* charstrs = res.str();
    const int stortyp = charstrs && *charstrs ? int(*charstrs - '0')
	: int(DataCharacteristics::F32);
    stortypfld_->setValue( stortyp );
}


bool uiCBVSPS3DOpts::fillPar( IOPar& iop ) const
{
    const int dctyp = stortypfld_->getIntValue();
    iop.set( sKey::DataStorage(), DataCharacteristics::UserTypeNames()[dctyp] );
    return true;
}


uiIOObjTranslatorWriteOpts* uiCBVSPS3DOpts::create( uiParent* p )
{
    return new uiCBVSPS3DOpts( p );
}


void uiCBVSPS3DOpts::initClass()
{
    factory().addCreator( create, mCBVSPS3DTranslInstance.getDisplayName() );
}
