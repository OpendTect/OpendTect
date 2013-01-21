#ifndef psviewer2dgatherpainter_h
#define psviewer2dgatherpainter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: psviewer2dgatherpainter.h 27530 2012-11-19 09:49:13Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "datapack.h"

namespace FlatView { class Viewer; };
template <class T> class SamplingData;

namespace PreStack { class Gather; }

//!Gather display

namespace PreStackView
{

mClass(uiPreStackProcessing) Viewer2DGatherPainter
{
public:
    				Viewer2DGatherPainter(FlatView::Viewer&);
    				~Viewer2DGatherPainter();

    void			setGather(DataPack::ID);
    void			setColorTableClipRate(float);
    float			colorTableClipRate() const;

    BinID			getBinID() const;
    bool			hasData() const { return inputgather_; }

protected:
    void			getGatherRange(const PreStack::Gather*,
					       SamplingData<float>&,
					       int& nrsamp) const;

    FlatView::Viewer&		viewer_;
    const PreStack::Gather*	inputgather_;
};


}; //namespace

#endif

