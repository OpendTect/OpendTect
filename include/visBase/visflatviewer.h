#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/

#include "visobject.h"
#include "flatview.h"


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

    typedef FlatView::AuxData	AuxData;

    static FlatViewer*		create()
				mCreateDataObj(FlatViewer);

    Notifier<FlatViewer>	dataChanged;
    Notifier<FlatViewer>	dispParsChanged;
    void			setPosition(const Coord3& c00,
					   const Coord3& c01,
						const Coord3& c10,
					   const Coord3& c11);
    void			turnOnGridLines(bool offsetlines,bool zlines);
    void			allowShading(bool yn);
    void			replaceChannels(TextureChannels*);
				/*!<Replaces internal texture. The new texture
				  will not be added to the scene. */
    Interval<float>		getDataRange(bool iswva) const;
    const SamplingData<float>	getDefaultGridSampling(bool x1) const;

    int				nrResolutions() const		{ return 3; }
    void			setResolution(int res);
    int				getResolution() const	{ return resolution_; }
    uiWord			getResolutionName(int) const;

    void			setDisplayTransformation(const mVisTrans*);
    virtual void		setPixelDensity(float);


protected:

				~FlatViewer();

    void			updateGridLines(bool x1);
    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;
    RefMan<TextureRectangle>	rectangle_;
    PolyLine*			x1gridlines_;
    PolyLine*			x2gridlines_;
    Material*			gridlinematerial_;

    int				resolution_;
    Coord3			c00_, c01_, c10_, c11_;

    virtual void	doHandleChange(unsigned int);
    virtual AuxData*	doCreateAuxData(const char*) const { return 0;}
    virtual int		gtNrAuxData() const		{ return 0; }
    virtual AuxData*	gtAuxData(int idx) const	{ return 0; }
    virtual void	doAddAuxData(AuxData*)		{}
    virtual AuxData*	doRemoveAuxData( AuxData* a )	{ return a;}
    virtual AuxData*	doRemoveAuxDataByIdx(int)	{ return 0; }

};

}; // Namespace
