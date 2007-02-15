/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Feb 2007
 RCS:           $Id: visanchor.cc,v 1.1 2007-02-15 23:38:28 cvskris Exp $
________________________________________________________________________

-*/

#include "visanchor.h"

#include <Inventor/nodes/SoWWWAnchor.h>

mCreateFactoryEntry( visBase::Anchor );

namespace visBase
{

Anchor::Anchor()
    : click( this )
    , ishighlighted_( false )
{}



SoGroup* Anchor::createGroup()
{
    SoWWWAnchor* anchor = new SoWWWAnchor;
    anchor->ref();
    anchor->setFetchURLCallBack( clickCB, this );
    anchor->setHighlightURLCallBack( highlightCB, this );
    anchor->mode = SoLocateHighlight::OFF;

    anchor->unrefNoDelete();
    return anchor;
}


void Anchor::enable( bool yn )
{
    getAnchor()->mode = yn ? SoLocateHighlight::AUTO : SoLocateHighlight::OFF;
}


SoWWWAnchor* Anchor::getAnchor()
{ return (SoWWWAnchor*) group_; }




void Anchor::clickCB( const SbString& str, void* obj, SoWWWAnchor* )
{
    Anchor* myself = (Anchor*)obj;

    if ( !myself->ishighlighted_ ) return;

    myself->click.trigger();
}


void Anchor::highlightCB( const SbString& str, void* obj, SoWWWAnchor* )
{
    Anchor* myself = (Anchor*)obj;
    myself->ishighlighted_ = !myself->ishighlighted_;
}


}; // namespace visBase
