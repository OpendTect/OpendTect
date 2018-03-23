#include "uiTutorialText.h"

namespace visSurvey
{
TutTextDisplay::TutTextDisplay():
				  visBase::VisualObjectImpl( true ),
				  text_( visBase::Text2::create() )
{
    text_->ref();
}

TutTextDisplay::TutTextDisplay(const uiString& str,const Coord3& c): 
				  visBase::VisualObjectImpl( true ),
				  text_( visBase::Text2::create() )
{
    text_->ref();
    text_->addText();

    text_->text(0)->setText( str );
    text_->text(0)->setPosition( c );

    FontData fd;
    fd.setWeight( FontData::Bold );
    text_->text( 0 )->setFontData( fd, 500 );

    addChild( text_->osgNode() );
}

TutTextDisplay::~TutTextDisplay()
{
    text_->unRef();
}

}
