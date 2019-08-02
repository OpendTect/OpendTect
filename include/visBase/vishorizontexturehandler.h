#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
________________________________________________________________________
-*/
// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!


#include "visbasecommon.h"
#include "rowcol.h"
#include "refcount.h"

class DataPointSet;
class BinnedValueSet;
class TaskRunnerProvider;
namespace osg {  class Node; }
namespace osgGeo { class LayeredTexture; }

namespace visBase
{
class TextureChannel2RGBA;
class TextureChannels;
class HorizonSection;

class HorizonTextureHandler : public RefCount::Referenced
{
public:
				HorizonTextureHandler(const HorizonSection*);
    osg::Node*		        getOsgNode();
    osgGeo::LayeredTexture*	getOsgTexture();
    int			        nrChannels() const;
    void		        useChannel(bool);
    void		        addChannel();
    void		        removeChannel(int);
    void		        swapChannels(int,int);
    const BinnedValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannels2RGBA(TextureChannel2RGBA*);
    void			setColTabSequence(int channel,
						  const ColTab::Sequence& se);
    const ColTab::Sequence&	getColTabSequence(int channel) const;

    void			setColTabMapper(int channel,
					    const ColTab::Mapper& mapper,
					    const TaskRunnerProvider&);
    const ColTab::Mapper&	getColTabMapper(int ch) const;
    void			setTransparency(int ch, unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels() const { return channels_; }
    int                         nrVersions(int channel) const;
    void                        setNrVersions(int channel,int);
    int                         activeVersion(int channel) const;
    void                        selectActiveVersion(int channel,int);

    void			setTextureData(int channel, int sectionid,
					       const DataPointSet* dtpntset);
    void			updateTexture(int channel,int sectionid,
					      const DataPointSet*);
    void			updateTileTextureOrigin();

    void			setHorizonSection(const HorizonSection&);


protected:
                                ~HorizonTextureHandler();

    const HorizonSection*       horsection_;
    ObjectSet<BinnedValueSet>	cache_;

    TextureChannels*		channels_;
    TextureChannel2RGBA*	channel2rgba_;

};

} // namesapce visBase
