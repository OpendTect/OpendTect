class __declspec( dllexport ) Mutex
{
public:
    Mutex () { InitializeCriticalSection (& _critSection); }
    ~Mutex () { DeleteCriticalSection (& _critSection); }
    void Lock()
    {
        EnterCriticalSection (& _critSection);
    }
    void UnLock()
    {
        LeaveCriticalSection (& _critSection);
    }
protected:
    CRITICAL_SECTION _critSection;
};
