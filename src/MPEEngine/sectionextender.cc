/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "sectionextender.h"

#include "mpeengine.h"
#include "position.h"
#include "trckeyvalue.h"

namespace MPE
{

mImplFactory1Param( SectionExtender, EM::EMObject*, ExtenderFactory )


SectionExtender::SectionExtender()
    : extboundary_(false)
{}


SectionExtender::~SectionExtender()
{}


EM::SectionID SectionExtender::sectionID() const
{ return EM::SectionID::def(); }


void SectionExtender::reset()
{
    addedpos_.erase();
    addedpossrc_.erase();
    excludedpos_ = 0;
}


void SectionExtender::setDirection( const TrcKeyValue& ) {}


const TrcKeyValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPosition( const TrcKey& tk )
{
    startpos_.erase();
    startpos_ += tk;
    prepareDataIfRequired();
}


void SectionExtender::setStartPositions( const TypeSet<TrcKey>& ns )
{
    startpos_ = ns;
    prepareDataIfRequired();
}


void SectionExtender::excludePositions( const TypeSet<TrcKey>* exclpos )
{ excludedpos_ = exclpos; }


bool SectionExtender::isExcludedPos( const TrcKey& pos ) const
{
    return excludedpos_ && excludedpos_->isPresent(pos);
}


int SectionExtender::nextStep() { return 0; }


#define mExtendDirection(inl,crl,z) \
setDirection( TrcKeyValue(BinID(inl,crl),z) ); \
while ( (res=nextStep())>0 )\
    ;\
\
if ( res==-1 ) return

void SectionExtender::extendInVolume(const BinID& bidstep, float zstep)
{
    int res;
    mExtendDirection(  bidstep.inl(),  0,	      0 );
    mExtendDirection( -bidstep.inl(),  0,	      0 );
    mExtendDirection(  0,	       bidstep.crl(), 0 );
    mExtendDirection(  0,	      -bidstep.crl(), 0 );
    mExtendDirection(  bidstep.inl(),  bidstep.crl(), 0 );
    mExtendDirection(  bidstep.inl(), -bidstep.crl(), 0 );
    mExtendDirection( -bidstep.inl(),  bidstep.crl(), 0 );
    mExtendDirection( -bidstep.inl(), -bidstep.crl(), 0 );
    mExtendDirection(  0,	       0,	      zstep );
    mExtendDirection(  0,	       0,	     -zstep );
}


const char* SectionExtender::errMsg() const { return errmsg.str(); }


const TypeSet<TrcKey>& SectionExtender::getAddedPositions() const
{ return addedpos_; }


const TypeSet<TrcKey>& SectionExtender::getAddedPositionsSource() const
{ return addedpossrc_; }


const TrcKeyZSampling& SectionExtender::getExtBoundary() const
{ return extboundary_; }


void SectionExtender::setExtBoundary( const TrcKeyZSampling& cs )
{ extboundary_ = cs; }


void SectionExtender::unsetExtBoundary()
{ extboundary_.setEmpty(); }


void SectionExtender::addTarget( const TrcKey& target, const TrcKey& src )
{
    if ( addedpos_.indexOf(target)!=-1 || isExcludedPos(target) )
	return;

    addedpossrc_ += src;
    addedpos_ += target;
}


float SectionExtender::getDepth( const TrcKey&, const TrcKey& ) const
{ return mUdf(float); }


} // namespace MPE
