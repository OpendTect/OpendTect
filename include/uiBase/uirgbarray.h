#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "odimage.h"

mFDQtclass(QImage)


mExpClass(uiBase) uiRGBArray : public OD::RGBImage
{ mODTextTranslationClass(uiRGBArray)
public:

			uiRGBArray(bool withalpha);
			uiRGBArray(const OD::RGBImage&);
			uiRGBArray(const char* filename);
    virtual		~uiRGBArray();

    OD::Color		get(int,int) const override;
    bool		set(int,int,const OD::Color&) override;

    unsigned char*		getData() override;
    const unsigned char*	getData() const override;

    char		nrComponents() const override
			{ return withalpha_ ? 4 : 3; }

    int			getSize(bool xdir) const override;
    bool		setSize(int,int) override;
				//!< destroys whatever is there
    int			getWidth() const	{ return getSize(true); }
    int			getHeight() const	{ return getSize(false); }

    bool		reSize(int,int);	//!< inter/extrapolates
    void		clear(const OD::Color&) override;
    void		enableAlpha( bool yn )	{ withalpha_ = yn; }

    bool		save(const char* fnm,const char* fmt=0,
			     int quality=-1) const;

    const mQtclass(QImage&) qImage() const	{ return *qimg_; }
    mQtclass(QImage&)	qImage()		{ return *qimg_; }

protected:

    mQtclass(QImage*)	qimg_;
    bool		withalpha_;

};


mExpClass(uiBase) uiRGBImageLoader : public OD::RGBImageLoader
{ mODTextTranslationClass(uiRGBImageLoader)
public:

    static void		initClass();

private:
			uiRGBImageLoader();
    OD::RGBImage*	loadImage(const char*,uiString&) const override;
};
