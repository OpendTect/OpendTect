#ifndef gpucalc_h
#define gpucalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Jan 2011
 RCS:           $Id: gpucalc.h,v 1.1 2011-02-04 20:23:42 cvskris Exp $
________________________________________________________________________

-*/

class uiMainWin;


namespace GPU
{

class PlatformData;


mClass Platform
{
public:
    bool		isGPU() const;
    const char*		name() const;

    void*		getContext(int platform);
    			//internal
protected:
    PlatformData&	data_;

};


class ProgramData;
mClass Program
{
public:
    			Program(Platform&);
			~Program();
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

    int			nrPlatforms() const;
    Platform*		getPlatform(int);
    const char*		platformName(int) const;
    bool		platformIsGPU(int) const;

protected:
    GPUManagerData*	data_;
};

//Access. 
mGlobal GPUManager& manager();


}; // namespace


#endif
