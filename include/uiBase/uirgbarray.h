#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id: uirgbarray.h,v 1.2 2006-09-15 14:36:29 cvshelene Exp $
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
    Color		get(int,int) const;
    void		set(int,int,const Color&);

    const QImage&	Image() const		{ return *qimg_; } ;
    QImage&		Image()			{ return *qimg_; } ;

protected:

    QImage*		qimg_;

};

#endif
