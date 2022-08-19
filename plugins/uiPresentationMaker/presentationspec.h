#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipresentationmakermod.h"

#include "uigeom.h"
#include "bufstring.h"

mExpClass(uiPresentationMaker) SlideLayout
{
public:
			SlideLayout();

    void		saveToSettings();
    void		readFromSettings();
    void		forBlankPresentation();

    float		availableWidth() const;
    float		availableHeigth() const;

    int			format_;
    float		width_;
    float		height_;
    float		left_;
    float		right_;
    float		top_;
    float		bottom_;
};


mExpClass(uiPresentationMaker) SlideContent
{
public:
			SlideContent(const char* title,const char* imgfnm);
			~SlideContent();

    void		setTitle(const char*);
    bool		setImageSizePos(const SlideLayout&);

    void		addBlankSlide(BufferString&);
    void		addAsFirstSlide(BufferString&);
    void		addWithFirstSlideLayout(BufferString&);

protected:
    BufferString	title_;
    BufferString	imagefnm_;
    Geom::Size2D<float>	imagesz_;
    Geom::Point2D<float> imagepos_;

    void		addTitle(BufferString&);
    void		addImage(BufferString&);
};


mExpClass(uiPresentationMaker) PresentationSpec
{
public:
			PresentationSpec();
			~PresentationSpec();

    SlideLayout&	getSlideLayout()	{ return slidelayout_; }

    void		setEmpty();
    int			nrSlides() const;
    void		addSlide(SlideContent&);
    void		insertSlide(int,SlideContent&);
    void		swapSlides(int idx0,int idx1);
    void		removeSlide(int);

    void		setTitle(const char*);
    void		setSlideTitle(int,const char*);

    void		setOutputFilename(const char*);
    void		setTemplateFilename(const char*);
    void		setLogFilename(const char*);

    void		getPythonScript(BufferString&);

    static BufferString	getTemplate();
    static void		setTemplate(const char*);
    static BufferString	getPyScriptDir();
    static BufferString	getPyExec();
    static void		setPyExec(const char*);

protected:
    ObjectSet<SlideContent>	slides_;
    BufferString		title_;
    bool			addtitleslide_;

    BufferString		templatefilename_;
    BufferString		outputfilename_;
    BufferString		logfilename_;

    SlideLayout			slidelayout_;
};
