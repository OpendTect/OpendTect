/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocmath.h"

#include "keystrs.h"
#include "mathformula.h"


namespace VolProc
{

Math::Math()
{}


Math::~Math()
{
    delete formula_;
}


void Math::setFormula( const char* expression )
{
    deleteAndNullPtr( formula_ );
    if ( StringView(expression).isEmpty() )
	return;

    formula_ = new ::Math::Formula();
    formula_->setText( expression );
}


bool Math::prepareComp( int )
{
    if ( !formula_ || formula_->isBad() )
	return false;

    const int nrformulainputs = formula_->nrInputs();
    const int nrstepinputs = getNrInputs();
    return nrformulainputs == nrstepinputs;
}


void Math::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    BufferString expression;
    if ( formula_ )
	expression = formula_->text();

    par.set( sKey::Formula(), expression.buf() );
}


bool Math::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    BufferString expression;
    if ( !par.get(sKey::Formula(),expression) )
	return false;

    setFormula( expression.buf() );
    return true;
}


bool Math::computeBinID( const BinID& bid, int )
{
    // ToDo:
    // - support no inputs
    // - support multiple inputs
    // - support constants
    // - support special variables

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    if ( !input || input->isEmpty() )
	return false;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() )
	return false;

    const int inputinlidx = input->sampling().inlIdx( bid.inl() );
    const int inputcrlidx = input->sampling().crlIdx( bid.crl() );
    const int outputinlidx = output->sampling().inlIdx( bid.inl() );
    const int outputcrlidx = output->sampling().crlIdx( bid.crl() );
    const int outputzsz = output->sampling().nrZ();

    double inputvals[1];
    for ( int zidx=0; zidx<outputzsz; zidx++ )
    {
	inputvals[0] = input->data().get( inputinlidx, inputcrlidx, zidx );
	const double outval = formula_->getValue( inputvals );
	output->data().set( outputinlidx, outputcrlidx, zidx, outval );
    }

    return true;
}

} // namespace VolProc
