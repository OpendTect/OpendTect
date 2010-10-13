/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID = "$Id: stratseqgen.cc,v 1.1 2010-10-13 11:27:57 cvsbert Exp $";

#include "stratlayseqgendesc.h"
#include "stratlaymodgen.h"
#include "stratlayermodel.h"
#include "ascstream.h"
#include "keystrs.h"
#include "iopar.h"

static const char* sKeyFileType = "Layer Sequence Generator Description";
mImplFactory(Strat::LayerGenDesc,Strat::LayerGenDesc::factory)


Strat::LayerModelGenerator::LayerModelGenerator(
		const Strat::LayerSequenceGenDesc& desc, Strat::LayerModel& lm )
    : Executor("Layer Sequence Generator")
    , desc_(desc)
    , lm_(lm)
    , nrseqs_(1)
    , seqnr_(0)
{
    reset();
}


void Strat::LayerModelGenerator::reset()
{
    lm_.setEmpty();
    seqnr_ = 0;
    msg_ = "Generating layer sequences";
}


int Strat::LayerModelGenerator::nextStep()
{
    const float modpos = nrseqs_ < 2 ? 0.5 : ((float)seqnr_)/(nrseqs_-1);
    desc_.generate( lm_.addSequence(), modpos );
    seqnr_++;
    return seqnr_ >= nrseqs_ ? Finished() : MoreToDo();
}


Strat::LayerGenDesc* Strat::LayerGenDesc::get( const IOPar& iop )
{
    Strat::LayerGenDesc* ret = factory().create( iop.find(sKey::Type) );
    if ( !ret ) return 0;
    ret->usePar( iop );
    return ret;
}


bool Strat::LayerSequenceGenDesc::getFrom( std::istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	{ errmsg_ = "Bad header found"; return false; }

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);
	if ( iop.isEmpty() ) continue;
	*this += LayerGenDesc::get( iop );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::putTo( std::ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	{ errmsg_ = "Cannot write file header"; return false; }

    for ( int idx=0; idx<size(); idx++ )
    {
	IOPar iop; (*this)[idx]->fillPar(iop);
	iop.putTo( astrm );
    }

    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty(); warnmsgs_.erase();

    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenDesc& lgd = *((*this)[idx]);
	if ( !lgd.genMaterial(ls,modpos) )
	{
	    errmsg_ = lgd.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_.add( "Error generating " )
		       .add( lgd.userIdentification() );
	    return false;
	}
	else if ( lgd.warnMsg() )
	    warnmsgs_.addIfNew( lgd.warnMsg() );
    }

    return true;
}
