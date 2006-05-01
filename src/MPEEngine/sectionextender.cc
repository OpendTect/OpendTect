/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionextender.cc,v 1.8 2006-05-01 17:28:20 cvskris Exp $";

#include "sectionextender.h"
#include "mpeengine.h"
#include "position.h"

namespace MPE 
{


SectionExtender::SectionExtender( const EM::SectionID& si)
    : sid_( si )
    , extboundary_( false )
{}


EM::SectionID SectionExtender::sectionID() const { return sid_; }


void SectionExtender::reset()
{
    addedpos_.erase();
    addedpossrc_.erase();
}


void SectionExtender::setDirection( const BinIDValue& ) {}


const BinIDValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPositions( const TypeSet<EM::SubID> ns )
{ startpos_ = ns; }


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


const char* SectionExtender::errMsg() const { return errmsg[0] ? errmsg : 0; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositions() const
{ return addedpos_; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositionsSource() const
{ return addedpossrc_; }


const CubeSampling& SectionExtender::getExtBoundary() const
{ return extboundary_.isEmpty() ? engine().activeVolume() : extboundary_; }


void SectionExtender::setExtBoundary( const CubeSampling& cs )
{ extboundary_ = cs; }


void SectionExtender::unsetExtBoundary() 
{ extboundary_.setEmpty(); }


void SectionExtender::addTarget( const EM::SubID& target,
			         const EM::SubID& src )
{
    if ( addedpos_.indexOf(target)!=-1 )
	return;

    addedpossrc_ += src;
    addedpos_ += target;
}


};



