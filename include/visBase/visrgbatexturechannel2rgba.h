#ifndef visrgbatexturechannel2rgba_h
#define visrgbatexturechannel2rgba_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
 RCS:		$Id$
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

    const ColTab::Sequence*	getSequence(int ch) const;
    void			getChannelName(int,BufferString&) const;

    void			swapChannels(int ch0,int ch1);

    void			setEnabled(int ch,bool yn);
    bool			isEnabled(int ch) const;

    void			setTransparency(unsigned char);
    unsigned char		getTransparency() const;

    void			applyUndefPerChannel(bool);
    bool			isUndefPerChannel() const;

    int				maxNrChannels() const	{ return 4; }
    int				minNrChannels() const	{ return 4; }

protected:

    				~RGBATextureChannel2RGBA();

    void			notifyChannelInsert(int ch);

    static ArrPtrMan<ColTab::Sequence>	sequences_;

    osgGeo::RGBALayerProcess*	proc_;
};


}; //namespace


#endif

