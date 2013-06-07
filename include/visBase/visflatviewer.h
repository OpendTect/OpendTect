#ifndef visflatviewer_h
#define visflatviewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "flatview.h"
#include "visobject.h"

namespace visBase
{

class IndexedPolyLine;    
class TextureChannels;
class ColTabTextureChannel2RGBA;
class SplitTexture2Rectangle;

/*!Implementation of FlatViewer::Viewer in 3D. */

mClass FlatViewer : public FlatView::Viewer, public VisualObjectImpl
{
public:
    static FlatViewer*		create()
				mCreateDataObj(FlatViewer);

    Notifier<FlatViewer> 	dataChange;    
    void			handleChange(FlatView::Viewer::DataChangeType,
	    				     bool dofill = true);
    void	                setPosition(const Coord3& c00,const Coord3& c01,
	                                  const Coord3& c10,const Coord3& c11);    
    void			turnOnGridLines(bool offsetlines,bool zlines);
    void			allowShading(bool yn);  
    void			replaceChannels(visBase::TextureChannels*);
    				/*!<Replaces internal texture. The new texture 
				  will not be added to the scene. */
    Interval<float>		getDataRange(bool iswva) const;  
    const SamplingData<float>	getDefaultGridSampling(bool x1) const;

    int				nrResolutions() const;
    void			setResolution(int res);
    int				getResolution() const	{ return resolution_; } 
    BufferString		getResolutionName(int) const;

protected:
    				~FlatViewer();
    
    void			updateGridLines(bool x1);
    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;
    SplitTexture2Rectangle* 	rectangle_;

    visBase::IndexedPolyLine*	x1gridlines_;
    visBase::IndexedPolyLine*	x2gridlines_;

    int				resolution_;
    Coord3			c00_, c01_, c10_, c11_;
};

}; // Namespace


#endif
