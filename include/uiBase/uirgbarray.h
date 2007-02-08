#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id: uirgbarray.h,v 1.4 2007-02-08 16:53:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "color.h"

class QImage;


class uiRGBArray
{
public:
                        uiRGBArray();
			uiRGBArray(const uiRGBArray&);
    virtual		~uiRGBArray();

    void                setSize(int,int);
    int			getSize(bool xdir) const;
    Color		get(int,int) const;
    void		set(int,int,const Color&);
    void		clear(const Color&);

    const QImage&	qImage() const		{ return *qimg_; } ;
    QImage&		qImage()		{ return *qimg_; } ;

protected:

    QImage*		qimg_;

};

#endif
