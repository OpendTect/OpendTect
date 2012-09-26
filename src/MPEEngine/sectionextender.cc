/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "sectionextender.h"
#include "mpeengine.h"
#include "position.h"

namespace MPE 
{

mImplFactory2Param( SectionExtender, EM::EMObject*, const EM::SectionID&,
		    ExtenderFactory );


SectionExtender::SectionExtender( const EM::SectionID& si)
    : sid_( si )
    , extboundary_( false )
    , excludedpos_( 0 )
    , sortedaddedpos_( false )
    , setundo_( true )
{}


EM::SectionID SectionExtender::sectionID() const { return sid_; }


void SectionExtender::reset()
{
    addedpos_.erase();
    sortedaddedpos_.erase();
    addedpossrc_.erase();
    excludedpos_ = 0;
}


void SectionExtender::setDirection( const BinIDValue& ) {}


const BinIDValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPositions( const TypeSet<EM::SubID> ns )
{
    startpos_ = ns;
    prepareDataIfRequired();
}


void SectionExtender::excludePositions( const TypeSet<EM::SubID>* exclpos )
{ excludedpos_ = exclpos; }


bool SectionExtender::isExcludedPos( const EM::SubID& pos ) const
{
    return excludedpos_ && excludedpos_->indexOf(pos)!=-1;
}


int SectionExtender::nextStep() { return 0; }


#define mExtendDirection(inl,crl,z) \
setDirection(BinIDValue(inl,crl,z)); \
while ( (res=nextStep())>0 )\
    ;\
\
if ( res==-1 ) return 

void SectionExtender::extendInVolume(const BinID& bidstep, float zstep)
{
    int res;
    mExtendDirection(bidstep.inl, 0, 0);
    mExtendDirection(-bidstep.inl, 0, 0);
    mExtendDirection(0, bidstep.crl, 0);
    mExtendDirection(0, -bidstep.crl, 0);
    mExtendDirection(bidstep.inl, bidstep.crl,0);
    mExtendDirection(bidstep.inl, -bidstep.crl,0);
    mExtendDirection(-bidstep.inl, bidstep.crl,0);
    mExtendDirection(-bidstep.inl, -bidstep.crl,0);
    mExtendDirection(0,0,zstep);
    mExtendDirection(0,0,-zstep);
}


const char* SectionExtender::errMsg() const { return errmsg.str(); }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositions() const
{ return addedpos_; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositionsSource() const
{ return addedpossrc_; }


const CubeSampling& SectionExtender::getExtBoundary() const
{ return extboundary_; }


void SectionExtender::setExtBoundary( const CubeSampling& cs )
{ extboundary_ = cs; }


void SectionExtender::unsetExtBoundary() 
{ extboundary_.setEmpty(); }


void SectionExtender::addTarget( const EM::SubID& target,
			         const EM::SubID& src )
{
    if ( sortedaddedpos_.indexOf(target)!=-1 || isExcludedPos(target) )
	return;

    addedpossrc_ += src;
    addedpos_ += target;
    sortedaddedpos_ += target;
}


};



