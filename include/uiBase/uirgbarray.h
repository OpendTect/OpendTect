#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id: uirgbarray.h,v 1.1 2006-09-08 13:25:28 cvshelene Exp $
________________________________________________________________________

-*/

#include "color.h"

class QImage;


class uiRGBArray
{
public:
                        uiRGBArray();
    virtual		~uiRGBArray();

    void                setSize(int,int);
    Color		get(int,int) const;
    void		set(int,int,const Color&);

    const QImage&	Image() const		{ return *qimg_; } ;
    QImage&		Image()			{ return *qimg_; } ;

protected:

    QImage*		qimg_;

};

#endif
