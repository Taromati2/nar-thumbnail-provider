#include "NarThumbnailProvider.h"
#include "GetThumbnail.h"
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

extern HINSTANCE g_hInst;
extern long g_cDllRef;

NarThumbnailProvider::NarThumbnailProvider() : m_cRef(1), m_pStream(NULL) {
	InterlockedIncrement(&g_cDllRef);
}

NarThumbnailProvider::~NarThumbnailProvider() {
	InterlockedDecrement(&g_cDllRef);
}

// IUnknown

IFACEMETHODIMP NarThumbnailProvider::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] =
	{
		QITABENT(NarThumbnailProvider, IThumbnailProvider),
		QITABENT(NarThumbnailProvider, IInitializeWithStream),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) NarThumbnailProvider::AddRef() {
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) NarThumbnailProvider::Release() {
	ULONG cRef = InterlockedDecrement(&m_cRef);

	if (0 == cRef)
		delete this;

	return cRef;
}

// IInitializeWithStream

IFACEMETHODIMP NarThumbnailProvider::Initialize(IStream *pStream, DWORD grfMode) {
	HRESULT hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

	if (m_pStream == NULL)
		hr = pStream->QueryInterface(&m_pStream);

	return hr;
}

// IThumbnailProvider

IFACEMETHODIMP NarThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
	*pdwAlpha = WTSAT_ARGB;
	*phbmp = GetNARThumbnail(m_pStream);

	// m_pStream->Release();

	return *phbmp != NULL ? NOERROR : E_NOTIMPL;
}
