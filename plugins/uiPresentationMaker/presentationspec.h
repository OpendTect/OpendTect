#ifndef presentationspec_h
#define presentationspec_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2015
 RCS:		$Id: $
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

    float		availableWidth() const;
    float		availableHeigth() const;

    int			layoutindex_;
    int			masterindex_;

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
    void		getPythonScript(const SlideLayout&,BufferString&) const;

protected:
    BufferString	title_;
    BufferString	imagefnm_;
    Geom::Size2D<float>	imagesz_;
    Geom::Point2D<float> imagepos_;

    bool		setImageSizePos(const SlideLayout&);
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
    void		setOutputFilename(const char*);
    void		setMasterFilename(const char*);

    void		getPythonScript(BufferString&) const;

    int				titlemasterindex_;
    int				titlelayoutindex_;

protected:
    ObjectSet<SlideContent>	slides_;
    BufferString		title_;

    BufferString		masterfilename_;
    BufferString		outputfilename_;

    SlideLayout			slidelayout_;
};

#endif
