/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionextender.cc,v 1.1 2005-07-22 10:11:28 cvskris Exp $";

#include "sectionextender.h"

namespace MPE 
{


SectionExtender::SectionExtender( const EM::SectionID& si)
    : sectionid( si )
{}


EM::SectionID SectionExtender::sectionID() const { return sectionid; }


void SectionExtender::reset() { addedpos.erase(); }


void SectionExtender::setDirection( const BinIDValue& ) {}


const BinIDValue* SectionExtender::getDirection() const { return 0; }


void SectionExtender::setStartPositions( const TypeSet<EM::SubID> ns )
{ startpos = ns; }


int SectionExtender::nextStep() { return 0; }


const char* SectionExtender::errMsg() const { return errmsg[0] ? errmsg : 0; }


const TypeSet<EM::SubID>& SectionExtender::getAddedPositions() const
{ return addedpos; }

};



