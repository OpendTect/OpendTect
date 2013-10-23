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

    const osg::Image*	createRGBA()		{ return 0; }
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

    ColTab::Sequence	sequence_;
    bool		enabled_;

/*  OSG-TODO: Port to OSG if class is prolongated
    SoTransferFunction*	transferfunc_;
    virtual SoNode*	gtInvntrNode();
*/
};

} //namespace


#endif

