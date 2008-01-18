/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : B.Bril & H.Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: prestackattrib.cc,v 1.4 2008-01-18 11:37:02 cvsbert Exp $";

#include "prestackattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"

#include "seispsprop.h"
#include "seispsioprov.h"
#include "seispsread.h"

#include "ioman.h"
#include "ioobj.h"


namespace Attrib
{

mAttrDefCreateInstance(PreStack)
    
void PreStack::initClass()
{
    mAttrStartInitClass

    desc->addParam( new SeisStorageRefParam("id") );

#define mDefEnumPar(var,typ) \
    epar = new EnumParam( var##Str() ); \
    epar->addEnums( typ##Names ); \
    desc->addParam( epar )

    EnumParam*
    mDefEnumPar(calctype,SeisPSPropCalc::CalcType);
    mDefEnumPar(stattype,Stats::Type);
    mDefEnumPar(lsqtype,SeisPSPropCalc::LSQType);
    mDefEnumPar(valaxis,SeisPSPropCalc::AxisType);
    mDefEnumPar(offsaxis,SeisPSPropCalc::AxisType);

    desc->addParam( new BoolParam( useazimStr(), false, false ) );
    IntParam* ipar = new IntParam( componentStr(), 0 , false );
    ipar->setLimits( Interval<int>(0,mUdf(int)) );
    desc->addParam( ipar );
    ipar = ipar->clone(); ipar->setKey( apertureStr() );
    desc->addParam( ipar );

    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


PreStack::PreStack( Desc& ds )
    : Provider(ds)
    , psrdr_(0)
    , propcalc_(0)
{
    if ( !isOK() ) return;

    const char* res; mGetString(res,"id") psid_ = res;

#define mGetSetupEnumPar(var,typ) \
    int tmp_##var = (int)setup_.var##_; \
    mGetEnum(tmp_##var,var##Str()); \
    setup_.var##_ = (typ)tmp_##var

    mGetSetupEnumPar(calctype,SeisPSPropCalc::CalcType);
    mGetSetupEnumPar(stattype,Stats::Type);
    mGetSetupEnumPar(lsqtype,SeisPSPropCalc::LSQType);
    mGetSetupEnumPar(valaxis,SeisPSPropCalc::AxisType);
    mGetSetupEnumPar(offsaxis,SeisPSPropCalc::AxisType);

    bool useazim = setup_.useazim_;
    mGetBool( useazim, useazimStr() ); setup_.useazim_ = useazim;
    mGetInt( setup_.component_, componentStr() );
    mGetInt( setup_.aperture_, apertureStr() );
}


PreStack::~PreStack()
{
    delete propcalc_;
    delete psrdr_;
}


bool PreStack::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool PreStack::getInputData( const BinID& relpos, int zintv )
{
    return psrdr_ && propcalc_->goTo( currentbid+relpos );
}


#define mErrRet(s1,s2,s3) { errmsg = BufferString(s1,s2,s3); return; }

void PreStack::prepPriorToBoundsCalc()
{
    IOObj* ioobj = IOM().get( psid_ );
    if ( !ioobj )
	mErrRet("Cannot find pre-stack data store ",psid_," in object manager")

    psrdr_ = SPSIOPF().getReader( *ioobj );
    delete ioobj;
    if ( !psrdr_ )
	mErrRet("Cannot create reader for ",psid_," pre-stack data store")
    const char* emsg = psrdr_->errMsg();
    if ( emsg && *emsg ) mErrRet("PS Reader: ",emsg,"")

    propcalc_ = new SeisPSPropCalc( *psrdr_, setup_ );
}


bool PreStack::computeData( const DataHolder& output, const BinID& relpos,
			  int z0, int nrsamples, int threadid ) const
{
    if ( !propcalc_ )
	return false;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float z = (z0 + idx) * refstep;
	setOutputValue( output, 0, idx, z0, propcalc_->getVal(z) );
    }

    return true;
}

} // namespace Attrib
