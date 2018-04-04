/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vistutorialdisplay.h"

#include "randcolor.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ioman.h"

#include "vismarkerset.h"
#include "vispolyline.h"
#include "vistext.h"

#include "vismaterial.h"
#include "vistransform.h"
#include "visscene.h"

#include "wellmanager.h"
#include "welltransl.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellmarker.h"

visSurvey::TutorialDisplay::TutorialDisplay()
    : visBase::VisualObjectImpl(true)
    , text_(visBase::Text2::create())
    , welllabels_(visBase::Text2::create())
    , transformation_(0)
{
    text_->ref();
    welllabels_->ref();
}


void visSurvey::TutorialDisplay::displayAllWells()
{
    DBDirEntryList wellobjlist( mIOObjContext(Well) );

    // Create and display Well objects, fills wells_.
    for ( int idx=0; idx<wellobjlist.size(); idx++ )
	displayWell( wellobjlist.ioobj(idx) );

    /* since well display track objects (polyline and markers) is created after 
       setDisplayTransformation is called, 
       we will have to call this ourselves  after their creation */

    for ( int idx=0; idx<wells_.size(); idx++ )
	wells_[idx]->setDisplayTransformation( transformation_ );
}


void visSurvey::TutorialDisplay::displayWellLabel( const uiString& texttodisp,
						   const Coord3& pos )
{
    const int index = welllabels_->addText();
    welllabels_->text(index)->setText( texttodisp );
    welllabels_->text(index)->setPosition( pos );

    addChild( welllabels_->osgNode() );
}


const mVisTrans* visSurvey::TutorialDisplay::getDisplayTransformation() const
{ return transformation_; }

void visSurvey::TutorialDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ == nt  )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    text_->setDisplayTransformation( transformation_ );
    welllabels_->setDisplayTransformation( transformation_ );

    for ( int idx=0; idx<wells_.size(); idx++ )
	wells_[idx]->setDisplayTransformation( transformation_ );
}


visSurvey::TutorialDisplay::~TutorialDisplay()
{
    removeChild( text_->osgNode() );
    text_->unRef( );
    removeChild( welllabels_->osgNode() );
    welllabels_->unRef();

    removeFromNodeAndUnRef();
}


void visSurvey::TutorialDisplay::displayWell( const IOObj& wellobj )
{
    uiRetVal uirv;
    
    ConstRefMan<Well::Data> data =
		Well::MGR().fetch( wellobj.key(), Well::LoadReqs::All(), uirv );

    Well::Track timetrack;
    if ( scene_ && false==scene_->zDomainInfo().def_.isDepth() )
	timetrack.toTime( *data );  // convert to time (if required)
    else
	timetrack = data->track();
   
    if ( data->track().size()<1 )
	return;

    /* this memory will be recovered in RemovefromNodeAndUnRef*/
    WellDisplay* welldisplaydata = new WellDisplay();
    
    visBase::PolyLine* polylinewell = visBase::PolyLine::create();
    polylinewell->ref();  /* after creation immediately call ref() */

    Well::TrackIter iter( timetrack );  /* using iterator */
    while ( iter.next() )
    {
	Coord3 pt = iter.pos();
	if ( !mIsUdf(pt.z_) )
	    polylinewell->addPoint( Coord3(pt) );
    }

    addChild( polylinewell->osgNode() );
    welldisplaydata->welltrack_ = polylinewell;

    // now to add markers
    visBase::MarkerSet* markerset = visBase::MarkerSet::create();
    markerset->ref();

    const Well::MarkerSet& markers = data->markers();
    for ( int idx = 0; idx<markers.size(); idx++ )
    {
	const Color markercol = getRandomColor();
	const Well::Marker marker = markers.getByIdx( idx );
	const Coord3 markerpos = timetrack.getPos( marker.dah() );
	const int index = markerset->addPos( markerpos );
	markerset->getMaterial()->setColor( markercol, index );

	displayWellLabel( toUiString(marker.name()), markerpos );
    }

    this->addChild( markerset->osgNode() );
    welldisplaydata->wellmarkers_ = markerset;

    wells_.add( welldisplaydata );
}


void visSurvey::TutorialDisplay::displayText( const uiString& texttodisp,
					      const Coord3& pos )
{
    text_->addText();
    text_->text(0)->setText( texttodisp );
    text_->text(0)->setPosition( pos );

    FontData fd;
    fd.setWeight( FontData::Bold );
    text_->text(0)->setFontData( fd, 500 );

    addChild( text_->osgNode() );
}


void visSurvey::TutorialDisplay::removeFromNodeAndUnRef()
{
    /* for clean up */
    for ( int idx=0; idx<wells_.size(); idx++ )
	wells_[idx]->removeFromNodeandUnref( this );

    deepErase( wells_ );
}

void visSurvey::TutorialDisplay::WellDisplay::removeFromNodeandUnref(
					TutorialDisplay* tutdisplay ) const
{
    tutdisplay->removeChild( welltrack_->osgNode() );
    tutdisplay->removeChild( wellmarkers_->osgNode() );

    welltrack_->unRef();
    wellmarkers_->unRef();
}

void visSurvey::TutorialDisplay::WellDisplay::setDisplayTransformation(
    					const mVisTrans* transformation )
{
    welltrack_->setDisplayTransformation( transformation );
    wellmarkers_->setDisplayTransformation( transformation );
}
