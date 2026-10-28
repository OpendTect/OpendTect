#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgcoltablookup.h"
#include "visvsgcoltabmapping.h"
#include "vsggeolayeredtexture.h"

#include "coltabmapper.h"
#include "odmemory.h"
#include "refcount.h"
#include "typeset.h"
#include "valseries.h"

#include <vsg/core/ref_ptr.h>

class TaskRunner;

namespace dGBVis
{

/*!\brief Counterpart of visBase::TextureChannels, with one channel and version.

  The data is a plane, as in visBase: setSize(0,1,height,width), the values row
  by row. They are mapped to color indices on the CPU and the shader looks the
  colors up. Images larger than LayeredTexture::maxTextureSize() are split in
  tiles that overlap by one row or column, so that interpolation shows no seam.
*/

mExpClass(vsgBase) TextureChannels : public CallBacker
				   , public ReferencedObject
{
public:
    static RefMan<TextureChannels> create();

    int				nrChannels() const	{ return 1; }
    bool			validIdx(int channel) const
				{ return channel==0; }

    void			setSize(int channel,int sz0,int sz1,int sz2);
				//!<sz0 must be 1, sz1 is the height
    int				getSize(int channel,unsigned char dim) const;

    void			setColTabMapperSetup(int channel,
						const ColTab::MapperSetup&);
				//!<Will not trigger a remap, use reMapData
    const ColTab::MapperSetup&	getColTabMapperSetup(int channel,
						     int version) const;
    const ColTab::Mapper&	getColTabMapper(int channel,int version) const;

    bool			setUnMappedData(int channel,int version,
						const float*,OD::PtrPolicy,
						TaskRunner*,bool skipclip=false);
				//!<The values are copied
    bool			setUnMappedVSData(int channel,int version,
					const ValueSeries<float>*,OD::PtrPolicy,
					TaskRunner*,bool skipclip=false);
				//!<The values are copied
    bool			setMappedData(int channel,int version,
					      unsigned char*,OD::PtrPolicy);
				//!<Per cell the color index and a band that is
				//!<255 if defined, as ColTabMapping makes them
    bool			isCurrentDataPremapped(int channel) const
				{ return validIdx(channel) && ispremapped_; }
    void			reMapData(int channel,bool dontreclip,
					  TaskRunner*);
    const TypeSet<float>*	getHistogram(int channel) const;
				//!<Null if there is none, as for mapped data

    void			enableTextureInterpolation(bool);
				//!<On by default, as in visBase
    bool			textureInterpolationEnabled() const
				{ return interpolate_; }

    void			setColTabSequence(const ColTab::Sequence&);
    const ColTab::Sequence&	getColTabSequence() const
				{ return lookup_.sequence(); }
    bool			hasTransparency() const
				{ return lookup_.sequence().hasTransparency(); }
				//!<Of the color table, with the undefined color

    struct Tile
    {
	vsg::ref_ptr<vsgGeo::LayeredTexture>	texture_;
	int					x0_ = 0;
	int					y0_ = 0;
	int					nrx_ = 0;
	int					nry_ = 0;
	int					overx_ = 0;
	int					overy_ = 0;

	bool operator==( const Tile& oth ) const
	{
	    return texture_==oth.texture_ && x0_==oth.x0_ && y0_==oth.y0_ &&
		   nrx_==oth.nrx_ && nry_==oth.nry_ &&
		   overx_==oth.overx_ && overy_==oth.overy_;
	}
    };

    int				nrTiles() const	{ return tiles_.size(); }
    const Tile&			tile(int idx) const
				{ return tiles_[idx]; }
				//!<The texture starts overx_ columns and overy_
				//!<rows before the cells x0_, y0_ of the tile

    Notifier<TextureChannels>	textureChanged;
				//!<New tiles, because the size changed
    Notifier<TextureChannels>	colTabChanged;
				//!<The sequence changed, or the range after a remap

private:
				TextureChannels();
				~TextureChannels();

    void			createTiles();
    void			mapToTexture(bool full,TaskRunner*);

    ColTabLookup		lookup_;
    ColTabMapping		mapping_;
    TypeSet<float>		data_;
    TypeSet<unsigned char>	premapped_;
    bool			ispremapped_	= false;
    bool			interpolate_	= true;
    int				size_[3] = { 0, 0, 0 };
    TypeSet<Tile>		tiles_;
};

} // namespace dGBVis
