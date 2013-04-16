#ifndef psviewer2dgatherpainter_h
#define psviewer2dgatherpainter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
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

mExpClass(uiPreStackProcessing) Viewer2DGatherPainter
{
public:
    				Viewer2DGatherPainter(FlatView::Viewer&);
    				~Viewer2DGatherPainter();

    void			setVDGather(DataPack::ID);
    void			setWVAGather(DataPack::ID);
    void			setColorTableClipRate(float);
    float			colorTableClipRate() const;

    BinID			getBinID() const;
    bool			hasData() const
    				{ return inputwvagather_ || inputvdgather_; }

protected:
    void			getGatherRange(const PreStack::Gather*,
					       SamplingData<float>&,
					       int& nrsamp) const;

    FlatView::Viewer&		viewer_;
    const PreStack::Gather*	inputwvagather_;
    const PreStack::Gather*	inputvdgather_;
};


}; //namespace

#endif

