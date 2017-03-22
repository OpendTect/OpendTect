#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2007
________________________________________________________________________

-*/

#include "visdata.h"
#include "visosg.h"
#include "color.h"
#include "ranges.h"
#include "trckeyzsampling.h"

class od_ostream;
template <class T> class Array3D;
template <class T> class ValueSeries;

namespace osgVolume { class Volume; class VolumeTile; class ImageLayer;
		      class TransparencyProperty; }
namespace osg { class Switch; class Image; class TransferFunction1D; }
namespace osgGeo { class RayTracedTechnique; }



namespace visBase
{

class Material;
class TextureChannel2RGBA;


mExpClass(visBase) VolumeRenderScalarField : public DataObject
{ mODTextTranslationClass(VolumeRenderScalarField)
public:

    static VolumeRenderScalarField*	create()
		mCreateDataObj(VolumeRenderScalarField);

    void			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;

    void			setScalarField(int attr,const Array3D<float>*,
					    bool mine,const TrcKeyZSampling&,
					    TaskRunner*);

    TrcKeyZSampling		getMultiAttribTrcKeyZSampling() const;

    void			setColTabMapper(int attr,const ColTab::Mapper&,
					        TaskRunner*);
    const ColTab::Mapper&	getColTabMapper(int attr);

    const uiString		writeVolumeFile(int attr,od_ostream&) const;
				//!<\returns 0 on success, otherwise errmsg

    static bool			isShadingSupported();
    void			allowShading(bool yn);
    bool			usesShading() const;

    bool			turnOn(bool);
    bool			isOn() const;

    void			setTexVolumeTransform(const Coord3& trans,
				    const Coord3& rotvec,double rotangle,
				    const Coord3& scale);
    void			setROIVolumeTransform(const Coord3& trans,
				    const Coord3& rotvec,double rotangle,
				    const Coord3& scale);
				/*!< Use these instead of parent transformation
				    node, because of normal rescaling issue at
				    fixed function technique! */

    bool			textureInterpolationEnabled() const;
    void			enableTextureInterpolation(bool);

    void			setMaterial(Material*);

				/*!< Auxiliary functions to be called to force
				processing of TextureChannel2RGBA changes */
    void			makeColorTables(int attr);
    void			enableAttrib(int attr,bool yn);
    void			swapAttribs(int attr0,int attr1);
    void			setAttribTransparency(int attr,unsigned char);

    void			setRightHandSystem(bool);
    bool			isRightHandSystem() const;

protected:
				~VolumeRenderScalarField();

    void			updateResizeCache(int attr,TaskRunner*);
    void			makeIndices(int attr,TaskRunner*);
    void			clipData(int attr,TaskRunner*);

    void			updateFragShaderType();
    void			updateVolumeSlicing();
    void			updateTransparencyRescaling();

    void			setDefaultRGBAValue(int channel);

    struct AttribData
    {
					AttribData();
					~AttribData();

	bool				isInVolumeCache() const;

	void				clearDataCache();
	void				clearResizeCache();
	void				clearIndexCache();

	ConstRefMan<ColTab::Mapper>	mapper_;
	unsigned char*			indexcache_;
	int				indexcachestep_;
	bool				ownsindexcache_;
	const ValueSeries<float>*	datacache_;
	bool				ownsdatacache_;
	TrcKeyZSampling			datatkzs_;
	const ValueSeries<float>*	resizecache_;
	bool				ownsresizecache_;
    };

    ObjectSet<AttribData>		attribs_;

    TextureChannel2RGBA*		channels2rgba_;
    bool				isrgba_;

    Material*				material_;
    bool				useshading_;

    bool				isrighthandsystem_;

    osgVolume::VolumeTile*		osgvoltile_;
    osg::Switch*			osgvolroot_;
    osgVolume::Volume*			osgvolume_;
    osgVolume::ImageLayer*		osgimagelayer_;
    osg::Image*				osgvoldata_;
    osg::TransferFunction1D*		osgtransfunc_;
    osgVolume::TransparencyProperty*	osgtransprop_;
    osgGeo::RayTracedTechnique*		raytt_;
};

}
