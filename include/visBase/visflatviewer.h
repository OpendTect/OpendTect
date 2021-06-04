#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "flatview.h"
#include "visobject.h"

namespace visBase
{

class PolyLine;
class TextureChannels;
class ColTabTextureChannel2RGBA;
class TextureRectangle;

/*!Implementation of FlatViewer::Viewer in 3D. */

mExpClass(visBase) FlatViewer : public FlatView::Viewer, public VisualObjectImpl
{
public:
    static FlatViewer*	       create()
			       mCreateDataObj(FlatViewer);

    Notifier<FlatViewer>       dataChanged;
    Notifier<FlatViewer>       dispParsChanged;
    void		       handleChange(unsigned int);
    void	               setPosition(const Coord3& c00,
					   const Coord3& c01,
	                                   const Coord3& c10,
					   const Coord3& c11);
    void		       turnOnGridLines(bool offsetlines,bool zlines);
    void		       allowShading(bool yn);
    void		       replaceChannels(TextureChannels*);
    			       /*!<Replaces internal texture. The new texture
				  will not be added to the scene. */
    Interval<float>	       getDataRange(bool iswva) const;
    const SamplingData<float>  getDefaultGridSampling(bool x1) const;

    int			       nrResolutions() const		{ return 3; }
    void		       setResolution(int res);
    int			       getResolution() const	{ return resolution_; }
    BufferString	       getResolutionName(int) const;

    FlatView::AuxData*	       createAuxData(const char* nm) const { return 0;}

    int			       nrAuxData() const { return 0; }
    FlatView::AuxData*	       getAuxData(int idx) { return 0; }
    const FlatView::AuxData*   getAuxData(int idx) const { return 0; }
    void		       addAuxData(FlatView::AuxData* a) {}
    FlatView::AuxData*	       removeAuxData(FlatView::AuxData* a) { return a;}
    FlatView::AuxData*	       removeAuxData(int idx) { return 0; }
    void		       setDisplayTransformation(const mVisTrans*);
    virtual void	       setPixelDensity(float);


protected:
    			       ~FlatViewer();
    
    void		       updateGridLines(bool x1);
    TextureChannels*	       channels_;
    ColTabTextureChannel2RGBA* channel2rgba_;
    RefMan<TextureRectangle>   rectangle_;
    PolyLine*	    	       x1gridlines_;
    PolyLine*		       x2gridlines_;
    Material*		       gridlinematerial_;

    int			       resolution_;
    Coord3		       c00_, c01_, c10_, c11_;
};

}; // Namespace


