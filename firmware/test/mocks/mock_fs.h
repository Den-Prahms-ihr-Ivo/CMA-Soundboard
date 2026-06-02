#ifndef MOCK_FS_H
#define MOCK_FS_H

#include "file_loader.h"

/* Reset all mock filesystem state. Call at the start of every test. */
void mock_fs_reset(void);

/* Register an empty folder (folder_exists = true, no files). */
void mock_fs_add_folder(const char *folder);

/* Register a file.  The mock stores the full path as "folder/filename".
 * list_audio_files() counts only .wav and .mp3 entries. */
void mock_fs_add_file(const char *folder, const char *filename);

/* fs_interface_t instance backed by the mock state above. */
extern const fs_interface_t MOCK_FS;

#endif /* MOCK_FS_H */
