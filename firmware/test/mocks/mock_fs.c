#include "mock_fs.h"
#include <string.h>
#include <stdio.h>

#define MOCK_MAX_FILES   64
#define MOCK_MAX_FOLDERS 16
#define MOCK_PATH_LEN    128

static char mock_paths[MOCK_MAX_FILES][MOCK_PATH_LEN];
static int  mock_path_count = 0;

static char mock_empty_folders[MOCK_MAX_FOLDERS][32];
static int  mock_empty_folder_count = 0;

void mock_fs_reset(void)
{
    mock_path_count         = 0;
    mock_empty_folder_count = 0;
}

void mock_fs_add_folder(const char *folder)
{
    if (mock_empty_folder_count < MOCK_MAX_FOLDERS)
        strncpy(mock_empty_folders[mock_empty_folder_count++], folder, 31);
}

void mock_fs_add_file(const char *folder, const char *filename)
{
    if (mock_path_count < MOCK_MAX_FILES)
        snprintf(mock_paths[mock_path_count++], MOCK_PATH_LEN, "%s/%s", folder, filename);
}

static int ends_with(const char *s, const char *suffix)
{
    size_t sl = strlen(s), xl = strlen(suffix);
    return sl >= xl && strcmp(s + sl - xl, suffix) == 0;
}

static int is_audio(const char *path)
{
    return ends_with(path, ".wav") || ends_with(path, ".mp3") ||
           ends_with(path, ".WAV") || ends_with(path, ".MP3");
}

static int folder_known(const char *folder)
{
    for (int i = 0; i < mock_empty_folder_count; i++)
        if (strcmp(mock_empty_folders[i], folder) == 0) return 1;

    char prefix[MOCK_PATH_LEN];
    snprintf(prefix, sizeof(prefix), "%s/", folder);
    size_t plen = strlen(prefix);
    for (int i = 0; i < mock_path_count; i++)
        if (strncmp(mock_paths[i], prefix, plen) == 0) return 1;

    return 0;
}

static int mock_list_audio_files(const char *folder,
                                 const char **out_files, int max_count)
{
    if (!folder_known(folder))
        return -1;

    char prefix[MOCK_PATH_LEN];
    snprintf(prefix, sizeof(prefix), "%s/", folder);
    size_t plen = strlen(prefix);
    int count = 0;
    for (int i = 0; i < mock_path_count && count < max_count; i++)
    {
        if (strncmp(mock_paths[i], prefix, plen) == 0 && is_audio(mock_paths[i]))
            out_files[count++] = mock_paths[i];
    }
    return count;
}

const fs_interface_t MOCK_FS = {
    .list_audio_files = mock_list_audio_files,
};
