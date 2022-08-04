
#include "GetThumbnail.h"

#include <string_view>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "../minizip-ng/mz.h"
#include "../minizip-ng/mz_strm.h"
#include "../minizip-ng/mz_zip.h"
#include "../minizip-ng/mz_zip_rw.h"

#include "zip_reader_open_IStream.h"

struct DATABLOCK {
	PBYTE data;
	int32_t size;
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

std::vector<DATABLOCK> GetIconResourcesFromNarFStream(IStream *is) {
	mz_zip_file *file_info = NULL;
	int32_t		 err	   = MZ_OK;
	void		 *reader = NULL;

	mz_zip_reader_create(&reader);
	err = mz_zip_reader_open_IStream(reader, is);

	if(err != MZ_OK) {
		mz_zip_reader_delete(&reader);
		return {};
	}

	err = mz_zip_reader_goto_first_entry(reader);

	if(err != MZ_OK && err != MZ_END_OF_LIST) {
		mz_zip_reader_delete(&reader);
		return {};
	}

	std::vector<DATABLOCK> aret;
	/* Enumerate all entries in the archive */
	do {
		err = mz_zip_reader_entry_get_info(reader, &file_info);

		if(err != MZ_OK) {
			break;
		}

		if(file_info->flag & MZ_ZIP_FLAG_ENCRYPTED || !file_info->uncompressed_size)
			goto next_file;
		{
			::std::u8string_view name((char8_t *)file_info->filename, file_info->filename_size);
			if(!name.starts_with(u8".nar_icon/"))
				goto next_file;
			if(file_info->uncompressed_size>=INT32_MAX)
				goto next_file;
			int32_t	  data_size = (int32_t)file_info->uncompressed_size;
			DATABLOCK tmp		= {(PBYTE)malloc(data_size), data_size};
			if(tmp.data) {
				mz_zip_reader_entry_save_buffer(reader, tmp.data, tmp.size);
				aret.push_back(tmp);
			}
		}

	next_file:
		err = mz_zip_reader_goto_next_entry(reader);

		if(err != MZ_OK && err != MZ_END_OF_LIST) {
			break;
		}
	} while(err == MZ_OK);

	mz_zip_reader_delete(&reader);

	if(err == MZ_END_OF_LIST)
		return aret;

	for(auto &db: aret)
		DESTORY(&db);
	return {};
}
HICON CreateIconFromMemory(PBYTE iconData, int32_t iconDataSize, UINT cx) {
	auto offset = LookupIconIdFromDirectoryEx(iconData, TRUE, cx, cx, 0);
	return CreateIconFromResourceEx(iconData + offset, iconDataSize - offset, TRUE, 0x30000, cx, cx, 0);
}
HBITMAP GetNARThumbnail(UINT cx,IStream* stream) {
	std::vector<DATABLOCK> dbs	  = GetIconResourcesFromNarFStream(stream);
	std::srand((unsigned)std::time(NULL));
	auto size = dbs.size();
	HICON hIcon = NULL;
	while(size && !hIcon) {
		auto  index = rand() % size;
		auto &db	= dbs[index];
		hIcon		= CreateIconFromMemory(db.data, db.size, cx);
		if(!hIcon) {
			DESTORY(&db);
			dbs.erase(dbs.begin() + index);
			size--;
		}
	}
	HBITMAP result = HICON_to_HBITMAP(hIcon);
	for(auto&db:dbs)
		DESTORY(&db);
	return result;
}
