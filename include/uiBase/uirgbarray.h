#ifndef uirgbarray_h
#define uirgbarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B. Bril & H. Huck
 Date:          08/09/06
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "odimage.h"

mFDQtclass(QImage)


mClass(uiBase) uiRGBArray : public OD::RGBImage
{
public:
                        uiRGBArray(bool withalpha);
			uiRGBArray(const OD::RGBImage&);
    virtual		~uiRGBArray();

    char		nrComponents() const { return withalpha_ ? 4 : 3; }
    bool                setSize(int,int);
    int			getSize(bool xdir) const;
    Color		get(int,int) const;
    bool		set(int,int,const Color&);
    void		clear(const Color&);

    const mQtclass(QImage&)	qImage() const	{ return *qimg_; } ;
    mQtclass(QImage&)		qImage()	{ return *qimg_; } ;

protected:

    mQtclass(QImage*)	qimg_;
    bool		withalpha_;

};

#endif

