/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vistutorialdisplay.h"


visSurvey::TutorialDisplay::TutorialDisplay():
				  visBase::VisualObjectImpl( true ),
				  text_( visBase::Text2::create() )
{
    text_->ref();
}


visSurvey::TutorialDisplay::TutorialDisplay( const uiString& texttodisp,
					   const Coord3& pos )
    : visBase::VisualObjectImpl(true)
    , text_(visBase::Text2::create())
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


visSurvey::TutorialDisplay::~TutorialDisplay()
{
    text_->unRef();
}
