#ifndef ui3dindirectviewer_h
#define ui3dindirectviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id: ui3dindirectviewer.h,v 1.1 2011-12-21 12:03:35 cvskris Exp $
________________________________________________________________________

-*/

#include "ui3dviewerbody.h"


class OsgIndirectGraphicsWin;

//Class used by ui3DViewer to render things indirectly

class ui3DIndirectViewBody : public ui3DViewerBody
{
public:
				ui3DIndirectViewBody(ui3DViewer&,uiParent*);
				~ui3DIndirectViewBody();

    const QWidget*              qwidget_() const;

protected:
    void			updateActModeCursor();
    osgGA::GUIActionAdapter&    getActionAdapter();
    osg::GraphicsContext*       getGraphicsContext();

						
    OsgIndirectGraphicsWin*	graphicswin_;    
};

#endif
