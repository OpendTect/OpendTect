/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "expnearsubtract.h"
#include "attribprovider.h"
#include "seistrc.h"
#include "attribdescsetproc.h"

#include <math.h>

const BinID NearSubtractAttrib::stepout = BinID( 1, 1 );

NearSubtractAttrib::NearSubtractAttrib( Parameters* params )
    : usedip( params->usedip )
    , relampl( params->relampl )
    , common( 0 )
    , AttribCalc( new NearSubtractAttrib::Task( *this ) )
{ 
    params->fillDefStr( desc );
    delete params;

    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc(
	"Data on which the NearSubtract should be calculated");
    inputspec += spec;

    if ( usedip )
    {
	spec = new AttribInputSpec;
	spec->setDesc("Inline dip");
	spec->forbiddenDts += Seis::Ampl;
	spec->forbiddenDts += Seis::Frequency;
	spec->forbiddenDts += Seis::Phase;
	spec->forbiddenDts += Seis::AVOGradient;
	spec->forbiddenDts += Seis::UnknowData;
	inputspec += spec;

	spec = new AttribInputSpec;
	spec->setDesc("Crossline dip");
	spec->forbiddenDts += Seis::Ampl;
	spec->forbiddenDts += Seis::Frequency;
	spec->forbiddenDts += Seis::Phase;
	spec->forbiddenDts += Seis::AVOGradient;
	spec->forbiddenDts += Seis::UnknowData;
	inputspec += spec;
    }

    prepareInputs();
}


NearSubtractAttrib::~NearSubtractAttrib( )
{
}


bool NearSubtractAttrib::init()
{
    inldist = common->inldist * common->stepoutstep.inl / dipFactor();
    crldist = common->crldist * common->stepoutstep.crl / dipFactor();

    return AttribCalc::init();
}


AttribCalc::Task* NearSubtractAttrib::Task::clone() const
{ return new NearSubtractAttrib::Task(calculator); }


bool NearSubtractAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    trc0 = inputproviders[0]->getTrc(pos.inl, pos.crl);
    trc1 = inputproviders[0]->getTrc(pos.inl, pos.crl+1);

    if ( !trc0 || !trc1 ) return false;

    trc0attrib = inputproviders[0]->attrib2component( inputattribs[0] );
    trc1attrib = inputproviders[1]->attrib2component( inputattribs[1] );

    if ( calculator.usedip )
    {
	inldiptrc = inputproviders[1]->getTrc(pos.inl, pos.crl);
	crldiptrc = inputproviders[2]->getTrc(pos.inl, pos.crl);

	if ( !inldiptrc || !crldiptrc )
	    return false;

	inldiptrcattrib = inputproviders[2]->attrib2component( inputattribs[2] );
	crldiptrcattrib = inputproviders[3]->attrib2component( inputattribs[3] );
    }

    return true;
}


int NearSubtractAttrib::Task::nextStep()
{
    const NearSubtractAttrib::Task::Input* inp = 
			(const NearSubtractAttrib::Task::Input*) input;

    const SeisTrc* trc0 = inp->trc0;
    const SeisTrc* trc1 = inp->trc1;
//    const SeisTrc* inldiptrc = inp->inldiptrc;
    const SeisTrc* crldiptrc = inp->crldiptrc;

//    const float inldist = calculator.inldist;
    const float crldist = calculator.crldist;

    const int trc0attrib = inp->trc0attrib;;
    const int trc1attrib = inp->trc1attrib;
//    const int inldiptrcattrib = inp->inldiptrcattrib;
    const int crldiptrcattrib = inp->crldiptrcattrib;

    bool usedip = calculator.usedip;
    bool relampl = calculator.relampl;

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	float t2 = curt;

	if ( usedip )
	{
	    t2 += crldiptrc->getValue( curt, crldiptrcattrib ) * crldist;
	}

	float val0 = trc0->getValue( curt, trc0attrib );
	float val1 = trc1->getValue( t2, trc1attrib );
	float diff = val0 - val1;

	if ( relampl )
	{
	    float numerator = fabs( val0 ) + fabs( val1 );

	    if ( mIsZero(numerator,mDefEps) ) outp[idx] = 0;
	    else outp[idx] = diff / numerator;
	}
	else
	    outp[idx] = diff;
    }

    return 0;
}
