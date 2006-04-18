/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon2d.cc,v 1.1 2006-04-18 17:30:13 cvskris Exp $
________________________________________________________________________

-*/

#include "emhorizon2d.h"

#include "errh.h"
#include "horizon2dline.h"

namespace EM
{

Horizon2D::Horizon2D( EMManager& man )
    : EMObject( man )
{}


const char* Horizon2D::getTypeStr() const { return "2D Horizon"; }


int Horizon2D::nrSections() const { return sids_.size(); }


SectionID Horizon2D::sectionID( int idx ) const { return sids_[idx]; }


BufferString Horizon2D::sectionName( const SectionID& sid ) const
{
    const int idx = sids_.indexOf( sid );
    return idx>=0 && sectionnames_[idx] ?
	sectionnames_.get(idx) : BufferString("");
}


bool Horizon2D::canSetSectionName() const	{ return true; }


bool Horizon2D::setSectionName( const SectionID& sid, const char* nm,
				bool addtohistory )
{
    const int idx = sids_.indexOf( sid );
    if ( !nm, !*nm || idx<0 )
	return false;

    sectionnames_.get(idx) = nm;

    if ( addtohistory )
	pErrMsg("Section namechange history not implemented" );

    changed = true;
    return true;
}


int Horizon2D::sectionIndex( const SectionID& sid ) const
{
    return sids_.indexOf( sid );
}


int Horizon2D::nrLines() const
{ return linenames_.size(); }


int Horizon2D::lineID( int idx ) const
{ return idx>=0 && idx<nrLines() ? lineids_[idx] : -1; }


const char* Horizon2D::lineName( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    if ( idx>=0 && idx<nrLines() )
    {
	const BufferString& str = linenames_.get(idx);
	return str.size() ? str.buf() : 0;
    }
    
    return 0;
}


int Horizon2D::addLine( const TypeSet<Coord>& path, int start, int step,
			 const char* nm )
{
    linenames_.add( nm );

    for ( int idx=sections_.size(); idx>=0; idx-- )
    {
	const int lineid = sections_[idx]->addRow( path, start, step );
	if ( idx )
	    continue;

	lineids_ += lineid;
    }

    return lineids_[lineids_.size()-1];
}


void Horizon2D::removeLine( int lid )
{
    const int lidx = lineids_.indexOf( lid );
    if ( lidx<0 || lidx>=linenames_.size() )
	return;

    linenames_.remove( lidx );
    lineids_.remove( lidx );
    for ( int idx=sections_.size(); idx>=0; idx-- )
	sections_[idx]->removeRow( lid );
}


}; //namespace
