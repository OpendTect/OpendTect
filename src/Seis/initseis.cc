/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "timedepthconv.h"
#include "seisseqio.h"
#include "segytr.h"
#include "seiscbvs.h"
#include "seis2dlineio.h"
#include "seispscubetr.h"
#include "segydirecttr.h"
#include "wavelet.h"
#include "seismulticubeps.h"
#include "seispacketinfo.h"


#define sKeySeisTrcTranslatorGroup "Seismic Data"
defineTranslatorGroup(SeisTrc,sKeySeisTrcTranslatorGroup);
defineTranslator(CBVS,SeisTrc,"CBVS");
defineTranslator(SEGY,SeisTrc,"SEG-Y");
defineTranslator(TwoD,SeisTrc,"2D");
defineTranslator(TwoDData,SeisTrc,"TwoD DataSet");
defineTranslator(SEGYDirect,SeisTrc,"SEGYDirect");
defineTranslator(SeisPSCube,SeisTrc,"PS Cube");

mDefSimpleTranslatorSelector(SeisTrc,sKeySeisTrcTranslatorGroup)
mDefSimpleTranslatorioContext(SeisTrc,Seis)

#define sKeySeisTrc2DTranslatorGroup "2D Seismic Data"
defineTranslatorGroup(SeisTrc2D,sKeySeisTrc2DTranslatorGroup);
defineTranslator(CBVS,SeisTrc2D,"CBVS");

mDefSimpleTranslatorSelector(SeisTrc2D,sKeySeisTrc2DTranslatorGroup)
mDefSimpleTranslatorioContext(SeisTrc2D,Seis)

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

    // The order here is important!
    // The first one is the default unless explicitly changed.
    CBVSSeisTrcTranslator::initClass();
    TwoDSeisTrcTranslator::initClass();
    TwoDDataSeisTrcTranslator::initClass();
    CBVSSeisTrc2DTranslator::initClass();
    SEGYSeisTrcTranslator::initClass();
    SEGYDirectSeisTrcTranslator::initClass();
    SEGYDirectSeisPS3DTranslator::initClass();
    SEGYDirectSeisPS2DTranslator::initClass();
    SeisPSCubeSeisTrcTranslator::initClass();
    CBVSSeisPS3DTranslator::initClass();
    CBVSSeisPS2DTranslator::initClass();
    MultiCubeSeisPS3DTranslator::initClass();

    LinearT2DTransform::initClass();
    LinearD2TTransform::initClass();
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
