#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.h,v 1.1 2000-11-27 10:19:27 bert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "iodraw.h"

class QPixmap;
class QPaintDevice;

class ArrayRGB;

//! off-screen pixel-based paint device
class ioPixmap : public UserIDObject , public ioDrawArea
{
public:
			ioPixmap() : mDrawArea_( 0 ) {}
			ioPixmap(const ArrayRGB&);
    virtual		~ioPixmap();

    bool		convertFromArrayRGB(const ArrayRGB&);

    QPixmap* 		Pixmap()		{ return mDrawArea_; }
    const QPixmap*  	Pixmap() const		{ return mDrawArea_; }
    
protected:
    
    virtual QPaintDevice* mQPaintDevice();         
    QPixmap*		mDrawArea_; 

};


#endif
