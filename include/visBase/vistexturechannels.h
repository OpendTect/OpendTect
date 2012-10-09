#ifndef vistexturechannels_h
#define vistexturechannels_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Jun 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visdata.h"
#include "odmemory.h"

class SoSwitch;
class SoTextureComposer;
class SbImagei32;
class TaskRunner;

template <class T> class Array2D;

namespace ColTab { class Mapper; class MapperSetup; };

namespace visBase
{

class MappedTextureDataSet;
class TextureChannel2RGBA;
class ChannelInfo;

mClass TextureChannels : public DataObject
{
public:
    static TextureChannels*	create()
				mCreateDataObj(TextureChannels);

    virtual bool		turnOn(bool yn);
    virtual bool		isOn() const;

    int				nrChannels() const;
    int				addChannel();
    int				insertChannel(int);
    void			removeChannel(int);
    void			swapChannels(int,int);

    void			setColTabMapperSetup(int channel,
	    					const ColTab::MapperSetup&);
    				//!<Will not trigger a remap, use reMapData
    const ColTab::MapperSetup&	getColTabMapperSetup(int channel,
	    					     int version) const;
    const ColTab::Mapper&	getColTabMapper(int channel,int version) const;
    void			reMapData(int channel,bool dontreclip,
	    				  TaskRunner*);
    const TypeSet<float>*	getHistogram(int channel) const;

    void			setSize(int channel,int sz0,int sz1,int sz2);
    int				getSize(int channel,unsigned char dim) const;

    				//Old, don't use in new code. Will be removed
    void			setSize(int sz0,int sz1,int sz2);
    int				getSize(unsigned char dim) const;

    void			setNrComponents(int channel,int nrcomp);
    int				getNrComponents(int channel) const;

    void			setNrVersions(int channel,int nrvers);
    int				nrVersions(int channel) const;
    int				currentVersion(int channel) const;
    void			setCurrentVersion(int channel,int version);

    bool			setUnMappedVSData(int channel,int version,
				    const ValueSeries<float>*, OD::PtrPolicy,
				    TaskRunner*);
    bool			setUnMappedData(int channel,int version,
	    				        const float*, OD::PtrPolicy,
						TaskRunner*);
    bool			setMappedData(int channel,int version,
	    				      unsigned char*, OD::PtrPolicy);

    bool			setChannels2RGBA(TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;

    const SbImagei32*		getChannels() const;
    void			touchMappedData();

protected:
    friend			class ChannelInfo;
    void			update(int channel,bool tc2rgba);
    void			update(ChannelInfo*,bool tc2rgba);
    				~TextureChannels();

    ObjectSet<ChannelInfo>	channelinfo_;
    MappedTextureDataSet*	tc_;
    SoSwitch*			onoff_;
    TextureChannel2RGBA*	tc2rgba_;

    virtual SoNode*		gtInvntrNode();

};



mClass TextureComposer : public DataObject
{
public:

    static TextureComposer*	create() 
				mCreateDataObj(TextureComposer);
    void			setOrigin(int,int,int);
    void			setSize(int,int,int);
protected:
				~TextureComposer();
    virtual SoNode*		gtInvntrNode();

    SoTextureComposer*		texturecomposer_;
};

}; // Namespace

#endif
