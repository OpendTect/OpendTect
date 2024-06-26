#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!

#include "visbasemod.h"

#include "rowcol.h"
#include "vishorizonsection.h"
#include "vistexturechannel2rgba.h"

class DataPointSet;
class BinIDValueSet;
class TaskRunner;

namespace osg { class Node; }

namespace visBase
{

class HorizonTextureHandler : public ReferencedObject
{
public:
				HorizonTextureHandler(HorizonSection*);

    osg::Node*			getOsgNode();
    osgGeo::LayeredTexture*	getOsgTexture();
    const osgGeo::LayeredTexture* getOsgTexture() const;
    int				nrChannels() const;
    void			useChannel(bool);
    void			addChannel();
    void			removeChannel(int);
    void			swapChannels(int,int);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannels2RGBA(TextureChannel2RGBA*);
    void			setColTabSequence(int channel,
						  const ColTab::Sequence& se);
    const ColTab::Sequence*	getColTabSequence(int channel) const;

    void			setColTabMapperSetup(int channel,
					    const ColTab::MapperSetup& mapper,
					    TaskRunner* tr);
    const ColTab::MapperSetup*  getColTabMapperSetup(int ch) const;
    const TypeSet<float>*	getHistogram(int ch) const;
    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels();
    const TextureChannels*	getChannels() const;
    int				nrVersions(int channel) const;
    void			setNrVersions(int channel,int);
    int				activeVersion(int channel) const;
    void			selectActiveVersion(int channel,int);

    void			setTextureData(int channel,int sectionid,
					       const DataPointSet*);
    void			updateTexture(int channel,int sectionid,
					      const DataPointSet*);
    void			updateTileTextureOrigin();

    void			setHorizonSection(const HorizonSection&);


protected:
				~HorizonTextureHandler();

    WeakPtr<HorizonSection>	horsection_;
    ObjectSet<BinIDValueSet>	cache_;

    RefMan<TextureChannels>	channels_;
    RefMan<TextureChannel2RGBA> channel2rgba_;
};

} // namespace visBase
