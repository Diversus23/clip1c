
#include "stdafx.h"


#if defined( __linux__ ) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#endif

#include <stdio.h>
#include <wchar.h>
#include "AddInNative.h"
#include <string>

#ifdef WIN32
#pragma setlocale("ru-RU" )
#endif

static const wchar_t *g_PropNames[] = {
	L"Text",
	L"Image",
	L"Files",
	L"Format",
	L"Version",
};
static const wchar_t *g_MethodNames[] = {
	L"SetText",
	L"SetFiles",
	L"SetImage",
	L"Empty",
};

static const wchar_t *g_PropNamesRu[] = {
	L"Текст",
	L"Картинка",
	L"Мониторинг",
	L"Файлы",
	L"Формат",
	L"Версия",
};
static const wchar_t *g_MethodNamesRu[] = {
	L"ЗаписатьТекст",
	L"ЗаписатьФайлы",
	L"ЗаписатьКартинку",
	L"Очистить",
};

static const WCHAR_T g_kClassNames[] = u"CAddInNative"; //|OtherClass1|OtherClass2";

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static std::u16string s_names(g_kClassNames);
//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
    if(!*pInterface)
    {
        *pInterface= new CAddInNative();
        return (long)*pInterface;
    }
    return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
    g_capabilities = capabilities;
    return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
AttachType GetAttachType()
{
    return eCanAttachAny;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
    if(!*pIntf)
        return -1;

    delete *pIntf;
    *pIntf = 0;
    return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    return s_names.c_str();
}
//---------------------------------------------------------------------------//
//CAddInNative
CAddInNative::CAddInNative()
{
	m_iMemory = nullptr;
	m_iConnect = nullptr;
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{ 
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{ 
    return 2000; 
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{ 
	const wchar_t *wsExtension = L"AddInNativeExtension";
	size_t iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{ 
    return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{ 
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;

	return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{ 
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;
	size_t iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{ 
/*	switch (lPropNum)
	{
	case ePropIsEnabled:
		TV_VT(pvarPropVal) = VTYPE_BOOL;
		TV_BOOL(pvarPropVal) = m_boolEnabled;
		break;
	case ePropIsTimerPresent:
		TV_VT(pvarPropVal) = VTYPE_BOOL;
		TV_BOOL(pvarPropVal) = true;
		break;
	case ePropLocale:
	{
		if (m_iMemory)
		{
			TV_VT(pvarPropVal) = VTYPE_PWSTR;
			WCHAR_T *wsPropName = NULL;
			if (m_iMemory->AllocMemory((void**)&(pvarPropVal->pwstrVal), (unsigned)(m_userLang.size() + 1) * sizeof(WCHAR_T)))
			{
				memcpy(pvarPropVal->pwstrVal, m_userLang.data(), m_userLang.size() * sizeof(WCHAR_T));
				pvarPropVal->wstrLen = m_userLang.size();
			}
		}
		else
			TV_VT(pvarPropVal) = VTYPE_EMPTY;
	}
	break;
	default:
		return false;
	}

	return true;  */

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant *varPropVal)
{ 
	/*	switch (lPropNum)
	{
	case ePropIsEnabled:
		if (TV_VT(varPropVal) != VTYPE_BOOL)
			return false;
		m_boolEnabled = TV_BOOL(varPropVal);
		break;
	case ePropIsTimerPresent:
	case ePropLocale:
	default:
		return false;
	}

	return true;
	*/
    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{ 
	/*
	switch(lPropNum)
    { 
    case ePropIsEnabled:
    case ePropIsTimerPresent:
    case ePropLocale:
        return true;
    default:
        return false;
    }

    return false;
	*/
    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	/*
	switch(lPropNum)
    { 
    case ePropIsEnabled:
        return true;
    case ePropIsTimerPresent:
    case ePropLocale:
        return false;
    default:
        return false;
    }

    return false;
	*/
    return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{ 
    return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{ 
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{ 
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsMethodName = NULL;
	size_t iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{ 
	/*
	    switch(lMethodNum)
    { 
    case eMethShowInStatusLine:
        return 1;
    case eMethLoadPicture:
        return 1;
    case eLoopback:
        return 1;
    default:
        return 0;
    }
    
    return 0;
	*/
    return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
                        tVariant *pvarParamDefValue)
{ 
	/*
	TV_VT(pvarParamDefValue)= VTYPE_EMPTY;

    switch(lMethodNum)
    { 
    case eMethEnable:
    case eMethDisable:
    case eMethShowInStatusLine:
    case eMethStartTimer:
    case eMethStopTimer:
    case eMethShowMsgBox:
        // There are no parameter values by default 
        break;
    default:
        return false;
    }

    return false;
	*/
    return false;
} 
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{ 
	/*
	    switch(lMethodNum)
    { 
    case eMethLoadPicture:
    case eLoopback:
        return true;
    default:
        return false;
    }

    return false;*/
    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray)
{ 
	/*
	IAddInDefBaseEx* cnn = (IAddInDefBaseEx*)m_iConnect;
    if (eAppCapabilities2 <= g_capabilities && cnn)
    {
        IAttachedInfo* con_info = (IAttachedInfo*)cnn->GetInterface(eIAttachedInfo);
        if (con_info && con_info->GetAttachedInfo() == IAttachedInfo::eAttachedIsolated)
        {
            //host connected
        }
    }

    switch(lMethodNum)
    { 
    case eMethEnable:
        m_boolEnabled = true;
        break;
    case eMethDisable:
        m_boolEnabled = false;
        break;
    case eMethShowInStatusLine:
        if (m_iConnect && lSizeArray)
        {
            tVariant *var = paParams;
            m_iConnect->SetStatusLine(var->pwstrVal);
        }
        break;
    case eMethStartTimer:
        pAsyncEvent = m_iConnect;
#if !defined( __linux__ ) && !defined(__APPLE__)
	m_hTimerQueue = CreateTimerQueue();
	CreateTimerQueueTimer(&m_hTimer, m_hTimerQueue,
		(WAITORTIMERCALLBACK)MyTimerProc, 0, 1000, 1000, 0);
#else
	struct sigaction sa;
	struct itimerval tv;
	memset(&tv, 0, sizeof(tv));

	sa.sa_handler = MyTimerProc;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);
	tv.it_interval.tv_sec = 1;
	tv.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &tv, NULL);
#endif
	break;
    case eMethStopTimer:
#if !defined( __linux__ ) && !defined(__APPLE__)
		if (m_hTimer != 0)
		{
			DeleteTimerQueueTimer(m_hTimerQueue, m_hTimer, INVALID_HANDLE_VALUE);
			DeleteTimerQueue(m_hTimerQueue);
			m_hTimerQueue = 0;
			m_hTimer = 0;
		}
#else
		alarm(0);
#endif
		m_uiTimer = 0;
		pAsyncEvent = NULL;
		break;
	case eMethShowMsgBox:
	{
		if (eAppCapabilities1 <= g_capabilities)
		{
			IMsgBox* imsgbox = (IMsgBox*)cnn->GetInterface(eIMsgBox);
			if (imsgbox)
			{
				IPlatformInfo* info = (IPlatformInfo*)cnn->GetInterface(eIPlatformInfo);
				assert(info);
				const IPlatformInfo::AppInfo* plt = info->GetPlatformInfo();
				if (!plt)
					break;
				tVariant retVal;
				tVarInit(&retVal);
				if (imsgbox->Confirm(plt->AppVersion, &retVal))
				{
					bool succeed = TV_BOOL(&retVal);
					std::u16string result;

					if (succeed)
						result = load_wstring(m_userLang, IDS_TEXT_OK);
					else
						result = load_wstring(m_userLang, IDS_TEXT_CANCEL);

					imsgbox->Alert(result.c_str());

				}
			}

		}
	}
	break;
	default:
		return false;
}

return true;
*/
    return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{ 
	/*
	    bool ret = false;
    FILE *file = 0;
    char *name = 0;
    size_t size = 0;
    char *mbstr = 0;
    wchar_t* wsTmp = 0;
    char* loc = 0;

    switch(lMethodNum)
    {
        // Method acceps one argument of type BinaryData ant returns its copy
        case eLoopback:
        {
            if (lSizeArray != 1 || !paParams)
                return false;

            if (TV_VT(paParams) != VTYPE_BLOB)
            {
                addError(ADDIN_E_VERY_IMPORTANT, u"AddInNative",
                         load_wstring(m_userLang, IDS_ERR_TYPE_MISMATCH).c_str(), -1);
                return false;
            }

            if (paParams->strLen > 0)
            {
                m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, paParams->strLen);
                memcpy((void*)pvarRetValue->pstrVal, (void*)paParams->pstrVal, paParams->strLen);
            }

            TV_VT(pvarRetValue) = VTYPE_BLOB;
            pvarRetValue->strLen = paParams->strLen;
            return true;
        }
        break;

    case eMethLoadPicture:
        {
            if (!lSizeArray || !paParams)
                return false;
            
            switch(TV_VT(paParams))
            {
            case VTYPE_PSTR:
                name = paParams->pstrVal;
                break;
            case VTYPE_PWSTR:
                loc = setlocale(LC_ALL, "");
                ::convFromShortWchar(&wsTmp, TV_WSTR(paParams));
                size = wcstombs(0, wsTmp, 0)+1;
                assert(size);
                mbstr = new char[size];
                assert(mbstr);
                memset(mbstr, 0, size);
                size = wcstombs(mbstr, wsTmp, getLenShortWcharStr(TV_WSTR(paParams)));
                name = mbstr;
                setlocale(LC_ALL, loc);
                delete[] wsTmp;
                break;
            default:
                return false;
            }
        }
                
        file = fopen(name, "rb");

        if (file == 0)
        {
            wchar_t* wsMsgBuf;
            uint32_t err = errno;
            name = strerror(err);
            size_t sizeloc = mbstowcs(0, name, 0) + 1;
            assert(sizeloc);
            wsMsgBuf = new wchar_t[sizeloc];
            assert(wsMsgBuf);
            memset(wsMsgBuf, 0, sizeloc * sizeof(wchar_t));
            sizeloc = mbstowcs(wsMsgBuf, name, sizeloc);

            addError(ADDIN_E_VERY_IMPORTANT, L"AddInNative", wsMsgBuf, RESULT_FROM_ERRNO(err));
            delete[] wsMsgBuf;
            return false;
        }

        fseek(file, 0, SEEK_END);
        size = ftell(file);
        
        if (size && m_iMemory->AllocMemory((void**)&pvarRetValue->pstrVal, (unsigned)size))
        {
            fseek(file, 0, SEEK_SET);
            size = fread(pvarRetValue->pstrVal, 1, size, file);
            pvarRetValue->strLen = (unsigned)size;
            TV_VT(pvarRetValue) = VTYPE_BLOB;
            
            ret = true;
        }
        if (file)
            fclose(file);

        if (mbstr && size != -1)
            delete[] mbstr;

        break;
    }
    return ret; 
*/
    return false; 
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale(LC_ALL, (wchar_t*)loc);
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
#endif
}
//---------------------------------------------------------------------------//
void ADDIN_API CAddInNative::SetUserInterfaceLanguageCode(const WCHAR_T * lang)
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}

//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source,
	const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T *err = 0;
		WCHAR_T *descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const char16_t * source, const char16_t * descriptor, long code)
{
	if (m_iConnect)
	{
		m_iConnect->AddError(wcode, source, descriptor, code);
	}
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name,
	const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}

//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, size_t len)
{
    if (!len)
        len = ::wcslen(Source) + 1;

    if (!*Dest)
        *Dest = new WCHAR_T[len];

    WCHAR_T* tmpShort = *Dest;
    wchar_t* tmpWChar = (wchar_t*) Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(WCHAR_T));

#if defined( __linux__ ) || defined(__APPLE__)
    size_t succeed = (size_t)-1;
    size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    iconv_t cd = iconv_open("UTF-16LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif 
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpShort = (WCHAR_T)*tmpWChar;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
    if (!len)
        len = getLenShortWcharStr(Source) + 1;

    if (!*Dest)
        *Dest = new wchar_t[len];

    wchar_t* tmpWChar = *Dest;
    WCHAR_T* tmpShort = (WCHAR_T*)Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len * sizeof(wchar_t));
#if defined( __linux__ ) || defined(__APPLE__)
    size_t succeed = (size_t)-1;
    const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
    size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
    iconv_t cd = iconv_open("UTF-32LE", fromCode);
    if (cd != (iconv_t)-1)
    {
        succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
        iconv_close(cd);
        if(succeed != (size_t)-1)
            return (uint32_t)succeed;
    }
#endif 
    for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
    {
        *tmpWChar = (wchar_t)*tmpShort;
    }

    return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
    uint32_t res = 0;
    WCHAR_T *tmpShort = (WCHAR_T*)Source;

    while (*tmpShort++)
        ++res;

    return res;
}
//---------------------------------------------------------------------------//
