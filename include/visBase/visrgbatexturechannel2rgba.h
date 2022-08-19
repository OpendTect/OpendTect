#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "vistexturechannel2rgba.h"

namespace osgGeo { class RGBALayerProcess; }


namespace visBase
{

/*!Converts texture channels with RGBA information to ... RGBA.
Does also handle enable/disable of the channels. */


mExpClass(visBase) RGBATextureChannel2RGBA : public TextureChannel2RGBA
{
public:
    static RGBATextureChannel2RGBA*	create()
				mCreateDataObj(RGBATextureChannel2RGBA);

    const ColTab::Sequence*	getSequence(int ch) const override;
    void			getChannelName(int,uiString&) const override;

    void			swapChannels(int ch0,int ch1) override;

    void			setEnabled(int ch,bool yn) override;
    bool			isEnabled(int ch) const override;

    void			setTransparency(unsigned char);
    unsigned char		getTransparency() const;

    void			applyUndefPerChannel(bool);
    bool			isUndefPerChannel() const;

    int				maxNrChannels() const override	{ return 4; }
    int				minNrChannels() const override	{ return 4; }

protected:

				~RGBATextureChannel2RGBA();

    void			notifyChannelInsert(int ch) override;

    BoolTypeSet			enabled_;
    unsigned char		proctransparency_;

    static ArrPtrMan<ColTab::Sequence>	sequences_;

    osgGeo::RGBALayerProcess*	proc_;
};

} // namespace visBase
