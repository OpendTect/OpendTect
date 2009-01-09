#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id: uirgbarray.h,v 1.6 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "color.h"

class QImage;


mClass uiRGBArray
{
public:
                        uiRGBArray(bool withalpha);
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
    bool		withalpha_;

};

#endif
