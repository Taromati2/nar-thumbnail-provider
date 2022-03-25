
#include "GetThumbnail.h"

#include <string_view>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "../minizip-ng/mz.h"
#include "../minizip-ng/mz_strm.h"
#include "../minizip-ng/mz_zip.h"
#include "../minizip-ng/mz_zip_rw.h"

typedef struct mz_zip_reader_s {
    void        *zip_handle;
    void        *file_stream;
    void        *buffered_stream;
    void        *split_stream;
    void        *mem_stream;
    void        *hash;
    uint16_t    hash_algorithm;
    uint16_t    hash_digest_size;
    mz_zip_file *file_info;
    const char  *pattern;
    uint8_t     pattern_ignore_case;
    const char  *password;
    void        *overwrite_userdata;
    mz_zip_reader_overwrite_cb
                overwrite_cb;
    void        *password_userdata;
    mz_zip_reader_password_cb
                password_cb;
    void        *progress_userdata;
    mz_zip_reader_progress_cb
                progress_cb;
    uint32_t    progress_cb_interval_ms;
    void        *entry_userdata;
    mz_zip_reader_entry_cb
                entry_cb;
    uint8_t     raw;
    uint8_t     buffer[UINT16_MAX];
    int32_t     encoding;
    uint8_t     sign_required;
    uint8_t     cd_verified;
    uint8_t     cd_zipped;
    uint8_t     entry_verified;
    uint8_t     recover;
} mz_zip_reader;

/***************************************************************************/

typedef struct mz_stream_IStream_s {
	mz_stream stream;
	IStream	*ps;
} mz_stream_IStream;

/***************************************************************************/

int32_t mz_stream_IStream_open(void *stream, const char *path, int32_t mode) {
	mz_stream_IStream *mem = (mz_stream_IStream *)stream;
	int32_t		   err = MZ_OK;

	mem->ps = (IStream *)path;

	return err;
}

int32_t mz_stream_IStream_is_open(void *stream) {
	mz_stream_IStream *mem = (mz_stream_IStream *)stream;
	if(mem->ps == NULL)
		return MZ_OPEN_ERROR;
	return MZ_OK;
}

int32_t mz_stream_IStream_read(void *stream, void *buf, int32_t size) {
	mz_stream_IStream *mem = (mz_stream_IStream *)stream;
	ULONG			   ret;
	mem->ps->Read(buf,size,&ret);

	return ret;
}

int32_t mz_stream_IStream_write(void *stream, const void *buf, int32_t size) {
	return 0;
}

int64_t mz_stream_IStream_tell(void *stream) {
	mz_stream_IStream *mem = (mz_stream_IStream *)stream;
	ULARGE_INTEGER	   ret;
	mem->ps->Seek({0}, STREAM_SEEK_CUR,&ret );
	return ret.QuadPart;
}

int32_t mz_stream_IStream_seek(void *stream, int64_t offset, int32_t origin) {
	mz_stream_IStream *mem	   = (mz_stream_IStream *)stream;
	int64_t		   new_pos = 0;
	int32_t		   err	   = MZ_OK;
	LARGE_INTEGER	   offL	   = {};
	offL.QuadPart			   = offset;
	switch(origin) {
	case MZ_SEEK_CUR:
		mem->ps->Seek(offL, STREAM_SEEK_CUR, NULL);
		break;
	case MZ_SEEK_END:
		mem->ps->Seek(offL, STREAM_SEEK_END, NULL);
		break;
	case MZ_SEEK_SET:
		mem->ps->Seek(offL, STREAM_SEEK_SET, NULL);
		break;
	default:
		return MZ_SEEK_ERROR;
	}

	return MZ_OK;
}

int32_t mz_stream_IStream_close(void *stream) {
	/* We never return errors */
	return MZ_OK;
}

int32_t mz_stream_IStream_error(void *stream) {
	/* We never return errors */
	return MZ_OK;
}

extern mz_stream_vtbl mz_stream_IStream_vtbl;
void *mz_stream_IStream_create(void **stream) {
	mz_stream_IStream *mem = NULL;

	mem = (mz_stream_IStream *)MZ_ALLOC(sizeof(mz_stream_IStream));
	if(mem != NULL) {
		memset(mem, 0, sizeof(mz_stream_IStream));
		mem->stream.vtbl = &mz_stream_IStream_vtbl;
	}
	if(stream != NULL)
		*stream = mem;

	return mem;
}

void mz_stream_IStream_delete(void **stream) {
	mz_stream_IStream *mem = NULL;
	if(stream == NULL)
		return;
	mem = (mz_stream_IStream *)*stream;
	if(mem != NULL) {
		MZ_FREE(mem);
	}
	*stream = NULL;
}


/***************************************************************************/

static mz_stream_vtbl mz_stream_IStream_vtbl = {
    mz_stream_IStream_open,
    mz_stream_IStream_is_open,
    mz_stream_IStream_read,
    mz_stream_IStream_write,
    mz_stream_IStream_tell,
    mz_stream_IStream_seek,
    mz_stream_IStream_close,
    mz_stream_IStream_error,
    mz_stream_IStream_create,
    mz_stream_IStream_delete,
    NULL,
    NULL
};

//

int32_t mz_zip_reader_open_IStream(void *handle, IStream *ps) {
	mz_zip_reader *reader = (mz_zip_reader *)handle;
	int32_t		   err	  = MZ_OK;

	mz_zip_reader_close(handle);

	mz_stream_IStream_create(&reader->mem_stream);

	mz_stream_IStream_open(reader->mem_stream, (char*)ps, MZ_OPEN_MODE_READ);

	if(err == MZ_OK)
		err = mz_zip_reader_open(handle, reader->mem_stream);

	return err;
}

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

std::vector<DATABLOCK> GetIconResourcesFromNarFStream(IStream *is) {
	mz_zip_file *file_info = NULL;
	int32_t		 err	   = MZ_OK;
	void		 *reader = NULL;

	mz_zip_reader_create(&reader);
	err = mz_zip_reader_open_IStream(reader, is);
	{
		mz_zip_reader *treader = (mz_zip_reader *)reader;

	}
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
			DATABLOCK tmp = {(PBYTE)malloc(file_info->uncompressed_size), file_info->uncompressed_size};
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
HICON CreateIconFromMemory(PBYTE iconData, size_t iconDataSize,UINT cx) {
	auto offset = LookupIconIdFromDirectoryEx(iconData, TRUE, cx, cx, 0);
	return CreateIconFromResourceEx(iconData + offset, iconDataSize - offset, TRUE, 0x30000, cx, cx, 0);
}
HBITMAP GetNARThumbnail(UINT cx,IStream* stream) {
	std::vector<DATABLOCK> dbs	  = GetIconResourcesFromNarFStream(stream);
	std::srand(std::time(NULL));
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
