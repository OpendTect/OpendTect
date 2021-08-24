/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2015
________________________________________________________________________

-*/

#include "uisegywriteopts.h"

#include "iopar.h"
#include "segydirecttr.h"

#include "uigeninput.h"
#include "uisegydef.h"

#define mSEGYDirectVolTranslInstance mTranslTemplInstance(SeisTrc,SEGYDirect)
#define mSEGYDirectPS3DTranslInstance mTranslTemplInstance(SeisPS3D,SEGYDirect)


uiSEGYDirectVolOpts::uiSEGYDirectVolOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mSEGYDirectVolTranslInstance)
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
    factory().addCreator( create, mSEGYDirectTranslNm );
}


#define mNrInlKey SEGYDirectSeisPS3DTranslator::sKeyNrInlPerFile()

uiSEGYDirectPS3DOpts::uiSEGYDirectPS3DOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mSEGYDirectPS3DTranslInstance)
{
    parsfld_ = new uiSEGYFilePars( this, false, 0, false );

    nrinlpfilefld_ = new uiGenInput( this, tr("Number of inlines per file"),
				     IntInpSpec(100,1) );
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
    factory().addCreator( create, mSEGYDirectTranslNm );
}
