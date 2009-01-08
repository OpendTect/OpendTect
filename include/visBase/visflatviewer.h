#ifndef visflatviewer_h
#define visflatviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.h,v 1.8 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "visobject.h"

namespace visBase
{

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
    void			allowShading(bool yn);  
    void			replaceChannels(visBase::TextureChannels*);
    				/*!<Replaces internal texture. The new texture 
				  will not be added to the scene. */
    Interval<float>		getDataRange(bool iswva) const;    

protected:
    				~FlatViewer();
    
    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;
    SplitTexture2Rectangle* 	rectangle_;
};

}; // Namespace


#endif
