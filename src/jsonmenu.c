#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmodule.h>
#include <gio/gio.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>

#include <stdbool.h>
#include <json.h>

// ================================================================================================================= //

/* The json file containing the menu entries. */
#define ENTRIES_FILE g_build_filename ( g_get_user_config_dir(), "rofi-json-menu", NULL )

/* The format used to display entries without description. Parameters are: "name" */
#define ENTRY_FORMAT "%s"

/* The format used to display entries with description. Parameters are: "name" - "description" */
#define ENTRY_FORMAT_DESC "%s <span alpha='50%%'><i>(%s)</i></span>"

// ================================================================================================================= //

G_MODULE_EXPORT Mode mode;

typedef struct {
    char* name;
    char* description;
    char* cmd;
    char* icon_name;
    bool open_in_terminal;

    /* Rofi icon fetcher request ID. */
    uint32_t icon_fetcher_request;
} Entry;

typedef struct {
    /* The list of entries. */
    Entry* entries;
    /* The number of entries. */
    unsigned int num_entries;

    bool enable_icons;
    /* Loaded icons by their names */
} JsonMenuModePrivateData;

// ================================================================================================================= //

/**
 * @param entries_file An absolute path to the entries file.
 * @param sw The current mode.
 *
 * Reads the entries file and stores the entries in the private data. Allocates and initializes pd->entries and
 * sets pd->num_entries.
 */
bool set_entries ( char* entries_file, const Mode* sw );

// ================================================================================================================= //

static int json_menu_init ( Mode* sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        JsonMenuModePrivateData* pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, ( void * ) pd );

        /* Set the entries_file. */
        char* entries_file = NULL;
        if ( find_arg_str ( "-json-menu-file", &entries_file ) ) {
            if ( g_file_test ( entries_file, G_FILE_TEST_IS_REGULAR ) ) {
                entries_file = g_strdup ( entries_file );
            } else {
                fprintf ( stderr, "[json-menu] Could not find menu file: %s\n", entries_file );
                return false;
            }
        } else {
            entries_file = ENTRIES_FILE;
            if ( !g_file_test ( entries_file, G_FILE_TEST_IS_REGULAR ) ) {
                fprintf ( stderr, "[json-menu] Could not find menu file: %s\n", entries_file );
                return false;
            }
        }

        pd->enable_icons = ( find_arg ( "-json-menu-disable-icons" ) == -1 );

        bool success = set_entries ( entries_file, sw );
        g_free ( entries_file );

        return success;
    }

    return true;
}

static void json_menu_destroy ( Mode* sw )
{
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );
    mode_set_private_data ( sw, NULL );

    if ( pd != NULL ) {
        for ( int i = 0; i < pd->num_entries; i++ ) {
            Entry* entry = &( pd->entries[i] );
            g_free ( entry->name );
            g_free ( entry->description );
            g_free ( entry->cmd );
            g_free ( entry->icon_name );
        }
        g_free ( pd->entries );

        /* Fill with zeros, just in case. */
        memset ( ( void* ) pd , 0, sizeof ( pd ) );

        g_free ( pd );
    }
}

static unsigned int json_menu_get_num_entries ( const Mode* sw )
{
    const JsonMenuModePrivateData* pd = ( const JsonMenuModePrivateData * ) mode_get_private_data ( sw );

    if ( pd != NULL ) {
        return pd->num_entries;
    } else {
        return 0;
    }
}

static ModeMode json_menu_result ( Mode* sw, int mretv, char **input, unsigned int selected_line )
{
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );

    ModeMode retv = RELOAD_DIALOG;

    /* Handle Return and Shift+Return*/
    if ( mretv & MENU_OK ) {
        Entry* entry = &pd->entries[selected_line];

        /* Execute with arguments, if the user typed arguments after the name. */
        char* cmd = NULL;
        if ( g_str_has_prefix ( *input, entry->name ) ) {
            int name_len = strlen ( entry->name );
            cmd = g_strconcat ( entry->cmd, *input + name_len, NULL );
        } else {
            cmd = g_strdup ( entry->cmd );
        }

        helper_execute_command ( NULL, cmd, entry->open_in_terminal, NULL );
        g_free ( cmd );
        retv = MODE_EXIT;

    /* Handle Control+Return or custom input. */
    } else if ( mretv & MENU_CUSTOM_INPUT ) {
        helper_execute_command ( NULL, *input, false, NULL );
        retv = MODE_EXIT;

    /* Default actions */
    } else if ( mretv & MENU_CANCEL ) {
        retv = MODE_EXIT;
    } else if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }

    return retv;
}

static int json_menu_token_match ( const Mode* sw, rofi_int_matcher **tokens, unsigned int index )
{
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );
    Entry *entry = &pd->entries[index];

    if ( tokens[0] == NULL ) {
        return true;
    }

    const char* first_token = g_regex_get_pattern ( tokens[0]->regex );

    /* Match if the name contains the input, or the input contains the name with a whitespace afterwards (for arguments). */
    if ( tokens[1] == NULL ) {
        if ( g_str_has_prefix ( entry->name, first_token ) ) {
            return true;
        }
    } else {
        if ( g_str_equal ( entry->name, first_token ) ) {
            return true;
        }
    }

    return false;
}

static char* json_menu_get_display_value ( const Mode* sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );
    Entry *entry = &pd->entries[selected_line];

    if ( !get_entry ) return NULL;

    // MARKUP flag, not defined in accessible headers
    *state |= 8;

    if ( entry->description != NULL ) {
        return g_strdup_printf ( ENTRY_FORMAT_DESC, entry->name, entry->description );
    } else {
        return g_strdup_printf ( ENTRY_FORMAT, entry->name );
    }
}

char* json_menu_get_completion ( const Mode *sw, unsigned int selected_line )
{
    const JsonMenuModePrivateData* pd = ( const JsonMenuModePrivateData * ) mode_get_private_data ( sw );
    Entry *entry = &pd->entries[selected_line];

    return entry->name;
}

static cairo_surface_t* json_menu_get_icon ( const Mode* sw, unsigned int selected_line, int height )
{
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );
    Entry *entry = &pd->entries[selected_line];

    if ( pd->enable_icons && entry->icon_name != NULL ) {
        if ( entry->icon_fetcher_request <= 0 ) {
            entry->icon_fetcher_request = rofi_icon_fetcher_query ( entry->icon_name, height );
        }
        return rofi_icon_fetcher_get ( entry->icon_fetcher_request );
    } else {
        return NULL;
    }
}

// ================================================================================================================= //

bool set_entries ( char* entries_file, const Mode* sw ) {
    JsonMenuModePrivateData* pd = ( JsonMenuModePrivateData * ) mode_get_private_data ( sw );

    char* file_content = NULL;

    if ( !g_file_get_contents ( entries_file, &file_content, NULL, NULL ) ) {
        fprintf ( stderr, "[json-menu] Could not read entries file: %s\n", entries_file );
        return false;
    }

    enum json_tokener_error error;
    json_object *entries = json_tokener_parse_verbose ( file_content, &error );
    if ( error != json_tokener_success ) {
        fprintf ( stderr, "[json-menu] Could not parse entries file: %s\n", entries_file );
        return false;
    }

    json_object_object_foreach ( entries, key, value ) {
        pd->entries = g_realloc ( pd->entries, ( pd->num_entries + 1 ) * sizeof ( Entry ) );
        Entry* entry = &pd->entries[pd->num_entries];
        pd->num_entries++;

        entry->name = g_strdup ( key );

        json_object* cmd;
        if ( json_object_object_get_ex ( value, "cmd", &cmd ) ) {
            entry->cmd = g_strdup ( json_object_get_string ( cmd ) );
        } else {
            entry->cmd = g_strdup ( key );
        }

        json_object* description;
        if ( json_object_object_get_ex ( value, "description", &description ) ) {
            entry->description = g_strdup ( json_object_get_string ( description ) );
        } else {
            entry->description = NULL;
        }

        json_object* icon;
        if ( json_object_object_get_ex ( value, "icon", &icon ) ) {
            entry->icon_name = g_strdup ( json_object_get_string ( icon ) );
        } else {
            entry->icon_name = g_strdup ( key );
        }

        json_object* terminal;
        if ( json_object_object_get_ex ( value, "terminal", &terminal ) ) {
            entry->open_in_terminal = json_object_get_boolean ( terminal );
        } else {
            entry->open_in_terminal = false;
        }
    }

    json_object_put ( entries );

    return true;
}

// ================================================================================================================= //

Mode mode =
{
    .abi_version        = ABI_VERSION,
    .name               = "json-menu",
    .cfg_name_key       = "display-json-menu",
    ._init              = json_menu_init,
    ._get_num_entries   = json_menu_get_num_entries,
    ._result            = json_menu_result,
    ._destroy           = json_menu_destroy,
    ._token_match       = json_menu_token_match,
    ._get_display_value = json_menu_get_display_value,
    ._get_icon          = json_menu_get_icon,
    ._get_message       = NULL,
    ._get_completion    = json_menu_get_completion,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL,
};
