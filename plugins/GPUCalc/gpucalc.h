#ifndef gpucalc_h
#define gpucalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Jan 2011
 RCS:           $Id: gpucalc.h,v 1.2 2011/02/07 12:54:26 cvskris Exp $
________________________________________________________________________

-*/


#include "sets.h"

class BufferStringSet;

namespace GPU
{

class DeviceData;
class ProgramData;
class Context;

mClass Device
{
public:
    bool		isGPU() const;
    const char*		platformName();
    const char*		name() const;


protected:
    			Device();
    			~Device();
    void*		getContext();
    void*		getDevice();

    friend class	GPUManager;
    friend class	Program;

    DeviceData&		data_;
};


mClass Program
{
public:
    			Program(Device&);
			~Program();

    bool		setSource(const BufferStringSet& s,
	    			  const char* mainfunction);

protected:

    ProgramData&	data_;
};

mClass ProgramObject
{
public:
    			ProgramObject(Program&);
};

mClass GPUManager
{
public:
    			GPUManager();
			~GPUManager();

    int			nrDevices() const	{ return devices_.size(); }
    Device*		getDevice(int idx)	{ return devices_[idx]; }

protected:
    ObjectSet<Device>		devices_;
    ObjectSet<Context>		contexts_;
};

//Access. 
mGlobal GPUManager& manager();


}; // namespace


#endif
