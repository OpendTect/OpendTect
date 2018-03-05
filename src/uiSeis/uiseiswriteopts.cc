/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/

#include "uiseiswriteopts.h"

#include "datachar.h"
#include "iopar.h"
#include "seiscbvs.h"
#include "seisblockstr.h"
#include "seiscbvsps.h"
#include "seispsioprov.h"

#include "uibutton.h"
#include "uigeninput.h"

#define mCBVSVolTranslInstance mTranslTemplInstance(SeisTrc,CBVS)
#define mSeisBlocksTranslInstance mTranslTemplInstance(SeisTrc,Blocks)
#define mCBVSPS3DTranslInstance mTranslTemplInstance(SeisPS3D,CBVS)


uiCBVSVolOpts::uiCBVSVolOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mCBVSVolTranslInstance)
{
    stortypfld_ = new uiGenInput( this, uiStrings::sStorage(),
		StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( (int)OD::AutoDataRep );

    optimdirfld_ =
		new uiCheckBox(this,
			       tr("Optimize for Z-slice viewing"));
    optimdirfld_->attach( alignedBelow, stortypfld_ );

    setHAlignObj( stortypfld_ );
}


void uiCBVSVolOpts::use( const IOPar& iop )
{
    DataCharacteristics::UserType ut;
    if ( DataCharacteristics::getUserTypeFromPar(iop,ut) )
	stortypfld_->setValue( (int)ut );

    const char* res = iop.find( CBVSSeisTrcTranslator::sKeyOptDir() );
    if ( res && *res )
	optimdirfld_->setChecked( *res == 'H' );
}


bool uiCBVSVolOpts::fill( IOPar& iop ) const
{
    DataCharacteristics::putUserTypeToPar( iop,
	    DataCharacteristics::UserType( stortypfld_->getIntValue() ) );

    iop.update( CBVSSeisTrcTranslator::sKeyOptDir(),
			optimdirfld_->isChecked() ? "Horizontal" : "" );
    return true;
}


void uiCBVSVolOpts::initClass()
{
    factory().addCreator( create, mCBVSVolTranslInstance.getDisplayName() );
}


uiSeisBlocksOpts::uiSeisBlocksOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mSeisBlocksTranslInstance)
{
    stortypfld_ = new uiGenInput( this, uiStrings::sStorage(),
		StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( (int)OD::AutoDataRep );

    setHAlignObj( stortypfld_ );
}


void uiSeisBlocksOpts::use( const IOPar& iop )
{
    DataCharacteristics::UserType ut;
    if ( DataCharacteristics::getUserTypeFromPar(iop,ut) )
	stortypfld_->setValue( (int)ut );
}


bool uiSeisBlocksOpts::fill( IOPar& iop ) const
{
    DataCharacteristics::putUserTypeToPar( iop,
	    DataCharacteristics::UserType( stortypfld_->getIntValue() ) );
    return true;
}


void uiSeisBlocksOpts::initClass()
{
    factory().addCreator( create, mSeisBlocksTranslInstance.getDisplayName() );
}


uiCBVSPS3DOpts::uiCBVSPS3DOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mCBVSPS3DTranslInstance)
{
    stortypfld_ = new uiGenInput( this, uiStrings::sStorage(),
		 StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( (int)OD::AutoDataRep );
    setHAlignObj( stortypfld_ );
}


void uiCBVSPS3DOpts::use( const IOPar& iop )
{
    DataCharacteristics::UserType ut;
    if ( DataCharacteristics::getUserTypeFromPar(iop,ut) )
	stortypfld_->setValue( (int)ut );
}


bool uiCBVSPS3DOpts::fill( IOPar& iop ) const
{
    DataCharacteristics::putUserTypeToPar( iop,
	    DataCharacteristics::UserType( stortypfld_->getIntValue() ) );
    return true;
}


void uiCBVSPS3DOpts::initClass()
{
    factory().addCreator( create, mCBVSPS3DTranslInstance.getDisplayName() );
}
