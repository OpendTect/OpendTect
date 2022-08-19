#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gpucalcmod.h"
#include "sets.h"

class BufferStringSet;

namespace GPU
{

class DeviceData;
class ProgramData;
class Context;

mExpClass(GPUCalc) Device
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


mExpClass(GPUCalc) Program
{
public:
    			Program(Device&);
			~Program();

    bool		setSource(const BufferStringSet& s,
	    			  const char* mainfunction);

protected:

    ProgramData&	data_;
};

mExpClass(GPUCalc) ProgramObject
{
public:
    			ProgramObject(Program&);
};

mExpClass(GPUCalc) GPUManager
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
mGlobal(GPUCalc) GPUManager& manager();


} // namespace
