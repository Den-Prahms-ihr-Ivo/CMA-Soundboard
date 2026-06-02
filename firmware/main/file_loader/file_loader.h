#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#define SB_FOLDER_PREFIX "SB"
#define MUSIC_FOLDER     "MUSIC"
#define TABATA_FOLDER    "TABATA"

#define FILE_LOADER_MAX_FILES 64

/* Filesystem interface — injected so the module is host-testable.
 * list_audio_files: returns -1 if folder is missing, else the count
 * of .wav/.mp3 files in the folder (0 = empty folder).
 * out_files is filled with up to max_count file paths. */
typedef struct
{
    int (*list_audio_files)(const char *folder,
                            const char **out_files, int max_count);
} fs_interface_t;

typedef struct
{
    const fs_interface_t *fs;
} file_loader_t;

void        file_loader_init(file_loader_t *loader, const fs_interface_t *fs);

/* Returns a path to a randomly-selected audio file for the given soundboard
 * button index (0–5 → SB1/–SB6/), or NULL if the folder is missing or empty. */
const char *file_loader_get_soundbyte_path(file_loader_t *loader, int button_index);

/* Returns the number of audio files in the MUSIC/ folder.
 * Passes this count to the caller so it can call
 * soundboard_mixer_set_playlist_length(). */
int         file_loader_get_music_playlist_length(file_loader_t *loader);

#endif /* FILE_LOADER_H */
