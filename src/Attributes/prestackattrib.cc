/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : B.Bril & H.Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: prestackattrib.cc,v 1.2 2008-01-16 16:16:29 cvsbert Exp $";

#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"

#include "seispsprop.h"
#include "seispsioprov.h"
#include "seispsread.h"

namespace Attrib
{

mAttrDefCreateInstance(PreStack)
    
void PreStack::initClass()
{
    mAttrStartInitClass

#define mDefEnumPar(str,enm) \
    epar = new EnumParam( str() ); \
    epar->addEnums( enm##Names ); \
    desc->addParam( epar )

    EnumParam*
    mDefEnumPar(calctypeStr,SeisPSPropCalc::CalcType);
    mDefEnumPar(stattypeStr,Stats::Type);
    mDefEnumPar(lsqtypeStr,SeisPSPropCalc::LSQType);
    mDefEnumPar(valaxisStr,SeisPSPropCalc::AxisType);
    mDefEnumPar(offsaxisStr,SeisPSPropCalc::AxisType);

    BoolParam* useazimpar = new BoolParam( useazimStr() );
    useazimpar->setDefaultValue( false );
    desc->addParam( useazimpar );

    IntParam* ipar = new IntParam( componentStr() );
    ipar->setDefaultValue( 0 );
    ipar->setLimits( Interval<int>(0,mUdf(int)) );
    desc->addParam( ipar );
    ipar = ipar->clone(); ipar->setKey( apertureStr() );
    desc->addParam( ipar );

    desc->addInput( InputSpec("Input Data",true) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


PreStack::PreStack( Desc& ds )
    : Provider(ds)
    , psrdr_(0)
    , propcalc_(0)
{
    if ( !isOK() ) return;

    BufferString str;
    mGetString( str, calctypeStr() );
    setup_.calctype_ = eEnum(SeisPSPropCalc::CalcType,str.buf());
    mGetString( str, stattypeStr() );
    setup_.stattype_ = eEnum(Stats::Type,str.buf());
    mGetString( str, lsqtypeStr() );
    setup_.lsqtype_ = eEnum(SeisPSPropCalc::LSQType,str.buf());
    mGetString( str, offsaxisStr() );
    setup_.offsaxis_ = eEnum(SeisPSPropCalc::AxisType,str.buf());
    mGetString( str, valaxisStr() );
    setup_.valaxis_ = eEnum(SeisPSPropCalc::AxisType,str.buf());

    bool useazim = setup_.useazim_;
    mGetBool( useazim, useazimStr() ); setup_.useazim_ = useazim;
    mGetInt( setup_.component_, componentStr() );
    mGetInt( setup_.aperture_, apertureStr() );
}


PreStack::~PreStack()
{
    delete psrdr_;
    delete propcalc_;
}


bool PreStack::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool PreStack::getInputData( const BinID& relpos, int zintv )
{
    return psrdr_ && propcalc_->goTo( currentbid+relpos );
}


void PreStack::prepPriorToBoundsCalc()
{
}


bool PreStack::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !psrdr_ )
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float val;
	//TODO: how to get val from prestack data?
	//val = getInputValue( *inputdata_, dataidx_, idx, z0 );
	if ( isOutputEnabled(0) )
	    setOutputValue( output, 0, idx, z0, val );
    }

    return true;
}

} // namespace Attrib
