#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2007
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "color.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "ranges.h"
#include "trckeyzsampling.h"
#include "visdata.h"
#include "visosg.h"

class TaskRunner;
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
{
public:
    static VolumeRenderScalarField*	create()
					mCreateDataObj(VolumeRenderScalarField);

    void			setScalarField(int attr,const Array3D<float>*,
					       bool mine,const TrcKeyZSampling&,
					       TaskRunner*);

    void			setChannels2RGBA(visBase::TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;

    void			setColTabMapperSetup(int attr,
						     const ColTab::MapperSetup&,
						     TaskRunner* tr );
    const ColTab::Mapper&	getColTabMapper(int attr);

    const TypeSet<float>&	getHistogram(int attr) const;

    const char*			writeVolumeFile(int attr,od_ostream&) const;
				//!<\returns 0 on success, otherwise errmsg

    static bool			isShadingSupported();
    void			allowShading(bool yn);
    bool			usesShading() const;

    bool			turnOn(bool) override;
    bool			isOn() const override;

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

    TrcKeyZSampling		getMultiAttribTrcKeyZSampling() const;

    void			setRightHandSystem(bool) override;
    bool			isRightHandSystem() const override;

protected:
				~VolumeRenderScalarField();

    void			makeIndices(int attr,bool doset,TaskRunner*);
				// doset argument obsolete now
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

	ColTab::Mapper			mapper_;
	unsigned char*			indexcache_;
	int				indexcachestep_;
	bool				ownsindexcache_;
	const ValueSeries<float>*	datacache_;
	bool				ownsdatacache_;
	TypeSet<float>			histogram_;

	void				clearDataCache();
	void				clearIndexCache();
	void				clearResizeCache();

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

    void			updateResizeCache(int attr,TaskRunner*);

public:

    mDeprecated("Use other setScalarField")
    void			setScalarField(int attr,const Array3D<float>*,
					   bool mine,TaskRunner*);

};

} // namespace visBase
