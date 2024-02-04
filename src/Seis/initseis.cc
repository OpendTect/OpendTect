/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "seis2dlineio.h"
#include "seis2dto3dinterpol.h"
#include "seisblockstr.h"
#include "seiscbvs.h"
#include "seismulticubeps.h"
#include "seispacketinfo.h"
#include "seisposprovider.h"
#include "seispscubetr.h"
#include "seisseqio.h"
#include "segydirect2d.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "synthseis.h"
#include "timedepthconv.h"
#include "waveletio.h"
#include "uistrings.h"

using intFromVoidFn = int(*)();
using voidFromuiStringFn = void(*)(uiString&,TaskRunner*);
mGlobal(General) void setConv2DSeis_General_Fns(intFromVoidFn,
						voidFromuiStringFn);
extern int Seis_Get_2D_Data_Conversion_Status();
extern void Seis_Convert_2DLineSets_To_2DDataSets(uiString&,TaskRunner*);

uiString SeisTrcTranslatorGroup::sTypeName( int num )
{ return uiStrings::sVolDataName(false,true,false,false,false); }

namespace Seis
{

static void seisUpdateGeoms( CallBacker* )
{
    Survey::GMAdmin().updateGeometries( nullptr );
    //Those using a transl from Seis.
}

static int Get_2D_Data_Conversion_Status()
{
    return Seis_Get_2D_Data_Conversion_Status();
}

static void Convert_2DLineSets_To_2DDataSets( uiString& msg,
					      TaskRunner* taskrunner )
{
    Seis_Convert_2DLineSets_To_2DDataSets( msg, taskrunner );
}

} // namespace Seis

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
    if ( StringView(synthgenfact.getDefaultName()).isEmpty() )
    {
	const int defidx = synthgenfact.getNames().indexOf(
		Seis::SynthGeneratorBasic::sFactoryKeyword() );
	synthgenfact.setDefaultName( defidx );
    }

    if ( NeedDataBase() )
    {
	if ( IOMan::isOK() )
	    Seis::seisUpdateGeoms( nullptr );
	else
	    IOMan::iomReady().notify( mSCB(Seis::seisUpdateGeoms) );

	setConv2DSeis_General_Fns( Seis::Get_2D_Data_Conversion_Status,
				   Seis::Convert_2DLineSets_To_2DDataSets );
    }

    Seis2DTo3DInterPolImpl::initClass();
    Pos::SeisProvider3D::initClass();

    Seis::addPluginSeisTrcTranslators();
}
