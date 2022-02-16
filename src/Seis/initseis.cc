/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "timedepthconv.h"
#include "seisseqio.h"
#include "segytr.h"
#include "seiscbvs.h"
#include "seisblockstr.h"
#include "seis2dlineio.h"
#include "seispscubetr.h"
#include "segydirecttr.h"
#include "segydirect2d.h"
#include "synthseis.h"
#include "waveletio.h"
#include "seismulticubeps.h"
#include "seispacketinfo.h"
#include "seis2dto3dinterpol.h"
#include "seisposprovider.h"

#include "uistrings.h"

uiString SeisTrcTranslatorGroup::sTypeName( int num )
{ return uiStrings::sVolDataName(false,true,false,false,false); }

defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(CBVS,SeisTrc,"CBVS");
defineTranslator(SEGY,SeisTrc,"SEG-Y");
defineTranslator(TwoD,SeisTrc,"2D");
defineTranslator(TwoDData,SeisTrc,"TwoD DataSet");
defineTranslator(SEGYDirect,SeisTrc,mSEGYDirectTranslNm);
defineTranslator(SeisPSCube,SeisTrc,"PS Cube");
defineTranslator(Blocks,SeisTrc,BlocksSeisTrcTranslator::sKeyTrName());
mDefSimpleTranslators(SyntheticDataPars,"Synthetic Data Parameters",dgb,Seis)

mDefSimpleTranslatorioContext(SeisTrc,Seis)
mDefSimpleTranslatorSelector(SeisTrc);

uiString SeisTrc2DTranslatorGroup::sTypeName( int num )
{ return uiStrings::sVolDataName(true,false,false,false,false); }

defineTranslatorGroup(SeisTrc2D,"2D Seismic Data");
defineTranslator(CBVS,SeisTrc2D,"CBVS");
defineTranslator(SEGYDirect,SeisTrc2D,mSEGYDirectTranslNm);
defineTranslator(SEGYDirect,SurvGeom2D,mSEGYDirectTranslNm);
mDefSimpleTranslatorioContext(SeisTrc2D,Seis)
mDefSimpleTranslatorSelector(SeisTrc2D);

mDefModInitFn(Seis)
{
    mIfNotFirstTime( return );

    SeisPacketInfo::initClass();

    SeisTrcTranslatorGroup::initClass();
    SeisTrc2DTranslatorGroup::initClass();
    SeisPS3DTranslatorGroup::initClass();
    SeisPS2DTranslatorGroup::initClass();
    WaveletTranslatorGroup::initClass();
    dgbWaveletTranslator::initClass();
    Seis::SynthGeneratorBasic::initClass();
    SyntheticDataParsTranslatorGroup::initClass();
    dgbSyntheticDataParsTranslator::initClass();

    // The order here is important!
    // The first one is the default unless explicitly changed.
    CBVSSeisTrcTranslator::initClass();
    TwoDSeisTrcTranslator::initClass();
    TwoDDataSeisTrcTranslator::initClass();
    CBVSSeisTrc2DTranslator::initClass();
    SEGYSeisTrcTranslator::initClass();
    SEGYDirectSeisTrcTranslator::initClass();
    SEGYDirectSeisTrc2DTranslator::initClass();
    SEGYDirectSurvGeom2DTranslator::initClass();
    BlocksSeisTrcTranslator::initClass();

    CBVSSeisPS3DTranslator::initClass();
    CBVSSeisPS2DTranslator::initClass();
    SEGYDirectSeisPS3DTranslator::initClass();
    SEGYDirectSeisPS2DTranslator::initClass();
    SeisPSCubeSeisTrcTranslator::initClass();
    MultiCubeSeisPS3DTranslator::initClass();

    LinearT2DTransform::initClass();
    LinearD2TTransform::initClass();
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();

    FactoryBase& synthgenfact = Seis::SynthGenerator::factory();
    if ( FixedString(synthgenfact.getDefaultName()).isEmpty() )
    {
	const int defidx = synthgenfact.getNames().indexOf(
		Seis::SynthGeneratorBasic::sFactoryKeyword() );
	synthgenfact.setDefaultName( defidx );
    }

    Survey::GMAdmin().updateGeometries( 0 ); //Those using a transl from Seis.

    Seis2DTo3DInterPolImpl::initClass();
    Pos::SeisProvider3D::initClass();
}
