#pragma once
#include <functional>
#include "COMBase.hpp"
#include "Common.hpp"
#include "UIElementEventArgs.hpp"
namespace libtf {
	class TFAPI UIElementSink :
		private COMBase,
		public ITfUIElementSink
	{
		typedef std::function<VOID(UIElementEventArgs*)> signal_UIElement;
		CComQIPtr<ITfSource>		m_source;
		DWORD						m_dwCookie = TF_INVALID_COOKIE;
	public:
		signal_UIElement			m_sigUIElement = [](UIElementEventArgs*) {};

		UIElementSink(IN CComPtrBase<ITfUIElementMgr> uiElementMgr) {
			m_source = uiElementMgr;
			THR_FAIL(m_source->AdviseSink(IID_ITfUIElementSink, (ITfUIElementSink*)this, &m_dwCookie), "Failed to Advisesink");
		}

		~UIElementSink() {
			if (m_dwCookie != TF_INVALID_COOKIE) {
				THR_FAIL(m_source->UnadviseSink(m_dwCookie), "Failed to Unadvisesink");
			}
		}
	private:
		HRESULT __stdcall BeginUIElement(DWORD dwUIElementId, BOOL* pbShow) override
		{
			*pbShow = TRUE;//Show IME's candidate window by default
			m_sigUIElement(new UIElementEventArgs(dwUIElementId, pbShow));
			return S_OK;
		}
		HRESULT __stdcall UpdateUIElement(DWORD dwUIElementId) override
		{
			m_sigUIElement(new UIElementEventArgs(UIElementState::Update, dwUIElementId));
			return S_OK;
		}
		HRESULT __stdcall EndUIElement(DWORD dwUIElementId) override
		{
			m_sigUIElement(new UIElementEventArgs(UIElementState::End, dwUIElementId));
			return S_OK;
		}

		HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override {
			COM_ASUNK(ITfUIElementSink);
			COM_RETURN;
		}
		COM_REFS
	};
}
