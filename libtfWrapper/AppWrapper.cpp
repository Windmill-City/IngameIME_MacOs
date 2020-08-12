#include "pch.h"
#include<boost/bind.hpp>
#include "AppWrapper.h"

AppWrapper::AppWrapper()
{
	m_App = new Application();
	HRESULT hr = m_App->Initialize();
	THROWHR(hr, "Failed to Initialize Application");
}

AppWrapper::~AppWrapper()
{
	if (m_TextStore) m_TextStore->Release();
	if (m_App)
		if (m_App->m_ClientId != TF_CLIENTID_NULL)
			m_App->m_pThreadMgr->Deactivate();
}

VOID AppWrapper::Initialize(HWND hWnd)
{
	//MS Pinyin cant open candidate window when using normal active with ITfContextOwnerCompositionSink
	//m_App->m_pThreadMgrEx->ActivateEx(&(m_App->m_ClientId), TF_TMAE_UIELEMENTENABLEDONLY);
	m_App->m_pThreadMgr->Activate(&(m_App->m_ClientId));
	m_Doc = new Document(m_App, hWnd);

	m_TextStore = new TextStore(hWnd);
	m_TextStore->AddRef();
	//reg events
	using namespace System::Runtime::InteropServices;
	m_TextStore->m_sigGetCompExt.connect(Marshal::GetFunctionPointerForDelegate(gcnew GetCompsitionExtDelegate(this, &AppWrapper::onGetCompsitionExt)).ToPointer());
	//m_TextStore->m_sigCommitStr.connect(Marshal::GetFunctionPointerForDelegate(gcnew CommitDelegate(this, &AppWrapper::onCommit)).ToPointer());
	//m_TextStore->m_sigUpdateCompStr.connect(Marshal::GetFunctionPointerForDelegate(gcnew CompStrDelegate(this, &AppWrapper::onCompStr)).ToPointer());
	//push ctx
	m_Ctx = new Context(m_Doc, (ITextStoreACP2*)m_TextStore);
	DisableIME();//Disable input before push, in case start composition
	m_Doc->m_pDocMgr->Push(m_Ctx->m_pCtx.p);
	m_Initilized = TRUE;
}

VOID AppWrapper::onCommit(TextStore* textStore, std::wstring commitStr)
{
	//todo: handle commit
}

VOID AppWrapper::onCompStr(TextStore* textStore, std::wstring compStr)
{
	//todo: handle compstr
}

VOID AppWrapper::onGetCompsitionExt(TextStore* textStore, RECT* rect)
{
	//todo: handle getext
}

VOID AppWrapper::DisableIME()
{
	if (m_IsIMEEnabled) {
		m_IsIMEEnabled = FALSE;
		/*
		By default, the TSF manager will process keystrokesand pass them to the text services.
		An application prevents this by calling this method.
		Typically, this method is called when text service input is inappropriate, for example when a menu is displayed.
		Calls to this method are cumulative, so every call to this method requires a subsequent call to ITfConfigureSystemKeystrokeFeed::EnableSystemKeystrokeFeed.

		So we use a bool to prevent multiple disable here
		*/
		m_App->m_pCfgSysKeyFeed->DisableSystemKeystrokeFeed();
		m_Ctx->m_pCtxOwnerCompServices->TerminateComposition(NULL);//pass NULL to terminate all composition
	}
}

VOID AppWrapper::EnableIME()
{
	if (!m_IsIMEEnabled)
		m_IsIMEEnabled = TRUE;
	m_App->m_pCfgSysKeyFeed->EnableSystemKeystrokeFeed();
}