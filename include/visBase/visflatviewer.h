#ifndef visflatviewer_h
#define visflatviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.h,v 1.1 2007-09-10 07:54:57 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "visobject.h"

template <class T> class Array2D;

namespace visBase
{
class MultiTexture2;
class FaceSet;
class VisColorTab;

/*!Implementation of FlatViewer::Viewer in 3D. */

class FlatViewer : public FlatView::Viewer, public VisualObjectImpl
{
public:
    static FlatViewer*		create()
				mCreateDataObj(FlatViewer);
    				~FlatViewer();

    Notifier<FlatViewer>	dataChange;    

    void			handleChange(FlatView::Viewer::DataChangeType);
    void			updateTextureCoords(const Array2D<float>*);
    void			setPosition(const Coord3& c00,const Coord3& c01,					   const Coord3& c10,const Coord3& c11);
  
    visBase::Transformation* 	getDisplayTransformation();
    void			setDisplayTransformation(mVisTrans*);

    void			allowShading(bool yn);  
   				
protected:
    FaceSet*			faceset_;
    MultiTexture2*		texture_;

};

}; // Namespace


#endif
