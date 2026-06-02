#include "file_loader.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void file_loader_init(file_loader_t *loader, const fs_interface_t *fs)
{
    loader->fs = fs;
}

const char *file_loader_get_soundbyte_path(file_loader_t *loader, int button_index)
{
    char folder[16];
    snprintf(folder, sizeof(folder), "%s%d", SB_FOLDER_PREFIX, button_index + 1);

    const char *files[FILE_LOADER_MAX_FILES];
    int count = loader->fs->list_audio_files(folder, files, FILE_LOADER_MAX_FILES);
    if (count <= 0)
        return NULL;

    return files[rand() % count];
}

int file_loader_get_music_playlist_length(file_loader_t *loader)
{
    const char *files[FILE_LOADER_MAX_FILES];
    int count = loader->fs->list_audio_files(MUSIC_FOLDER, files, FILE_LOADER_MAX_FILES);
    return count > 0 ? count : 0;
}
