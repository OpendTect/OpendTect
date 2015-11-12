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

#include "slidespec.h"

mExpClass(uiPresentationMaker) PresentationSpec
{
public:
			PresentationSpec();
			~PresentationSpec();

    int			nrSlides() const;
    void		addSlide(SlideSpec&);
    void		insertSlide(int,SlideSpec&);
    void		swapSlides(int idx0,int idx1);
    void		removeSlide(int);

    void		setTitle(const char*);
    void		setOutputFilename(const char*);
    void		setMasterFilename(const char*);

    void		getPythonScript(BufferString&) const;

protected:
    ObjectSet<SlideSpec>	slides_;
    BufferString		title_;

    BufferString		masterfilename_;
    BufferString		outputfilename_;

};

#endif
