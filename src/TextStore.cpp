#include "TextStore.h"
#include "TextEditor.h"
#include "TextInputCtrl.h"
#include "InputScope.h"
#include "tsattrs.h"
#include <fmt/xchar.h>

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CTextStore::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextStoreACP))
    {
        *ppvObj = (ITextStoreACP *)this;
    }
    else if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
    {
        *ppvObj = (ITfContextOwnerCompositionSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfMouseTrackerACP))
    {
        *ppvObj = (ITfMouseTrackerACP *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTextStore::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CTextStore::Release()
{
    long cr;

    cr = --_cRef;

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] AdviseSink mask=0x{:08X} punk={}\n",
                                  static_cast<unsigned int>(dwMask), (void *)punk)
                          .c_str());

    if (!IsEqualGUID(riid, IID_ITextStoreACPSink))
    {
        OutputDebugString(L"[TSF][TextStore] AdviseSink no object: unsupported riid\n");
        return TS_E_NOOBJECT;
    }

    if (FAILED(punk->QueryInterface(IID_ITextStoreACPSink, (void **)&_pSink)))
    {
        OutputDebugString(L"[TSF][TextStore] AdviseSink QueryInterface failed\n");
        return E_NOINTERFACE;
    }

    OutputDebugString(L"[TSF][TextStore] AdviseSink success\n");
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::UnadviseSink(IUnknown *punk)
{
    // we're dealing with TSF. We don't have to check punk is same instance of _pSink.
    if (_pSink)
    {
        _pSink->Release();
        _pSink = NULL;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::RequestLock(DWORD dwLockFlags, HRESULT *phrSession)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] RequestLock flags=0x{:08X} sink={}\n",
                                  static_cast<unsigned int>(dwLockFlags), (void *)_pSink)
                          .c_str());
    if (!_pSink)
    {
        return E_UNEXPECTED;
    }

    *phrSession = _pSink->OnLockGranted(dwLockFlags);
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetStatus(TS_STATUS *pdcs)
{
    OutputDebugString(L"[TSF][TextStore] GetStatus\n");
    pdcs->dwDynamicFlags = 0;
    pdcs->dwStaticFlags = 0;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::QueryInsert(LONG acpInsertStart, LONG acpInsertEnd, ULONG cch, LONG *pacpResultStart,
                               LONG *pacpResultEnd)
{
    *pacpResultStart = acpInsertStart;
    *pacpResultEnd = acpInsertEnd;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] GetSelection index={} count={}\n", ulIndex, ulCount).c_str());
    *pcFetched = 0;
    if ((ulCount > 0) && ((ulIndex == 0) || (ulIndex == TS_DEFAULT_SELECTION)))
    {
        pSelection[0].acpStart = _pEditor->GetSelectionStart();
        pSelection[0].acpEnd = _pEditor->GetSelectionEnd();
        pSelection[0].style.ase = TS_AE_END;
        pSelection[0].style.fInterimChar = _pEditor->GetLayout()->IsInterimCaret();
        *pcFetched = 1;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
    if (ulCount > 0)
    {
        _pEditor->MoveSelection(pSelection[0].acpStart, pSelection[0].acpEnd);
        _pEditor->UpdateLayout();
        _pEditor->InvalidateRect();
        _pEditor->SetInterimCaret(pSelection->style.fInterimChar);
    }

    return S_OK;
    ;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetText(LONG acpStart, LONG acpEnd, __out_ecount(cchPlainReq) WCHAR *pchPlain, ULONG cchPlainReq,
                           ULONG *pcchPlainOut, TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut,
                           LONG *pacpNext)
{

    if ((cchPlainReq == 0) && (ulRunInfoReq == 0))
    {
        return S_OK;
    }

    if (acpEnd == -1)
        acpEnd = _pEditor->GetTextLength();

    acpEnd = min(acpEnd, acpStart + (int)cchPlainReq);

    if ((acpStart != acpEnd) && !_pEditor->GetText(acpStart, pchPlain, acpEnd - acpStart))
    {
        return E_FAIL;
    }

    *pcchPlainOut = acpEnd - acpStart;
    if (ulRunInfoReq)
    {
        prgRunInfo[0].uCount = acpEnd - acpStart;
        prgRunInfo[0].type = TS_RT_PLAIN;
        *pulRunInfoOut = 1;
    }

    *pacpNext = acpEnd;

    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, __in_ecount(cch) const WCHAR *pchText, ULONG cch,
                           TS_TEXTCHANGE *pChange)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] SetText flags=0x{:08X} start={} end={} cch={} hasComp={}\n",
                                  static_cast<unsigned int>(dwFlags), acpStart, acpEnd, cch,
                                  _pCurrentCompositionView != NULL)
                          .c_str());
    // Check the composition status
    if (cch == 0 && _pCurrentCompositionView)
    {
        OutputDebugString(L"[TSF][TextStore] SetText requests terminate composition\n");
        _pEditor->TerminateCompositionString();
    }

    LONG acpRemovingEnd;

    if (acpStart > (LONG)_pEditor->GetTextLength())
    {
        OutputDebugString(L"[TSF][TextStore] SetText invalid argument: start beyond text length\n");
        return E_INVALIDARG;
    }

    acpRemovingEnd = min(acpEnd, (LONG)_pEditor->GetTextLength() + 1);
    if (!_pEditor->RemoveText(acpStart, acpRemovingEnd - acpStart))
        return E_FAIL;

    if (!_pEditor->InsertText(acpStart, pchText, cch))
        return E_FAIL;

    pChange->acpStart = acpStart;
    pChange->acpOldEnd = acpEnd;
    pChange->acpNewEnd = acpStart + cch;

    // Update selection after text change
    _pEditor->MoveSelection(acpStart + cch, acpStart + cch);
    _pEditor->UpdateLayout();

    _pEditor->InvalidateRect();
    OutputDebugString(fmt::format(L"[TSF][TextStore] SetText complete newEnd={}\n", pChange->acpNewEnd).c_str());
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject,
                                  TS_TEXTCHANGE *pChange)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] RequestSupportedAttrs flags=0x{:08X} filterCount={}\n",
                                  static_cast<unsigned int>(dwFlags), cFilterAttrs)
                          .c_str());
    PrepareAttributes(cFilterAttrs, paFilterAttrs);
    if (!_nAttrVals)
        return S_FALSE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
                                          DWORD dwFlags)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] RequestAttrsAtPosition pos={} flags=0x{:08X} filterCount={}\n",
                                  acpPos, static_cast<unsigned int>(dwFlags), cFilterAttrs)
                          .c_str());
    PrepareAttributes(cFilterAttrs, paFilterAttrs);
    if (!_nAttrVals)
        return S_FALSE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
                                                       DWORD dwFlags)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
                                          const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound,
                                          LONG *plFoundOffset)
{
    *pacpNext = 0;
    *pfFound = FALSE;
    *plFoundOffset = 0;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] RetrieveRequestedAttrs count={}\n", ulCount).c_str());
    *pcFetched = 0;
    for (int i = 0; (i < (int)ulCount) && (i < _nAttrVals); i++)
    {
        paAttrVals[i] = _attrval[i];
        (*pcFetched)++;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetEndACP(LONG *pacp)
{
    *pacp = _pEditor->GetTextLength();
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetActiveView(TsViewCookie *pvcView)
{
    *pvcView = 0;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] GetTextExt view={} start={} end={}\n", vcView, acpStart, acpEnd)
                          .c_str());
    RECT rcStart;
    RECT rcEnd;

    if (_pEditor->GetLayout()->RectFromCharPos(acpStart, &rcStart) &&
        _pEditor->GetLayout()->RectFromCharPos(acpEnd, &rcEnd))
    {
        if (rcStart.top == rcEnd.top)
        {
            prc->left = min(rcStart.left, rcEnd.left);
            prc->right = max(rcStart.right, rcEnd.right);
        }
        else
        {
            RECT rcClient;
            GetClientRect(_pEditor->GetWnd(), &rcClient);
            prc->left = _pEditor->GetLayout()->GetPaddingLeftPixels();
            prc->right = max(prc->left, rcClient.right - _pEditor->GetLayout()->GetPaddingRightPixels());
        }
        prc->top = min(rcStart.top, rcEnd.top);
        prc->bottom = max(rcStart.bottom, rcEnd.bottom);
    }
    else
    {
        prc->left = 0;
        prc->right = 0;
        prc->top = 0;
        prc->bottom = 0;
    }

    ClientToScreen(_pEditor->GetWnd(), (POINT *)&prc->left);
    ClientToScreen(_pEditor->GetWnd(), (POINT *)&prc->right);
    *pfClipped = FALSE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetScreenExt(TsViewCookie vcView, RECT *prc)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] GetScreenExt view={}\n", vcView).c_str());
    GetClientRect(_pEditor->GetWnd(), prc);
    ClientToScreen(_pEditor->GetWnd(), (POINT *)&prc->left);
    ClientToScreen(_pEditor->GetWnd(), (POINT *)&prc->right);
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::GetWnd(TsViewCookie vcView, HWND *phwnd)
{
    OutputDebugString(fmt::format(L"[TSF][TextStore] GetWnd view={}\n", vcView).c_str());
    *phwnd = _pEditor->GetWnd();
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::InsertTextAtSelection(DWORD dwFlags, __in_ecount(cch) const WCHAR *pchText, ULONG cch,
                                         LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
    LONG acpStart = _pEditor->GetSelectionStart();
    LONG acpEnd = _pEditor->GetSelectionEnd();
    OutputDebugString(fmt::format(L"[TSF][TextStore] InsertTextAtSelection flags=0x{:08X} selStart={} selEnd={} cch={}\n",
                                  static_cast<unsigned int>(dwFlags), acpStart, acpEnd, cch)
                          .c_str());

    if (dwFlags & TS_IAS_QUERYONLY)
    {
        *pacpStart = acpStart;
        *pacpEnd = acpStart + cch;
        return S_OK;
    }

    if (!_pEditor->RemoveText(acpStart, acpEnd - acpStart))
        return E_FAIL;

    if (pchText && !_pEditor->InsertText(acpStart, pchText, cch))
        return E_FAIL;

    if (pacpStart)
    {
        *pacpStart = acpStart;
    }

    if (pacpEnd)
    {
        *pacpEnd = acpStart + cch;
    }

    if (pChange)
    {
        pChange->acpStart = acpStart;
        pChange->acpOldEnd = acpEnd;
        pChange->acpNewEnd = acpStart + cch;
    }

    _pEditor->MoveSelection(acpStart, acpStart + cch);
    _pEditor->UpdateLayout();
    _pEditor->InvalidateRect();
    OutputDebugString(fmt::format(L"[TSF][TextStore] InsertTextAtSelection complete newSelEnd={}\n", acpStart + cch)
                          .c_str());
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd,
                                             TS_TEXTCHANGE *pChange)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::AdviseMouseSink(ITfRangeACP *range, ITfMouseSink *pSink, DWORD *pdwCookie)
{
    DWORD dwCookie;
    if (!_prgMouseSinks)
    {
        _prgMouseSinks = (MOUSESINK *)LocalAlloc(LPTR, sizeof(MOUSESINK));
        if (!_prgMouseSinks)
        {
            return E_OUTOFMEMORY;
        }
        _nMouseSinks = 1;
        dwCookie = 1;
    }
    else
    {
        UINT i;
        for (i = 0; i < _nMouseSinks; i++)
        {
            if (_prgMouseSinks[i].pRange == NULL)
            {
                dwCookie = i + 1;
                break;
            }
        }
        if (i == _nMouseSinks)
        {
            void *pvNew =
                LocalReAlloc(_prgMouseSinks, (_nMouseSinks + 1) * sizeof(MOUSESINK), LMEM_MOVEABLE | LMEM_ZEROINIT);
            if (!pvNew)
            {
                return E_OUTOFMEMORY;
            }
            _prgMouseSinks = (MOUSESINK *)pvNew;
            _nMouseSinks++;
            dwCookie = _nMouseSinks;
        }
    }

    _prgMouseSinks[dwCookie - 1].pRange = range;
    _prgMouseSinks[dwCookie - 1].pMouseSink = pSink;
    _prgMouseSinks[dwCookie - 1].pRange->AddRef();
    _prgMouseSinks[dwCookie - 1].pMouseSink->AddRef();
    *pdwCookie = dwCookie;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::UnadviseMouseSink(DWORD dwCookie)
{
    if ((dwCookie == 0) || (dwCookie > _nMouseSinks))
    {
        return E_INVALIDARG;
    }

    if (_prgMouseSinks == NULL)
    {
        return E_UNEXPECTED;
    }

    if (_prgMouseSinks[dwCookie - 1].pRange == NULL || _prgMouseSinks[dwCookie - 1].pMouseSink == NULL)
    {
        return E_INVALIDARG;
    }

    _prgMouseSinks[dwCookie - 1].pRange->Release();
    _prgMouseSinks[dwCookie - 1].pRange = NULL;

    _prgMouseSinks[dwCookie - 1].pMouseSink->Release();
    _prgMouseSinks[dwCookie - 1].pMouseSink = NULL;

    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

#define IF_ATTR_FONT_STYLE_HEIGHT 1
#define IF_ATTR_FONT_SIZEPTS 2
#define IF_ATTR_TEXT_READONLY 3
#define IF_ATTR_TEXT_ORIENTATION 4
#define IF_ATTR_TEXT_VERTICALWRITING 5

const GUID *c_rgSupportedAttr[5] = {&TSATTRID_Font_Style_Height, &TSATTRID_Font_SizePts, &TSATTRID_Text_ReadOnly,
                                    &TSATTRID_Text_Orientation, &TSATTRID_Text_VerticalWriting};

void CTextStore::PrepareAttributes(ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
    _nAttrVals = 0;
    memset(_attrval, 0, sizeof(_attrval));

    for (int i = 0; i < ARRAYSIZE(c_rgSupportedAttr); i++)
    {
        if (cFilterAttrs)
        {
            BOOL fFound = FALSE;
            for (ULONG j = 0; j < cFilterAttrs; j++)
            {
                if (IsEqualGUID(*c_rgSupportedAttr[i], paFilterAttrs[j]))
                {
                    fFound = TRUE;
                    break;
                }
            }

            if (!fFound)
            {
                continue;
            }
        }

        _attrval[_nAttrVals].idAttr = *c_rgSupportedAttr[i];
        _attrval[_nAttrVals].dwOverlapId = i + 1;

        switch (i + 1)
        {
        case IF_ATTR_FONT_STYLE_HEIGHT:
            _attrval[_nAttrVals].varValue.vt = VT_I4;
            _attrval[_nAttrVals].varValue.lVal = _pEditor->GetLineHeight();
            break;

        case IF_ATTR_FONT_SIZEPTS:
            _attrval[_nAttrVals].varValue.vt = VT_I4;
            _attrval[_nAttrVals].varValue.lVal = (int)((double)_pEditor->GetLineHeight() / 96.0 * 72.0);
            break;

        case IF_ATTR_TEXT_READONLY:
            _attrval[_nAttrVals].varValue.vt = VT_BOOL;
            _attrval[_nAttrVals].varValue.bVal = FALSE;
            break;

        case IF_ATTR_TEXT_ORIENTATION:
            _attrval[_nAttrVals].varValue.vt = VT_I4;
            _attrval[_nAttrVals].varValue.lVal = 0;
            break;

        case IF_ATTR_TEXT_VERTICALWRITING:
            _attrval[_nAttrVals].varValue.vt = VT_BOOL;
            _attrval[_nAttrVals].varValue.bVal = FALSE;
            break;
        }

        _nAttrVals++;
    }
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk)
{
    if (_pCurrentCompositionView)
    {
        _pCurrentCompositionView->Release();
        _pCurrentCompositionView = NULL;
    }

    _pCurrentCompositionView = pComposition;
    OutputDebugString(fmt::format(L"[TSF][TextStore] OnStartComposition view={}\n", (void *)pComposition).c_str());
    _pCurrentCompositionView->AddRef();

    *pfOk = TRUE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew)
{
    if (_pCurrentCompositionView)
    {
        _pCurrentCompositionView->Release();
        _pCurrentCompositionView = NULL;
    }

    OutputDebugString(fmt::format(L"[TSF][TextStore] OnUpdateComposition view={} range={}\n", (void *)pComposition,
                                  (void *)pRangeNew)
                          .c_str());

    _pCurrentCompositionView = pComposition;
    _pCurrentCompositionView->AddRef();
    return S_OK;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

STDAPI CTextStore::OnEndComposition(ITfCompositionView *pComposition)
{
    if (_pCurrentCompositionView)
    {
        _pCurrentCompositionView->Release();
        _pCurrentCompositionView = NULL;
    }

    OutputDebugString(fmt::format(L"[TSF][TextStore] OnEndComposition view={}\n", (void *)pComposition).c_str());
    // Reset selection to current position
    LONG nSelStart = _pEditor->GetSelectionStart();
    _pEditor->MoveSelection(nSelStart, nSelStart);

    return S_OK;
}
