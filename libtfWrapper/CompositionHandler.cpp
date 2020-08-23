#include "pch.h"
#include "CompositionHandler.h"

CompositionHandler::CompositionHandler(TextStore* ts)
{
	sink_comp = gcnew CompositionSink_native(this, &CompositionHandler::onComposition);
	ts->m_sigComposition.connect(reinterpret_cast<CompositionSink_nativeType>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(sink_comp).ToPointer()));

	sink_ext = gcnew CompositionExtSink_native(this, &CompositionHandler::onCompositionExt);
	ts->m_sigGetCompExt.connect(reinterpret_cast<CompositionExtSink_nativeType>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(sink_ext).ToPointer()));
}

VOID CompositionHandler::onComposition(ITfContextOwnerCompositionSink* sink, CompositionEventArgs* comp)
{
	eventComposition((System::IntPtr)comp);
}

VOID CompositionHandler::onCompositionExt(TextStore* textStore, RECT* rect)
{
	eventGetCompExt((System::IntPtr)rect);
}