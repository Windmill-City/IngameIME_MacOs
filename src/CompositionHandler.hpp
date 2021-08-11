#pragma once
#include "CommitHandler.hpp"
#include "libtfdef.h"
#include <atlbase.h>
#include <atlcom.h>
#include <functional>
#include <msctf.h>

extern "C"
{
    typedef enum libtf_CompositionState
    {
        libtf_CompositionBegin,
        libtf_CompositionUpdate,
        libtf_CompositionEnd
    } libtf_CompositionState_t;
    typedef RECT libtf_BoundingBox_t;
    typedef BSTR libtf_PreEdit;
    typedef struct libtf_Composition
    {
        libtf_CompositionState_t state;
        /**
         * @brief Only Available at CompositionUpdate
         */
        libtf_PreEdit preEdit;
        /**
         * @brief Only Available at CompositionUpdate
         */
        long selection[2];
    } libtf_Composition_t, *libtf_pComposition;
    typedef void (*libtf_CallbackComposition)(libtf_Composition_t);
    typedef void (*libtf_CallbackBoundingBox)(libtf_BoundingBox_t *);
}

namespace libtf
{
    class CompositionHandler : public CComObjectRoot, public ITfContextOwnerCompositionSink, public ITfTextEditSink
    {
    protected:
        TfClientId m_clientId;
        CComPtr<ITfContext> m_context;
        DWORD m_textEditSinkCookie;
        CComPtr<ITfCompositionView> m_compositionView;

    public:
        /**
         * @brief Callback when PreEdit updates
         */
        typedef std::function<void(libtf_Composition_t)> signalComposition;
        signalComposition m_sigComposition = [](libtf_Composition_t) {};

        /**
         * @brief Callback to get PreEdit's BoundingBox
         * for positioning Candidate List window
         */
        typedef std::function<void(libtf_BoundingBox_t *)> signalBoundingBox;
        signalBoundingBox m_sigBoundingBox = [](libtf_BoundingBox_t *) {};

        /**
         * @brief Handle input method commit str
         */
        CComPtr<CCommitHandler> m_commitHandler = new CCommitHandler();

        BEGIN_COM_MAP(CompositionHandler)
        COM_INTERFACE_ENTRY(ITfContextOwnerCompositionSink)
        COM_INTERFACE_ENTRY(ITfTextEditSink)
        END_COM_MAP()

        /**
         * @brief Initialize handler
         * 
         * @param clientId ClientId to request edit session
         * @param context ITfContext to request edit session
         * @return HRESULT 
         */
        HRESULT initialize(TfClientId clientId, CComPtr<ITfContext> context)
        {
            m_clientId = clientId;
            m_context = context;
            CHECK_HR(m_commitHandler->initialize(m_context));

            CComQIPtr<ITfSource> evtCtx = m_context;
            CHECK_HR(evtCtx->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink *)this, &m_textEditSinkCookie));

            return S_OK;
        }

        /**
         * @brief Dispose the handler
         * 
         * @return HRESULT 
         */
        HRESULT dispose()
        {
            CHECK_HR(m_commitHandler->dispose());

            CComQIPtr<ITfSource> evtCtx = m_context;
            CHECK_HR(evtCtx->UnadviseSink(m_textEditSinkCookie));

            return S_OK;
        }

#pragma region ITfContextOwnerCompositionSink
        /**
         * @brief Always allow Composition start
         * 
         * @param pfOk allow Composition start?
         * @return HRESULT 
         */
        HRESULT OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk) override
        {
            //Always allow Composition start
            *pfOk = TRUE;

            m_sigComposition({libtf_CompositionBegin});

            return S_OK;
        }

        /**
         * @brief Handle Composition Update at ITfTextEditSink
         * 
         * @return HRESULT 
         */
        HRESULT OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew) override
        {
            m_compositionView = pComposition;
            return S_OK;
        }

        /**
         * @brief Handle Composition End
         * 
         * @return HRESULT 
         */
        HRESULT OnEndComposition(ITfCompositionView *pComposition) override
        {
            m_sigComposition({libtf_CompositionEnd});

            m_compositionView.Release();

            HRESULT hr;
            CHECK_HR(m_context->RequestEditSession(m_clientId, m_commitHandler, TF_ES_ASYNC | TF_ES_READWRITE, &hr));
            return hr;
        }
#pragma endregion
#pragma region ITfTextEditSink
        /**
         * @brief Get PreEdit text and its selection
         */
        HRESULT OnEndEdit(ITfContext *pic, TfEditCookie ec, ITfEditRecord *pEditRecord) override
        {
            //No active composition
            if (!m_compositionView) return S_OK;

            CComPtr<ITfRange> preEditRange;
            CHECK_HR(m_compositionView->GetRange(&preEditRange));

            ULONG charCount;
            WCHAR *buf = new WCHAR[65];
            CHECK_OOM(buf);

            CHECK_HR(preEditRange->GetText(ec, 0, buf, 64, &charCount));
            BSTR bstr = SysAllocStringLen(buf, charCount);
            delete[] buf;
            CHECK_OOM(bstr);

            TF_SELECTION sel[1];
            ULONG fetched;
            CComPtr<ITfRange> selRange;
            CHECK_HR(m_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, sel, &fetched));
            selRange.Attach(sel[0].range);
            CComQIPtr<ITfRangeACP> selRangeAcp = selRange;
            LONG acpStart, len;
            CHECK_HR(selRangeAcp->GetExtent(&acpStart, &len));

            libtf_Composition_t composition = {libtf_CompositionUpdate, bstr, {acpStart, acpStart + len}};
            m_sigComposition(composition);

            //Cleanup
            SysFreeString(bstr);

            return S_OK;
        }
#pragma endregion
    };

    typedef CComObjectNoLock<CompositionHandler> CCompositionHandler;
}