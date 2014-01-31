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


mExpClass(uiBase) uiRGBArray : public OD::RGBImage
{
public:

                        uiRGBArray(bool withalpha);
			uiRGBArray(const OD::RGBImage&);
                        uiRGBArray(const char* filename);
    virtual		~uiRGBArray();

    virtual Color	get(int,int) const;
    virtual bool	set(int,int,const Color&);

    virtual char	nrComponents() const	{ return withalpha_ ? 4 : 3; }
    virtual int		getSize(bool xdir) const;
    virtual bool	setSize(int,int);	//!< destroys whatever is there

    bool                reSize(int,int);	//!< inter/extrapolates
    void		clear(const Color&);

    const mQtclass(QImage&) qImage() const	{ return *qimg_; } ;
    mQtclass(QImage&)	qImage()		{ return *qimg_; } ;

protected:

    mQtclass(QImage*)	qimg_;
    bool		withalpha_;

};


#endif
