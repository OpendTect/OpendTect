/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2006
 RCS:           $Id: gapdeconattrib.cc,v 1.4 2006-08-18 15:33:57 cvshelene Exp $
________________________________________________________________________

-*/

#include "gapdeconattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"


/*!> Solves a symmetric Toeplitz linear system of equations rf=g 
     ( finds f given r ( top row of Toeplitz matrix ) 
     		 and g ( right-hand-side column vector ),
	a is the array[systdim] of solution to Ra=v
	(Claerbout, FGDP, p. 57) )
*/
static inline
void solveSymToeplitzsystem(int systdim, float* r, float* g, float* f, float* a)
{
    if ( mIsZero( r[0] ) ) return;

    a[0] = 1;
    float v = r[0];
    f[0] = g[0]/r[0];

    for ( int j=1; j<systdim; j++ )
    {
	a[j] = 0;
	f[j] = 0;
	float tmpvar = 0;		// corresponds to e in Clearbout, FGDP
	for ( int i=0; i<j; i++ )
	    tmpvar += a[i]*r[j-i];
	
	float coef = tmpvar/v;		// corresponds to c in Clearbout, FGDP
	v -= coef*tmpvar;

	for ( int i=0; i<=j/2; i++ )
	{
	    float bot = a[j-i]-coef*a[i];
	    a[i] -= coef*a[j-i];
	    a[j-i] = bot;
	}

	/* use a and v above to get f[i], i = 0,1,2,...,j */
	
	float w;
	for ( int i=0,w=0; i<j; i++ )
	    w += f[i]*r[j-i];
	
	coef = (w-g[j])/v;
	for ( int i=0; i<=j; i++ )
	    f[i] -= coef*a[j-i];
    }
}


namespace Attrib
{
    
mAttrDefCreateInstance(GapDecon)
    
void GapDecon::initClass()
{
    mAttrStartInitClass
	
    IntParam* lagsize = new IntParam( lagsizeStr() );
    desc->addParam( lagsize );
		
    IntParam* gapsize = new IntParam( gapsizeStr() );
    desc->addParam( gapsize );
    
    IntParam* noiselevel = new IntParam( noiselevelStr() );
    desc->addParam( noiselevel );

    IntParam* nrtrcs = new IntParam( nrtrcsStr() );
    desc->addParam( nrtrcs );

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( SI.zRange() );
    desc->addParam( gate );
    
    BoolParam* isinputzerophase = new BoolParam( isinp0phaseStr() );
    isinputzerophase->setDefaultValue( true );
    desc->addParam( isinputzerophase );

    BoolParam* isoutputzerophase = new BoolParam( isout0phaseStr() );
    isoutputzerophase->setDefaultValue( true );
    desc->addParam( isoutputzerophase );

    desc->addInput( InputSpec("Input data",true) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    mAttrEndInitClass
}


GapDecon::GapDecon( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetBool( isinpzerophase_, isinp0phaseStr() );
    mGetBool( isoutzerophase_, isout0phaseStr() );
    mGetInt( lagsize_, lagsizeStr() );
    mGetInt( gapsize_, gapsizeStr() );
    int nrtrcs;
    mGetInt( nrtrcs, nrtrcsStr() );
    stepout_ = BinID( nrtrcs/2, nrtrcs/2 );

    mGetInt( noiselevel_, noiselevelStr() );
}


bool GapDecon::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool GapDecon::getInputData( const BinID& relpos, int zintv )
{
    //rework
    /*
    while ( inputdata_.size() < trcpos_.size() )
	inputdata_ += 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos_.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs[0]->getData( relpos+trcpos_[idx]*bidstep, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
    }
    
    dataidx_ = getDataIndex( 0 );

    steeringdata_ = dosteer_ ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;
*/
    return true;
}


bool GapDecon::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples ) const
{
    if ( !inputdata_.size() ) return false;

    /* compute filter sizes and correlation number */
    if ( !inited_ )
    {
	/* compute filter sizes and correlation number */
	nlag  = lagsize_ / refstep;
	ncorr = gate_.width() / refstep;
	//lcorr = imaxlag[0] + 1;//TODO here only nlag usefull?
    }

    float* wiener = new float[nlag];
    float* spiker = new float[nlag];
    float* autocorr = new float[nlag];//TODO confirm with lcorr
    float* temp = new float[nlag];//TODO idem
    
    crosscorr = autocorr;//diff code source: in this plugin mincorr = minlag

    //TODO :use the multiple trcs to get a "trc avg" which is then used for the 
    //autocorrelation -> why not using the volume statistic attribute to quickly
    // come to the same result? because here we only have 1 lag and 1 gap,
    // thus no use to interpolate and also no weithing of the trcs.

    

    delete wiener; delete spiker; delete autocorr; delete temp;
    return true;
}


const BinID* GapDecon::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }

}; //namespace
