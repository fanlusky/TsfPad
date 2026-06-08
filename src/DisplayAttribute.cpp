#include "DisplayAttribute.h"
#include <string>
#include <Windows.h>
#include <msctf.h>
#include <fmt/xchar.h>

ITfDisplayAttributeMgr *g_pdam = NULL;

std::wstring ColorTypeToStr(TF_DA_COLORTYPE type)
{
    std::string res;
    switch (type)
    {
    case TF_CT_NONE:
        res = "None";
        break;
    case TF_CT_SYSCOLOR:
        res = "SysColor";
        break;
    case TF_CT_COLORREF:
        res = "ColorRef";
        break;
    default:
        res = "Unknown";
    }
    return std::wstring(res.begin(), res.end());
}

std::wstring UnderlineStyleToStr(TF_DA_LINESTYLE style)
{
    std::string res;
    switch (style)
    {
    case TF_LS_NONE:
        res = "None";
        break;
    case TF_LS_SOLID:
        res = "Solid";
        break;
    case TF_LS_DOT:
        res = "Dot";
        break;
    case TF_LS_DASH:
        res = "Dash";
        break;
    case TF_LS_SQUIGGLE:
        res = "Squiggle";
        break;
    default:
        res = "Unknown";
    }
    return std::wstring(res.begin(), res.end());
}

void PrintColor(const TF_DA_COLOR &color)
{
    OutputDebugString(fmt::format(ColorTypeToStr(color.type)).c_str());
    if (color.type == TF_CT_COLORREF)
    {
        COLORREF cr = color.cr;
    }
    else if (color.type == TF_CT_SYSCOLOR)
    {
    }
}

void PrintDisplayAttribute(const TF_DISPLAYATTRIBUTE &da)
{
    OutputDebugString(fmt::format(UnderlineStyleToStr(da.lsStyle)).c_str());
}

CDispAttrProps *GetDispAttrProps()
{
    IEnumGUID *pEnumProp = NULL;
    CDispAttrProps *pProps = NULL;
    ITfCategoryMgr *pcat;
    HRESULT hr = E_FAIL;
    if (SUCCEEDED(hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                        (void **)&pcat)))
    {
        hr = pcat->EnumItemsInCategory(GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY, &pEnumProp);
        pcat->Release();
    }

    //
    // make a database for Display Attribute Properties.
    //
    if (SUCCEEDED(hr) && pEnumProp)
    {
        GUID guidProp;
        pProps = new CDispAttrProps;

        //
        // add System Display Attribute first.
        // so no other Display Attribute property overwrite it.
        //
        pProps->Add(GUID_PROP_ATTRIBUTE);
        while (pEnumProp->Next(1, &guidProp, NULL) == S_OK)
        {
            if (!IsEqualGUID(guidProp, GUID_PROP_ATTRIBUTE))
                pProps->Add(guidProp);
        }
    }

    if (pEnumProp)
        pEnumProp->Release();

    return pProps;
}

//+---------------------------------------------------------------------------
//
//  InitDisplayAttributeLib
//
//----------------------------------------------------------------------------

HRESULT InitDisplayAttrbute()
{

    OutputDebugString(fmt::format(L"InitDisplayAttrbute tsfpad").c_str());

    if (g_pdam)
        g_pdam->Release();

    g_pdam = NULL;

    if (FAILED(CoCreateInstance(CLSID_TF_DisplayAttributeMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfDisplayAttributeMgr,
                                (void **)&g_pdam)))
    {
        OutputDebugString(fmt::format(L"CoCreateInstance failed").c_str());
        return E_FAIL;
    }

    if (g_pdam == NULL)
    {
        OutputDebugString(fmt::format(L"CoCreateInstance failed02").c_str());
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  UninitDisplayAttributeLib
//
//----------------------------------------------------------------------------

HRESULT UninitDisplayAttrbute()
{

    if (g_pdam)
    {
        g_pdam->Release();
        g_pdam = NULL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  GetDisplayAttributeTrackPropertyRange
//
//----------------------------------------------------------------------------

HRESULT GetDisplayAttributeTrackPropertyRange( //
    TfEditCookie ec,                           //
    ITfContext *pic,                           //
    ITfRange *pRange,                          //
    ITfReadOnlyProperty **ppProp,              //
    CDispAttrProps *pDispAttrProps             //
)
{
    ITfReadOnlyProperty *pProp = NULL;
    HRESULT hr = E_FAIL;
    GUID *pguidProp = NULL;
    const GUID **ppguidProp;
    ULONG ulNumProp = 0;
    ULONG i;

    if (!pDispAttrProps)
        goto Exit;

    pguidProp = pDispAttrProps->GetPropTable();
    if (!pguidProp)
        goto Exit;

    ulNumProp = pDispAttrProps->Count();
    if (!ulNumProp)
        goto Exit;

    // TrackProperties wants an array of GUID *'s
    if ((ppguidProp = (const GUID **)LocalAlloc(LMEM_ZEROINIT, sizeof(GUID *) * ulNumProp)) == NULL)
        return E_OUTOFMEMORY;

    for (i = 0; i < ulNumProp; i++)
    {
        ppguidProp[i] = pguidProp++;
    }

    if (SUCCEEDED(hr = pic->TrackProperties(ppguidProp, ulNumProp, 0, NULL, &pProp)))
    {
        *ppProp = pProp;
    }

    LocalFree(ppguidProp);

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetDisplayAttributeData
//
//----------------------------------------------------------------------------

HRESULT GetDisplayAttributeData(TfEditCookie ec, ITfReadOnlyProperty *pProp, ITfRange *pRange, TF_DISPLAYATTRIBUTE *pda,
                                TfGuidAtom *pguid)
{
    VARIANT var;
    IEnumTfPropertyValue *pEnumPropertyVal;
    TF_PROPERTYVAL tfPropVal;
    GUID guid;
    TfGuidAtom gaVal;
    ITfDisplayAttributeInfo *pDAI;

    HRESULT hr = E_FAIL;

    ITfCategoryMgr *pcat = NULL;
    if (FAILED(hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                     (void **)&pcat)))
    {
        return hr;
    }

    hr = S_FALSE;
    if (SUCCEEDED(pProp->GetValue(ec, pRange, &var)))
    {
        OutputDebugString(fmt::format(L"GetDisplayAttributeData01").c_str());
        if (SUCCEEDED(var.punkVal->QueryInterface(IID_IEnumTfPropertyValue, (void **)&pEnumPropertyVal)))
        {
            OutputDebugString(fmt::format(L"GetDisplayAttributeData02").c_str());
            while (pEnumPropertyVal->Next(1, &tfPropVal, NULL) == S_OK)
            {
                if (tfPropVal.varValue.vt == VT_EMPTY)
                    continue; // prop has no value over this span

                gaVal = (TfGuidAtom)tfPropVal.varValue.lVal;

                pcat->GetGUID(gaVal, &guid);

                if ((g_pdam != NULL))
                {
                    OutputDebugString(fmt::format(L"GetDisplayAttributeData2.5").c_str());
                }

                if ((g_pdam != NULL) && SUCCEEDED(g_pdam->GetDisplayAttributeInfo(guid, &pDAI, NULL)))
                {
                    OutputDebugString(fmt::format(L"GetDisplayAttributeData03").c_str());
                    //
                    // Issue: for simple apps.
                    //
                    // Small apps can not show multi underline. So
                    // this helper function returns only one
                    // DISPLAYATTRIBUTE structure.
                    //
                    if (pda)
                    {
                        pDAI->GetAttributeInfo(pda);
                        PrintDisplayAttribute(*pda);
                    }

                    if (pguid)
                    {
                        *pguid = gaVal;
                    }

                    pDAI->Release();
                    hr = S_OK;
                    break;
                }
            }
            pEnumPropertyVal->Release();
        }
        VariantClear(&var);
    }

    pcat->Release();

    return hr;
}
