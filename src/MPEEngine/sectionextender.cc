/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionextender.cc,v 1.3 2005-08-05 01:37:57 cvsduntao Exp $";

#include "sectionextender.h"

namespace MPE 
{


SectionExtender::SectionExtender( const EM::SectionID& si)
    : sectionid( si )
    , trkstattbl(0)
{}


EM::SectionID SectionExtender::sectionID() const { return sectionid; }


void SectionExtender::reset()
{
    addedpos.erase();
    addedpossrc.erase();
    trkstattbl = 0;
}


void SectionExtender::setDirection( const BinIDValue& ) {}


const BinIDValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPositions( const TypeSet<EM::SubID> ns )
{ startpos = ns; }


int SectionExtender::nextStep() { return 0; }


const char* SectionExtender::errMsg() const { return errmsg[0] ? errmsg : 0; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositions() const
{ return addedpos; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositionsSource() const
{ return addedpossrc; }


void SectionExtender::addTarget( const EM::SubID& target,
			         const EM::SubID& src )
{
    if ( addedpos.indexOf(target)!=-1 )
	return;

    addedpossrc += src;
    addedpos += target;
}


};



