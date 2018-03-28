/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vistutorialdisplay.h"
#include "vistransform.h"


visSurvey::TutorialDisplay::TutorialDisplay()
    : visBase::VisualObjectImpl(true)
    , text_(visBase::Text2::create())
    , transformation_(0)
{
    text_->ref();
}


visSurvey::TutorialDisplay::TutorialDisplay( const uiString& texttodisp,
					   const Coord3& pos )
    : visBase::VisualObjectImpl(true)
    , text_(visBase::Text2::create())
    , transformation_(0)
{
    text_->ref();
    text_->addText();

    text_->text(0)->setText( texttodisp );
    text_->text(0)->setPosition( pos );

    FontData fd;
    fd.setWeight( FontData::Bold );
    text_->text( 0 )->setFontData( fd, 500 );

    addChild( text_->osgNode() );
}


const mVisTrans* visSurvey::TutorialDisplay::getDisplayTransformation() const
{ return text_->getDisplayTransformation(); }

void visSurvey::TutorialDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_==nt  )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    text_->setDisplayTransformation( transformation_ );
}


visSurvey::TutorialDisplay::~TutorialDisplay()
{
    text_->unRef();
}
