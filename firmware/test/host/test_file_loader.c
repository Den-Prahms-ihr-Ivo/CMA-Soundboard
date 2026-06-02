#include "unity.h"
#include "file_loader.h"
#include "mock_fs.h"
#include <string.h>

static file_loader_t loader;

void setUp(void)
{
    mock_fs_reset();
    file_loader_init(&loader, &MOCK_FS);
}

void tearDown(void) {}

// –– REQ-PLAY-003 –––––––––––––––––––––––––––––––––––––––––––––––

void test_loader_maps_button_index_to_folder(void)
{
    // Button 0 → SB1/, button 5 → SB6/.
    // If the loader uses the wrong folder the mock returns NULL.
    mock_fs_add_file("SB1", "clap.wav");
    mock_fs_add_file("SB6", "horn.mp3");

    const char *path0 = file_loader_get_soundbyte_path(&loader, 0);
    TEST_ASSERT_NOT_NULL(path0);
    TEST_ASSERT_NOT_NULL(strstr(path0, "SB1"));

    const char *path5 = file_loader_get_soundbyte_path(&loader, 5);
    TEST_ASSERT_NOT_NULL(path5);
    TEST_ASSERT_NOT_NULL(strstr(path5, "SB6"));
}

void test_loader_returns_null_for_empty_folder(void)
{
    // Folder exists but contains no audio files → NULL.
    mock_fs_add_folder("SB1");
    TEST_ASSERT_NULL(file_loader_get_soundbyte_path(&loader, 0));
}

void test_loader_returns_null_for_missing_folder(void)
{
    // No folder registered at all → NULL, no crash.
    TEST_ASSERT_NULL(file_loader_get_soundbyte_path(&loader, 0));
}

void test_loader_ignores_non_audio_files(void)
{
    // Folder contains only non-audio files → treated as empty → NULL.
    mock_fs_add_file("SB1", "readme.txt");
    mock_fs_add_file("SB1", "cover.jpg");
    TEST_ASSERT_NULL(file_loader_get_soundbyte_path(&loader, 0));
}

void test_loader_reports_playlist_length(void)
{
    // MUSIC/ file count is returned accurately.
    mock_fs_add_file("MUSIC", "track1.mp3");
    mock_fs_add_file("MUSIC", "track2.mp3");
    mock_fs_add_file("MUSIC", "track3.wav");
    TEST_ASSERT_EQUAL_INT(3, file_loader_get_music_playlist_length(&loader));
}

void test_loader_folder_constants_are_named(void)
{
    // All three folder constants must have the specified values.
    TEST_ASSERT_EQUAL_STRING("SB",     SB_FOLDER_PREFIX);
    TEST_ASSERT_EQUAL_STRING("MUSIC",  MUSIC_FOLDER);
    TEST_ASSERT_EQUAL_STRING("TABATA", TABATA_FOLDER);
}

void test_loader_returns_path_for_audio_file(void)
{
    // Audio file present → non-NULL path that names the correct folder.
    mock_fs_add_file("SB3", "boom.wav");
    const char *path = file_loader_get_soundbyte_path(&loader, 2); // index 2 → SB3
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_NOT_NULL(strstr(path, "SB3"));
}

void test_loader_music_count_excludes_non_audio(void)
{
    // Non-audio files in MUSIC/ must not inflate playlist_length.
    mock_fs_add_file("MUSIC", "track1.mp3");
    mock_fs_add_file("MUSIC", "track2.wav");
    mock_fs_add_file("MUSIC", "artwork.jpg");
    mock_fs_add_file("MUSIC", "metadata.xml");
    TEST_ASSERT_EQUAL_INT(2, file_loader_get_music_playlist_length(&loader));
}

int main(void)
{
    UNITY_BEGIN();

    // –– REQ-PLAY-003 ––––––––––––––––––––––––––––––––––––––––––
    RUN_TEST(test_loader_maps_button_index_to_folder);
    RUN_TEST(test_loader_returns_null_for_empty_folder);
    RUN_TEST(test_loader_returns_null_for_missing_folder);
    RUN_TEST(test_loader_ignores_non_audio_files);
    RUN_TEST(test_loader_reports_playlist_length);
    RUN_TEST(test_loader_folder_constants_are_named);
    RUN_TEST(test_loader_returns_path_for_audio_file);
    RUN_TEST(test_loader_music_count_excludes_non_audio);

    return UNITY_END();
}
