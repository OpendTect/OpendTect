#ifndef uiprestackviewer2d_h
#define uiprestackviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
 RCS:           $Id: uipsviewer2d.h,v 1.4 2011-05-04 15:20:02 cvsbruno Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "uiobjectitemview.h"
#include "uigroup.h"

/*! brief display multiple ps gathers side by side with dynamic redraw possibilities !*/

class uiFlatViewer;
class BinID;

namespace PreStackView
{
    class Viewer2DGatherPainter;
    class uiViewer2DAxisPainter;

mClass uiGatherDisplay : public uiGroup
{
public:
    				uiGatherDisplay(uiParent*,bool havepan=false);
    				~uiGatherDisplay();

    virtual void                setPosition(const BinID&,
					    const Interval<double>* zrg=0);
    virtual void		setGather(int id);

    void                        displayAnnotation(bool yn);
    bool                        displaysAnnotation() const;

    void                        setFixedOffsetRange(bool yn,
				    const Interval<float>&);
    bool                        getFixedOffsetRange() const;
    const Interval<float>&      getOffsetRange() const;
    const Interval<double>*     getZRange() const	{ return zrg_; }

    uiFlatViewer*               getUiFlatViewer() 	{ return viewer_; }
    BinID			getBinID() const;

    void			setInitialSize(const uiSize&);
    void			setWidth(int);

protected:

    uiFlatViewer*               viewer_;
    Viewer2DGatherPainter* 	gatherpainter_;

    bool                        fixedoffset_;
    Interval<float>             offsetrange_;
    Interval<double>*           zrg_;
    BinID			bid_;
    bool                        displayannotation_;

    void                        updateViewRange(const uiWorldRect&);
};

}; //namespace

#endif
