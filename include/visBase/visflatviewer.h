#ifndef visflatviewer_h
#define visflatviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.h,v 1.5 2008-11-26 06:59:52 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "visobject.h"

namespace visBase
{

class MultiTexture2;
class SplitTexture2Rectangle;

/*!Implementation of FlatViewer::Viewer in 3D. */

class FlatViewer : public FlatView::Viewer, public VisualObjectImpl
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
    void			replaceTexture(visBase::MultiTexture2*);
    				/*!<Replaces internal texture. The new texture 
				  will not be added to the scene. */	
protected:
    				~FlatViewer();
    
    MultiTexture2*		texture_;
    SplitTexture2Rectangle* 	rectangle_;
};

}; // Namespace


#endif
