/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vistutorialdisplay.h"

#include "randcolor.h"

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

#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"
#include "uiodscenemgr.h"
#include "dbman.h"

void visSurvey::TutorialWellDisplay::displayWellLabel( 
						   visBase::Text2* welllabels
						   ,const uiString& texttodisp
						   ,const Coord3& pos )
{
    const int index = welllabels->addText();
    welllabels->text( index )->setText( texttodisp );
    welllabels->text( index )->setPosition( pos );
}


void visSurvey::TutorialWellDisplay::
		     setDisplayTransformation(const mVisTrans* transformation )
{
    if( welltrack_ )   welltrack_->setDisplayTransformation( transformation );
    if( wellmarkers_ ) wellmarkers_->setDisplayTransformation( transformation);
    if( welllabels_ )  welllabels_->setDisplayTransformation( transformation );
}


visSurvey::TutorialWellDisplay::~TutorialWellDisplay()
{
    if ( welltrack_ )   
    {
	removeChild( welltrack_->osgNode() );
	welltrack_->unRef();
	welltrack_ = NULL;
    }

    if( wellmarkers_ ) 
    {
	removeChild( wellmarkers_->osgNode() );
	wellmarkers_->unRef();
	wellmarkers_ = NULL;
    }

    if( welllabels_ )  
    {
	removeChild( welllabels_->osgNode() );
	welllabels_->unRef();
	welllabels_ = NULL;
    }

}


visSurvey::TutorialWellDisplay::TutorialWellDisplay()
					: wellmarkers_(0)
					, welllabels_(0),welltrack_(0)
					, visBase::VisualObjectImpl(true)
					    
{
    wellmarkers_ = visBase::MarkerSet::create();wellmarkers_->ref();
    welllabels_  = visBase::Text2::create();welllabels_->ref();
    welltrack_   = visBase::PolyLine::create();welltrack_->ref();

    addChild(wellmarkers_->osgNode());
    addChild(welllabels_->osgNode());
    addChild(welltrack_->osgNode());

    uiODSceneMgr& scenemgr = ODMainWin()->applMgr().sceneMgr();
    ODMainWin()->applMgr().visServer()->addObject(this,
					     scenemgr.getActiveSceneID(),true);
}


visSurvey::TutorialWellDisplay::TutorialWellDisplay
				     (const DBKey key):TutorialWellDisplay()
{
    key_ = key;
    IOObj* ioobj = DBM().get( key_ );
    if( ioobj ) setName( ioobj->getName() );
}


void visSurvey::TutorialWellDisplay::loadAndDisplayWell()
{
    uiRetVal uirv;

    ConstRefMan<Well::Data> data =
	Well::MGR().fetch( key_, Well::LoadReqs::All(), uirv);

    Well::Track timetrack;
    
    if ( scene_ && false==scene_->zDomainInfo().def_.isDepth() )
	timetrack.toTime( *data );  // convert to time (if required)
    else
	timetrack = data->track();
   
    if ( data->track().size()<1 )
	return;

    Well::TrackIter iter( timetrack );  /* using iterator */
    while ( iter.next() )
    {
	Coord3 pt = iter.pos();
	if ( !mIsUdf(pt.z_) )
	    welltrack_->addPoint( Coord3(pt) );
    }

    // now to add markers
    const Well::MarkerSet& markers = data->markers();
    for ( int idx = 0; idx<markers.size(); idx++ )
    {
	const Color markercol = getRandomColor();
	const Well::Marker marker = markers.getByIdx( idx );
	const Coord3 markerpos = timetrack.getPos( marker.dah() );
	const int index = wellmarkers_->addPos( markerpos );
	wellmarkers_->getMaterial()->setColor( markercol, index );

	OD::MarkerStyle3D markerstyle;

	markerstyle.size_ = 3; /*cylindrical marker style*/
	markerstyle.type_ = OD::MarkerStyle3D::Cylinder;
	wellmarkers_->setMarkerStyle( markerstyle );

	displayWellLabel(welllabels_, toUiString(marker.name()), markerpos );
    } 
}
