
#include "GetThumbnail.h"

#include <string_view>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "../LZMA-SDK/C/CpuArch.h"

#include "../LZMA-SDK/C/7z.h"
#include "../LZMA-SDK/C/7zAlloc.h"
#include "../LZMA-SDK/C/7zBuf.h"
#include "../LZMA-SDK/C/7zCrc.h"
#include "../LZMA-SDK/C/7zFile.h"
#include "../LZMA-SDK/C/7zVersion.h"


struct DATABLOCK{
	PBYTE data;
	DWORD size;
};
void DESTORY(DATABLOCK *dbp) {
	free(dbp->data);
}
DATABLOCK COPY(const DATABLOCK *dbp) {
	DATABLOCK aret=*dbp;
	aret.data = (PBYTE)malloc(aret.size);
	if(aret.data)
		memcpy(aret.data, dbp->data, aret.size);
	else
		aret.size = 0;
	return aret;
}

HBITMAP HICON_to_HBITMAP(HICON hIcon) {
	if(!hIcon)
		return NULL;
	ICONINFO iconinfo;
	GetIconInfo(hIcon, &iconinfo);
	HBITMAP hResultBmp = iconinfo.hbmColor;
	DeleteObject(iconinfo.hbmMask);
	DeleteObject(hIcon);
	return hResultBmp;
}


#define kInputBufSize ((size_t)1 << 18)
SRes IStream_Read(const ISeekInStream *p, void *buf, size_t *size) {
	IStream *is = (IStream *)((const CFileInStream *)(p))->file.handle;
	ULONG	 tmp;
	is->Read(buf,*size,&tmp);
	*size = tmp;
	return SZ_OK;
}
SRes IStream_Seek(const ISeekInStream *p, Int64 *pos, ESzSeek origin) {
	IStream *is = (IStream *)((const CFileInStream *)(p))->file.handle;
	LARGE_INTEGER tmp{};
	tmp.QuadPart = *pos;
	ULARGE_INTEGER tmp2;
	is->Seek(tmp,origin,&tmp2);
	*pos = tmp2.QuadPart;
	return SZ_OK;
}
void IStream_BIND_InFile(IStream *is, CFileInStream *SzFIS) {
	SzFIS->file.handle = is;
	SzFIS->vt.Read	   = IStream_Read;
	SzFIS->vt.Seek	   = IStream_Seek;
}

static const ISzAlloc g_Alloc = {SzAlloc, SzFree};
std::vector<DATABLOCK> GetIconResourcesFromNarFStream(IStream *is) {
	//TODO
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	CFileInStream archiveStream;
	CLookToRead2  lookStream;
	CSzArEx		  db;
	SRes		  res;
	char16_t	 *temp	   = NULL;
	size_t		  tempSize = 0;
	allocImp			   = g_Alloc;
	allocTempImp		   = g_Alloc;
	std::vector<DATABLOCK> aret;

	IStream_BIND_InFile(is,&archiveStream);

	archiveStream.wres = 0;
	LookToRead2_CreateVTable(&lookStream, False);
	lookStream.buf = NULL;

	res = SZ_OK;

	{
		lookStream.buf = (Byte *)ISzAlloc_Alloc(&allocImp, kInputBufSize);
		if(!lookStream.buf)
			res = SZ_ERROR_MEM;
		else {
			lookStream.bufSize	  = kInputBufSize;
			lookStream.realStream = &archiveStream.vt;
			LookToRead2_Init(&lookStream);
		}
	}

	CrcGenerateTable();

	SzArEx_Init(&db);

	if(res == SZ_OK) {
		res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
	}

	if(res == SZ_OK) {
		UInt32 i;

		/*
		  if you need cache, use these 3 variables.
		  if you use external function, you can make these variable as static.
		*/
		UInt32 blockIndex	 = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
		Byte	 *outBuffer	 = 0;		   /* it must be 0 before first call for each new archive. */
		size_t outBufferSize = 0;		   /* it can have any value before first call (if outBuffer = 0) */

		for(i = 0; i < db.NumFiles; i++) {
			size_t offset			= 0;
			size_t outSizeProcessed = 0;
			// const CSzFileItem *f = db.Files + i;
			size_t		  len;
			const BoolInt isDir = SzArEx_IsDir(&db, i);
			if(isDir)
				continue;
			len = SzArEx_GetFileNameUtf16(&db, i, NULL);
			// len = SzArEx_GetFullNameLen(&db, i);

			if(len > tempSize) {
				SzFree(NULL, temp);
				tempSize = len;
				temp	 = (char16_t *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
				if(!temp) {
					res = SZ_ERROR_MEM;
					break;
				}
			}

			SzArEx_GetFileNameUtf16(&db, i, (UInt16 *)temp);
			::std::u16string_view name(temp,tempSize);
			if(!name.starts_with(u".nar_icon"))
				continue;
			{
				res = SzArEx_Extract(&db, &lookStream.vt, i,
									 &blockIndex, &outBuffer, &outBufferSize,
									 &offset, &outSizeProcessed,
									 &allocImp, &allocTempImp);
				if(res != SZ_OK)
					break;
			}
			DATABLOCK tmp = {outBuffer, outBufferSize};
			aret.push_back(COPY(&tmp));

			ISzAlloc_Free(&allocImp, outBuffer);
		}
	}

	SzFree(NULL, temp);
	SzArEx_Free(&db, &allocImp);
	ISzAlloc_Free(&allocImp, lookStream.buf);

	if(res == SZ_OK) {
		//Everything is Ok
		return aret;
	}

	return {};
}
HICON CreateIconFromMemory(PBYTE iconData, size_t iconDataSize) {
	int offset = LookupIconIdFromDirectory(iconData, TRUE);
	return CreateIconFromResource(iconData + offset, iconDataSize - offset, TRUE, 0x30000);
}
HBITMAP GetNARThumbnail(IStream* stream) {
	std::vector<DATABLOCK> dbs	  = GetIconResourcesFromNarFStream(stream);
	std::srand(std::time(NULL));
	auto size = dbs.size();
	HICON hIcon = NULL;
	while(size && !hIcon) {
		auto  index = rand() % size;
		auto &db	= dbs[index];
		hIcon		= CreateIconFromMemory(db.data, db.size);
		if(!hIcon) {
			dbs.erase(dbs.begin() + index);
			size--;
		}
	}
	HBITMAP result = HICON_to_HBITMAP(hIcon);
	for(auto&db:dbs)
		DESTORY(&db);
	return result;
}
