/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.1 2000-11-27 10:20:34 bert Exp $
________________________________________________________________________

-*/

#include <pixmap.h>
#include <errh.h>

#include <color.h>
#include <arraynd.h>
#include <arrayndimpl.h>
#include <arrayrgb.h>

#include <qpixmap.h>

ioPixmap::ioPixmap( const ArrayRGB& anImage )
{
    mDrawArea_ = new QPixmap;
    if( !convertFromArrayRGB( anImage ) )
    {
        ErrMsg( "Could not create Pixmap" );
        pErrMsg( "Qt's QPixmap::convertFromImage returned with error" );
    }
}


ioPixmap::~ioPixmap()
{
    if(mDrawArea_) delete mDrawArea_;
}

bool ioPixmap::convertFromArrayRGB( const ArrayRGB & theImage )
{
    if( mDrawTool ) { delete mDrawTool; mDrawTool=0; };

    if( !mDrawArea_ )  mDrawArea_ = new QPixmap;
    return mDrawArea_->convertFromImage( theImage.Image() );
    // returns TRUE if successful
}    

QPaintDevice*  ioPixmap::mQPaintDevice()
{ 
    return mDrawArea_; 
}

//  qwidget.html:
//  During paintEvent(), any QPainter you create on the widget will be 
//  clipped to at most the area covered by the update region. 

