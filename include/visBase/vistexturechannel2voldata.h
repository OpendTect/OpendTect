#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "vistexturechannel2rgba.h"
#include "coltabsequence.h"


namespace visBase
{

/*! Implementation of TextureChannel2VolData that feeds the 8-bit values from
 the texture channel(s) into a volume data object.
*/

mExpClass(visBase) TextureChannel2VolData : public TextureChannel2RGBA
{
public:
    static TextureChannel2VolData*	create()
			mCreateDataObj(TextureChannel2VolData);

    const osg::Image*	createRGBA() const override	{ return nullptr; }
    bool		canSetSequence() const override	{ return true; }
    void		setSequence(int channel,
				    const ColTab::Sequence&) override;
    const ColTab::Sequence* getSequence(int channel) const override;

    void		setEnabled(int ch, bool yn) override;
    bool		isEnabled(int ch) const override;

    bool		canUseShading() const override	{ return false; }
    bool		usesShading() const override	{ return false; }
    int			maxNrChannels() const override	{ return 1; }
    int			minNrChannels() const override	{ return 1; }

    MappedTextureDataSet* createMappedDataSet() const override;

protected:
			~TextureChannel2VolData();

    void		setChannels(TextureChannels*) override;
    void		notifyChannelChange() override;
    void		update();
    void		makeColorTables();

    ColTab::Sequence	sequence_;
    bool		enabled_;

/*  OSG-TODO: Port to OSG if class is prolongated
    SoTransferFunction*	transferfunc_;
    virtual SoNode*	gtInvntrNode();
*/
};

} // namespace visBase
