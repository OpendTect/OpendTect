#ifndef vistexturechannel2voldata_h
#define vistexturechannel2voldata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Karthika
 Date:		Nov 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vistexturechannel2rgba.h"
#include "coltabsequence.h"

class SoVolumeData;
class SoGroup;
class SoTransferFunction;

namespace visBase
{ 

/*! Implementation of TextureChannel2VolData that feeds the 8-bit values from 
 the texture channel(s) into a volume data object. 
*/

mClass TextureChannel2VolData : public TextureChannel2RGBA
{
public:
    static TextureChannel2VolData*	create()
			mCreateDataObj(TextureChannel2VolData);

    bool        	createRGBA(SbImagei32&) const	{ return false; }
    bool		canSetSequence() const	{ return true; }
    void		setSequence(int channel,const ColTab::Sequence&);
    const ColTab::Sequence* getSequence(int channel) const;

    void		setEnabled(int ch, bool yn);
    bool		isEnabled(int ch) const;

    bool		canUseShading() const	{ return false; }
    bool		usesShading() const	{ return false; }
    int			maxNrChannels() const	{ return 1; }
    int			minNrChannels() const	{ return 1; }

    MappedTextureDataSet* createMappedDataSet() const;

protected:
    			~TextureChannel2VolData();

    void		setChannels(TextureChannels*);
    void		notifyChannelChange();
    void		update();
    void		makeColorTables();

    SoTransferFunction*	transferfunc_;
    ColTab::Sequence	sequence_;
    bool		enabled_;

    virtual SoNode*	gtInvntrNode();

};

} //namespace


#endif
