#ifndef vistexturechannel2voldata_h
#define vistexturechannel2voldata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Karthika
 Date:		Nov 2009
 RCS:		$Id: vistexturechannel2voldata.h,v 1.5 2009-12-17 14:28:36 cvskarthika Exp $
________________________________________________________________________


-*/

#include "vistexturechannel2rgba.h"
#include "coltabsequence.h"

class SoVolumeData;
class SoGroup;
class SoTransferFunction;

namespace visBase
{ 

/*!A destination where the texturechannels can put the mapped data. The class
   instanciation is provided by the TextureChannel2VolData. */

mClass VolumeDataSet : public MappedTextureDataSet
{
public:
				VolumeDataSet();
	Interval<float>		getVolumeSize(int dim) const;
	void 			setVolumeSize(const Interval<float>& x,
						const Interval<float>& y,
						const Interval<float>& z);

protected:
					~VolumeDataSet();

	SoVolumeData*		voldata_;
	unsigned char* 	datacache_;
	unsigned char	dummytexture_;
    
};


/*! Implementation of TextureChannel2VolData that feeds the 8-bit values from 
 the texture channel(s) into a volume data object. 
*/

mClass TextureChannel2VolData : public TextureChannel2RGBA
{
public:
    static TextureChannel2VolData*	create()
			mCreateDataObj(TextureChannel2VolData);

    bool        	createRGBA(SbImage&) const	{ return false; }
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
    SoNode*		getInventorNode();

    void		setChannels(TextureChannels*);
    void		notifyChannelChange();
    void		update();
    void		makeColorTables();

    SoTransferFunction*	transferfunc_;
    ColTab::Sequence	sequence_;
    bool		enabled_;

};

}; //namespace


#endif
