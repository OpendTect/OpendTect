/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "expdipview.h"
#include "attribprovider.h"
#include "seistrc.h"

#include <math.h>

#define M_360_2_PI       57.2957795131   // 360/(2*PI)

DipViewAttrib::DipViewAttrib( Parameters* params )
    : aspect( params->aspect )
    , AttribCalc( new DipViewAttrib::Task( *this ) )
{ 
    params->fillDefStr( desc );
    delete params;
    AttribInputSpec* spec = new AttribInputSpec;
    spec->setDesc( "Inline dipangle");
    spec->forbiddenDts += Seis::Ampl;
    spec->forbiddenDts += Seis::Dip;
    spec->forbiddenDts += Seis::Frequency;
    spec->forbiddenDts += Seis::Phase;
    spec->forbiddenDts += Seis::AVOGradient;
    inputspec += spec;

    spec = new AttribInputSpec;
    spec->setDesc( "Crossline dipangle");
    spec->forbiddenDts += Seis::Ampl;
    spec->forbiddenDts += Seis::Dip;
    spec->forbiddenDts += Seis::Frequency;
    spec->forbiddenDts += Seis::Phase;
    spec->forbiddenDts += Seis::AVOGradient;
    inputspec += spec;

    prepareInputs();
}


DipViewAttrib::~DipViewAttrib( )
{ }


AttribCalc::Task* DipViewAttrib::Task::clone() const
{ return new DipViewAttrib::Task(calculator); }


bool DipViewAttrib::Task::Input::set( const BinID& pos,
			     const ObjectSet<AttribProvider>& inputproviders,
			     const TypeSet<int>& inputattribs,
			     const TypeSet<float*>&)
{
    inldip = inputproviders[0]->getTrc(pos.inl, pos.crl);
    crldip = inputproviders[1]->getTrc(pos.inl, pos.crl);

    if ( !inldip || !crldip ) return false;

    inlattrib = inputproviders[0]->attrib2component( inputattribs[0] );
    crlattrib = inputproviders[1]->attrib2component( inputattribs[1] );

    return true;
}


int DipViewAttrib::Task::nextStep()
{
    if ( !outp ) return 0;

    const DipViewAttrib::Task::Input* inp = 
			(const DipViewAttrib::Task::Input*) input;

    const SeisTrc* inldip = inp->inldip;
    const SeisTrc* crldip = inp->crldip;
    const int inlattrib = inp->inlattrib;
    const int crlattrib = inp->crlattrib;

    BinID aspect = calculator.aspect;

    for ( int idx=0; idx<nrtimes; idx++ )
    {
	const float curt = t1 + idx*step;
	float inldipval = inldip->getValue(curt,inlattrib) / M_360_2_PI;
	float crldipval = crldip->getValue(curt,crlattrib) / M_360_2_PI;

	float a1 = -1;			// One vector in the dipplane
	float b1 = -1;
	float c1 = a1 * sin( inldipval ) + b1*sin( crldipval );

	float a2 = -1;			// Another vector in the dipplane
	float b2 = 1;
	float c2 = a2*sin( inldipval ) + b2*sin( crldipval );

	float a3 = b1*c2-b2*c1;		// The planes normal vector
	float b3 = -a1*c2+a2*c1;
	float c3 = a1*b2-a2*b1;

	if ( c3 < 0 )
	{
	    a3 = -a3;
	    b3 = -b3;
	    c3 = -c3;
	}

	float a4 = 0;			// The lights vector
	float b4 = 0;
	float c4 = 1;

	if ( aspect.inl!=90 )
	{
	    a4 = cos( aspect.crl / M_360_2_PI );
	    b4 = sin( aspect.crl / M_360_2_PI );
	    c4 = tan( aspect.inl / M_360_2_PI );
	}

	float val = 1 - (a3*a4+b3*b4+c3*c4)/ Math::Sqrt( (a3*a3+b3*b3+c3*c3) *
					  (a4*a4+b4*b4+c4*c4 ));	
	outp[idx] = val > 0 ? val : 0;
    }

    return 0;
}
